// Unit Includes
#include "mounter_proxy.h"

// Qt Includes
#include <QAuthenticator>
#include <QUrlQuery>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>

// Qx Includes
#include <qx/core/qx-string.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// MounterError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
MounterProxyError::MounterProxyError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool MounterProxyError::isValid() const { return mType != NoError; }
QString MounterProxyError::specific() const { return mSpecific; }
MounterProxyError::Type MounterProxyError::type() const { return mType; }

//Private:
Qx::Severity MounterProxyError::deriveSeverity() const { return Qx::Critical; }
quint32 MounterProxyError::deriveValue() const { return mType; }
QString MounterProxyError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString MounterProxyError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// Mounter
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
MounterProxy::MounterProxy(QObject* parent) :
    QObject(parent),
    mMounting(false),
    mProxyServerPort(0)
{
    // Setup Network Access Manager
    mNam.setAutoDeleteReplies(true);
    mNam.setTransferTimeout(PROXY_TRANSFER_TIMEOUT);

    // Connections - Work
    connect(&mNam, &QNetworkAccessManager::finished, this, &MounterProxy::proxyMountFinishedHandler);

    /* Network check (none of these should be triggered, they are here in case a FP update would required
     * them to be used as to help make that clear in the logs when the update causes this to stop working).
     */
    connect(&mNam, &QNetworkAccessManager::authenticationRequired, this, [this](){
        signalEventOccurred(u"Unexpected use of authentication by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, [this](){
        signalEventOccurred(u"Unexpected use of PSK authentication by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::proxyAuthenticationRequired, this, [this](){
        signalEventOccurred(u"Unexpected use of proxy by PHP server!"_s);
    });
    connect(&mNam, &QNetworkAccessManager::sslErrors, this, [this](QNetworkReply* reply, const QList<QSslError>& errors){
        Q_UNUSED(reply);
        QString errStrList = Qx::String::join(errors, [](const QSslError& err){ return err.errorString(); }, u","_s);
        signalEventOccurred(u"Unexpected SSL errors from PHP server! {"_s + errStrList + u"}"_s"}");
    });
}

//-Instance Functions---------------------------------------------------------------------------------------------------------
//Private:
void MounterProxy::finish(const MounterProxyError& errorState)
{
    mMounting = false;
    emit mountFinished(errorState);
}

void MounterProxy::noteProxyRequest(QNetworkAccessManager::Operation op, const QUrl& url, QByteArrayView data)
{
    signalEventOccurred(EVENT_REQUEST_SENT.arg(ENUM_NAME(op), url.toString(), QString::fromLatin1(data)));
}

void MounterProxy::noteProxyResponse(const QString& response)
{
    signalEventOccurred(EVENT_PROXY_RESPONSE.arg(response));
}

void MounterProxy::signalEventOccurred(const QString& event) { emit eventOccurred(NAME, event); }
void MounterProxy::signalErrorOccurred(const MounterProxyError& errorMessage) { emit errorOccurred(NAME, errorMessage); }

//Public:
bool MounterProxy::isMounting() { return mMounting; }

quint16 MounterProxy::proxyServerPort() const { return mProxyServerPort; }
QString MounterProxy::filePath() const { return mFilePath; }

void MounterProxy::setProxyServerPort(quint16 port) { mProxyServerPort = port; }
void MounterProxy::setFilePath(const QString& path) { mFilePath = QDir::toNativeSeparators(path); }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void MounterProxy::proxyMountFinishedHandler(QNetworkReply* reply)
{
    assert(reply == mProxyMountReply.get());

    MounterProxyError err;

    if(reply->error() != QNetworkReply::NoError)
    {
        err = MounterProxyError(MounterProxyError::ProxyMount, reply->errorString());
        signalErrorOccurred(err);
    }
    else
    {
        QByteArray response = reply->readAll();
        noteProxyResponse(QString::fromLatin1(response));
    }

    finish(err);
}

//Public Slots:
void MounterProxy::mount()
{
    signalEventOccurred(EVENT_MOUNTING);

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
    // These headers are used by the stock launcher, but don't seem to be needed
    //
    // mountReq.setRawHeader("Connection"_ba, "close"_ba);
    // mountReq.setRawHeader("Accept"_ba, "application/json, text/plain, */*"_ba);
    // mountReq.setRawHeader("Accept-Encoding"_ba, "gzip, compress, deflate, br"_ba);

    // Data
    QJsonDocument jdData(QJsonObject{{u"filePath"_s, mFilePath}});
    QByteArray data = jdData.toJson(QJsonDocument::Compact);

    //-POST Request---------------------------------
    mProxyMountReply = mNam.post(mountReq, data);

    // Log request
    noteProxyRequest(mProxyMountReply->operation(), mountUrl, data);

    // Await finished() signal...
}

void MounterProxy::abort()
{
    if(mProxyMountReply && mProxyMountReply->isRunning())
        mProxyMountReply->abort();
}
