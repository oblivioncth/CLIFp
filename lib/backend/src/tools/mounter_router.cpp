// Unit Includes
#include "mounter_router.h"

// Qt Includes
#include <QAuthenticator>
#include <QUrlQuery>

// Qx Includes
#include <qx/core/qx-string.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// MounterRouterError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
MounterRouterError::MounterRouterError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool MounterRouterError::isValid() const { return mType != NoError; }
QString MounterRouterError::specific() const { return mSpecific; }
MounterRouterError::Type MounterRouterError::type() const { return mType; }

//Private:
Qx::Severity MounterRouterError::deriveSeverity() const { return Qx::Critical; }
quint32 MounterRouterError::deriveValue() const { return mType; }
QString MounterRouterError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString MounterRouterError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// Mounter
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
MounterRouter::MounterRouter(QObject* parent, Director* director) :
    QObject(parent),
    Directorate(director),
    mMounting(false),
    mRouterPort(0)
{
    // Setup Network Access Manager
    mNam.setAutoDeleteReplies(true);
    mNam.setTransferTimeout(PHP_TRANSFER_TIMEOUT);

    // Connections
    connect(&mNam, &QNetworkAccessManager::finished, this, &MounterRouter::mountFinishedHandler);

    /* Network check (none of these should be triggered, they are here in case a FP update would required
     * them to be used as to help make that clear in the logs when the update causes this to stop working).
     */
    connect(&mNam, &QNetworkAccessManager::authenticationRequired, this, [this](){
        logEvent(u"Unexpected use of authentication by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, [this](){
        logEvent(u"Unexpected use of PSK authentication by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::proxyAuthenticationRequired, this, [this](){
        logEvent(u"Unexpected use of proxy by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::sslErrors, this, [this](QNetworkReply* reply, const QList<QSslError>& errors){
        Q_UNUSED(reply);
        QString errStrList = Qx::String::join(errors, [](const QSslError& err){ return err.errorString(); }, u","_s);
        logEvent(u"Unexpected SSL errors from PHP server! {"_s + errStrList + u"}"_s"}");
    });
}

//-Instance Functions---------------------------------------------------------------------------------------------------------
//Private:
void MounterRouter::finish(const MounterRouterError& result)
{
    mMounting = false;
    emit mountFinished(result);
}

//Public:
QString MounterRouter::name() const { return NAME; }
bool MounterRouter::isMounting() { return mMounting; }

quint16 MounterRouter::routerPort() const { return mRouterPort; }
QString MounterRouter::mountValue() const { return mMountValue; }

void MounterRouter::setRouterPort(quint16 port) { mRouterPort = port; }
void MounterRouter::setMountValue(const QString& value) { mMountValue = value; }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void MounterRouter::mountFinishedHandler(QNetworkReply* reply)
{
    assert(reply == mRouterMountReply.get());

    MounterRouterError err;

    // FP (as of 11) is currently bugged and is expected to give an internal server error so it must be ignored
    if(reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::InternalServerError)
    {
        err = MounterRouterError(MounterRouterError::Failed, reply->errorString());
        postDirective<DError>(err);
    }
    else
    {
        QByteArray response = reply->readAll();
        logEvent(EVENT_ROUTER_RESPONSE.arg(response));
    }

    finish(err);
}

//Public Slots:
void MounterRouter::mount()
{
    logEvent(EVENT_MOUNTING_THROUGH_ROUTER);

    // Create mount request
    QUrl mountUrl;
    mountUrl.setScheme(u"http"_s);
    mountUrl.setHost(u"127.0.0.1"_s);
    mountUrl.setPort(mRouterPort);
    mountUrl.setPath(u"/mount.php"_s);

    QUrlQuery query;
    QString queryKey = u"file"_s;
    QString queryValue = QUrl::toPercentEncoding(mMountValue);
    query.addQueryItem(queryKey, queryValue);
    mountUrl.setQuery(query);

    QNetworkRequest mountReq(mountUrl);

    // GET request
    mRouterMountReply = mNam.get(mountReq);

    // Log request
    logEvent(EVENT_REQUEST_SENT.arg(ENUM_NAME(mRouterMountReply->operation()), mountUrl.toString()));

    // Await finished() signal...
}

void MounterRouter::abort()
{
    if(mRouterMountReply && mRouterMountReply->isRunning())
        mRouterMountReply->abort();
}
