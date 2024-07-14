// Unit Includes
#include "mounter_game_server.h"

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
MounterGameServerError::MounterGameServerError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool MounterGameServerError::isValid() const { return mType != NoError; }
QString MounterGameServerError::specific() const { return mSpecific; }
MounterGameServerError::Type MounterGameServerError::type() const { return mType; }

//Private:
Qx::Severity MounterGameServerError::deriveSeverity() const { return Qx::Critical; }
quint32 MounterGameServerError::deriveValue() const { return mType; }
QString MounterGameServerError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString MounterGameServerError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// Mounter
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
MounterGameServer::MounterGameServer(QObject* parent) :
    QObject(parent),
    mMounting(false),
    mGameServerPort(0)
{
    // Setup Network Access Manager
    mNam.setAutoDeleteReplies(true);
    mNam.setTransferTimeout(GAME_SERVER_TRANSFER_TIMEOUT);

    // Connections - Work
    connect(&mNam, &QNetworkAccessManager::finished, this, &MounterGameServer::gameServerMountFinishedHandler);

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
void MounterGameServer::finish(const MounterGameServerError& errorState)
{
    mMounting = false;
    emit mountFinished(errorState);
}

void MounterGameServer::noteProxyRequest(QNetworkAccessManager::Operation op, const QUrl& url, QByteArrayView data)
{
    signalEventOccurred(EVENT_REQUEST_SENT.arg(ENUM_NAME(op), url.toString(), QString::fromLatin1(data)));
}

void MounterGameServer::noteProxyResponse(const QString& response)
{
    signalEventOccurred(EVENT_GAMESERVER_RESPONSE.arg(response));
}

void MounterGameServer::signalEventOccurred(const QString& event) { emit eventOccurred(NAME, event); }
void MounterGameServer::signalErrorOccurred(const MounterGameServerError& errorMessage) { emit errorOccurred(NAME, errorMessage); }

//Public:
bool MounterGameServer::isMounting() { return mMounting; }

quint16 MounterGameServer::gameServerPort() const { return mGameServerPort; }
QString MounterGameServer::filePath() const { return mFilePath; }

void MounterGameServer::setGameServerPort(quint16 port) { mGameServerPort = port; }
void MounterGameServer::setFilePath(const QString& path) { mFilePath = QDir::toNativeSeparators(path); }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void MounterGameServer::gameServerMountFinishedHandler(QNetworkReply* reply)
{
    assert(reply == mGameServerMountReply.get());

    MounterGameServerError err;

    if(reply->error() != QNetworkReply::NoError)
    {
        err = MounterGameServerError(MounterGameServerError::ProxyMount, reply->errorString());
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
void MounterGameServer::mount()
{
    signalEventOccurred(EVENT_MOUNTING);

    //-Create mount request-------------------------

    // Url
    QUrl mountUrl;
    mountUrl.setScheme(u"http"_s);
    mountUrl.setHost(u"localhost"_s);
    mountUrl.setPort(mGameServerPort);
    mountUrl.setPath(u"/fpProxy/api/mountzip"_s);

    // Req
    QNetworkRequest mountReq(mountUrl);

    // Header
    mountReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"_ba);
    /* These following headers are used by the stock launcher, but don't seem to be needed;
     * however, we include them anyway out of posterity in hopes of avoiding future issues
     * in case FPGS starts caring
     */
    mountReq.setHeader(QNetworkRequest::UserAgentHeader, "axios/1.6.8"_ba);
    mountReq.setRawHeader("Connection"_ba, "close"_ba);
    mountReq.setRawHeader("Accept"_ba, "application/json, text/plain, */*"_ba);
    mountReq.setRawHeader("Accept-Encoding"_ba, "gzip, compress, deflate, br"_ba);

    // Data
    QJsonDocument jdData(QJsonObject{{u"filePath"_s, mFilePath}});
    QByteArray data = jdData.toJson(QJsonDocument::Compact);

    //-POST Request---------------------------------
    mGameServerMountReply = mNam.post(mountReq, data);

    // Log request
    noteProxyRequest(mGameServerMountReply->operation(), mountUrl, data);

    // Await finished() signal...
}

void MounterGameServer::abort()
{
    if(mGameServerMountReply && mGameServerMountReply->isRunning())
        mGameServerMountReply->abort();
}
