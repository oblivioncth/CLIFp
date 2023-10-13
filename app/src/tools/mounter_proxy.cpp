// Unit Includes
#include "mounter_proxy.h"

// Qt Includes
#include <QAuthenticator>
#include <QUrlQuery>
#include <QFileInfo>

// Qx Includes
#include <qx/core/qx-json.h>
#include <qx/core/qx-base85.h>
#include <qx/core/qx-string.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// MounterError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
MounterError::MounterError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool MounterError::isValid() const { return mType != NoError; }
QString MounterError::specific() const { return mSpecific; }
MounterError::Type MounterError::type() const { return mType; }

//Private:
Qx::Severity MounterError::deriveSeverity() const { return Qx::Critical; }
quint32 MounterError::deriveValue() const { return mType; }
QString MounterError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString MounterError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// Mounter
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
Mounter::Mounter(QObject* parent) :
    QObject(parent),
    mMounting(false),
    mProxyServerPort(0)
{
    // Setup Network Access Manager
    mNam.setAutoDeleteReplies(true);
    mNam.setTransferTimeout(PROXY_TRANSFER_TIMEOUT);

    // Connections - Work
    connect(&mNam, &QNetworkAccessManager::finished, this, &Mounter::proxyMountFinishedHandler);

    /* Network check (none of these should be triggered, they are here in case a FP update would required
     * them to be used as to help make that clear in the logs when the update causes this to stop working).
     */
    connect(&mNam, &QNetworkAccessManager::authenticationRequired, this, [this](){
        emit eventOccured(NAME, u"Unexpected use of authentication by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, [this](){
        emit eventOccured(NAME, u"Unexpected use of PSK authentication by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::proxyAuthenticationRequired, this, [this](){
        emit eventOccured(NAME, u"Unexpected use of proxy by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::sslErrors, this, [this](QNetworkReply* reply, const QList<QSslError>& errors){
        Q_UNUSED(reply);
        QString errStrList = Qx::String::join(errors, [](const QSslError& err){ return err.errorString(); }, u","_s);
        emit eventOccured(NAME, u"Unexpected SSL errors from PHP server! {"_s + errStrList + u"}"_s"}");
    });
}

//-Instance Functions---------------------------------------------------------------------------------------------------------
//Private:
void Mounter::finish(const MounterError& errorState)
{
    mMounting = false;
    emit mountFinished(errorState);
}

void Mounter::postMountToServer(QStringView filePath)
{
    emit eventOccured(NAME, EVENT_MOUNTING);

    //-Create mount request-------------------------

    // Url
    QUrl mountUrl;
    mountUrl.setScheme(u"http"_s);
    mountUrl.setHost(u"localhost"_s);
    mountUrl.setPort(mProxyServerPort);
    mountUrl.setPath(u"/fpProxy/api/mountzip"_s);

    // Req
    QNetworkRequest mountReq(mountUrl);

    // Header
    mountReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Data (could use QJsonDocument but for such a simple object that's overkill
    QByteArray data = "{\"filePath\":\""_ba + filePath.toLatin1() + "\"}"_ba;

    //-POST Request---------------------------------
    mProxyMountReply = mNam.post(mountReq, data);

    // Log request
    noteProxyRequest(mProxyMountReply->operation(), mountUrl, data);

    // Await finished() signal...
}

void Mounter::noteProxyRequest(QNetworkAccessManager::Operation op, const QUrl& url, QByteArrayView data)
{
    emit eventOccured(NAME, EVENT_REQUEST_SENT.arg(ENUM_NAME(op), url.toString(), QString::fromLatin1(data)));
}

void Mounter::noteProxyResponse(const QString& response)
{
    emit eventOccured(NAME, EVENT_PROXY_RESPONSE.arg(response));
}

//Public:
bool Mounter::isMounting() { return mMounting; }
quint16 Mounter::proxyServerPort() const { return mProxyServerPort; }
void Mounter::setProxyServerPort(quint16 port) { mProxyServerPort = port; }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void Mounter::proxyMountFinishedHandler(QNetworkReply* reply)
{
    assert(reply == mProxyMountReply.get());

    if(reply->error() != QNetworkReply::NoError)
    {
        MounterError err(MounterError::ProxyMount, reply->errorString());

        emit errorOccured(NAME, err);
        finish(err);
    }
    else
    {
        QByteArray response = reply->readAll();
        noteProxyResponse(QString::fromLatin1(response));
        finish(MounterError());
    }
}

//Public Slots:
void Mounter::mount(QStringView filePath)
{
    // Update state
    mMounting = true;
    emit mountProgress(0);
    emit mountProgressMaximumChanged(0); // Cause busy state

    // Send mount request
    postMountToServer(filePath);
}

void Mounter::abort()
{
    if(mProxyMountReply && mProxyMountReply->isRunning())
        mProxyMountReply->abort();
}
