// Unit Includes
#include "mounter.h"

// Qt Includes
#include <QAuthenticator>
#include <QRandomGenerator>
#include <QUrlQuery>
#include <QFileInfo>

// Qx Includes
#include <qx/core/qx-json.h>
#include <qx/core/qx-base85.h>
#include <qx/core/qx-string.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// Mounter
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
Mounter::Mounter(QObject* parent) :
    QObject(parent),
    mMounting(false),
    mErrorStatus(ErrorCode::NO_ERR),
    mWebServerPort(0),
    mQemuMounter(QHostAddress::LocalHost, 0),
    mQemuProdder(QHostAddress::LocalHost, 0), // Currently not used
    mQemuEnabled(true),
    mCompletedQemuCommands(0)
{
    // Setup Network Access Manager
    mNam.setAutoDeleteReplies(true);
    mNam.setTransferTimeout(PHP_TRANSFER_TIMEOUT);

    // Setup QMPI
    mQemuMounter.setTransactionTimeout(QMP_TRANSACTION_TIMEOUT);

    // Connections - Work
    connect(&mQemuMounter, &Qmpi::readyForCommands, this, &Mounter::qmpiReadyForCommandsHandler);
    connect(&mQemuMounter, &Qmpi::finished, this, &Mounter::qmpiFinishedHandler);
    connect(&mNam, &QNetworkAccessManager::finished, this, &Mounter::phpMountFinishedHandler);

    // Connections - Error
    connect(&mQemuMounter, &Qmpi::connectionErrorOccurred, this, &Mounter::qmpiConnectionErrorHandler);
    connect(&mQemuMounter, &Qmpi::communicationErrorOccurred, this, &Mounter::qmpiCommunicationErrorHandler);
    connect(&mQemuMounter, &Qmpi::errorResponseReceived, this, &Mounter::qmpiCommandErrorHandler);

    // Connections - Log
    connect(&mQemuMounter, &Qmpi::connected, this, &Mounter::qmpiConnectedHandler);
    connect(&mQemuMounter, &Qmpi::responseReceived, this, &Mounter::qmpiCommandResponseHandler);
    connect(&mQemuMounter, &Qmpi::eventReceived, this, &Mounter::qmpiEventOccurredHandler);

    /* Network check (none of these should be triggered, they are here in case a FP update would required
     * them to be used as to help make that clear in the logs when the update causes this to stop working).
     */
    connect(&mNam, &QNetworkAccessManager::authenticationRequired, this, [this](){
        emit eventOccured("Unexpected use of authentication by PHP server!");
    });
    connect(&mNam, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, [this](){
        emit eventOccured("Unexpected use of PSK authentication by PHP server!");
    });
    connect(&mNam, &QNetworkAccessManager::proxyAuthenticationRequired, this, [this](){
        emit eventOccured("Unexpected use of proxy by PHP server!");
    });
    connect(&mNam, &QNetworkAccessManager::sslErrors, this, [this](QNetworkReply* reply, const QList<QSslError>& errors){
        Q_UNUSED(reply);
        QString errStrList = Qx::String::join(errors, [](const QSslError& err){ return err.errorString(); }, ",");
        emit eventOccured("Unexpected SSL errors from PHP server! {" + errStrList + "}");
    });
}

//-Instance Functions---------------------------------------------------------------------------------------------------------
//Private:
void Mounter::finish()
{
    ErrorCode code = mErrorStatus.value();
    mErrorStatus.reset();
    mMounting = false;
    mCurrentMountInfo = {};
    mCompletedQemuCommands = 0;
    emit mountFinished(code);
}

void Mounter::createMountPoint()
{
    emit eventOccured(EVENT_CREATING_MOUNT_POINT);

    // Build commands
    QString blockDevAddCmd = "blockdev-add";
    QString deviceAddCmd = "device_add";

    QJsonObject blockDevAddArgs;
    blockDevAddArgs["node-name"] = mCurrentMountInfo.driveId;
    blockDevAddArgs["driver"] = "raw";
    blockDevAddArgs["read-only"] = true;
    QJsonObject fileArgs;
    fileArgs["driver"] = "file";
    fileArgs["filename"] = mCurrentMountInfo.filePath;
    blockDevAddArgs["file"] = fileArgs;

    QJsonObject deviceAddArgs;
    deviceAddArgs["driver"] = "virtio-blk-pci";
    deviceAddArgs["drive"] = mCurrentMountInfo.driveId;
    deviceAddArgs["id"] = mCurrentMountInfo.driveId;
    deviceAddArgs["serial"] = mCurrentMountInfo.driveSerial;

    // Log formatter
    QJsonDocument formatter;
    QString cmdLog;

    // Queue/Log commands
    formatter.setObject(blockDevAddArgs);
    cmdLog = formatter.toJson(QJsonDocument::Compact);
    mQemuMounter.execute(blockDevAddCmd, blockDevAddArgs,
                         blockDevAddCmd + " " + cmdLog);

    formatter.setObject(deviceAddArgs);
    cmdLog = formatter.toJson(QJsonDocument::Compact);
    mQemuMounter.execute(deviceAddCmd, deviceAddArgs,
                         deviceAddCmd + " " + cmdLog);

    // Await finished() signal...
}

void Mounter::setMountOnServer()
{
    emit eventOccured(EVENT_MOUNTING_THROUGH_SERVER);

    // Create mount request
    QUrl mountUrl;
    mountUrl.setScheme("http");
    mountUrl.setHost("127.0.0.1");
    mountUrl.setPort(mWebServerPort);
    mountUrl.setPath("/mount.php");

    QUrlQuery query;
    QString queryKey = "file";
    QString queryValue = QUrl::toPercentEncoding(mQemuEnabled ? mCurrentMountInfo.driveSerial :
                                                                QFileInfo(mCurrentMountInfo.filePath).fileName());
    query.addQueryItem(queryKey, queryValue);
    mountUrl.setQuery(query);

    QNetworkRequest mountReq(mountUrl);

    // GET request
    mPhpMountReply = mNam.get(mountReq);

    // Log request
    emit eventOccured(EVENT_REQUEST_SENT.arg(ENUM_NAME(mPhpMountReply->operation()), mountUrl.toString()));

    // Await finished() signal...
}

void Mounter::notePhpMountResponse(const QString& response)
{
    emit eventOccured(EVENT_PHP_RESPONSE.arg(response));
    finish();
}

//Public:
bool Mounter::isMounting() { return mMounting; }

quint16 Mounter::webServerPort() const { return mWebServerPort; }
quint16 Mounter::qemuMountPort() const { return mQemuMounter.port(); }
quint16 Mounter::qemuProdPort() const { return mQemuProdder.port(); }
bool Mounter::isQemuEnabled() const { return mQemuEnabled; }

void Mounter::setWebServerPort(quint16 port) { mWebServerPort = port; }
void Mounter::setQemuMountPort(quint16 port) { mQemuMounter.setPort(port); }
void Mounter::setQemuProdPort(quint16 port) { mQemuProdder.setPort(port); }
void Mounter::setQemuEnabled(bool enabled) { mQemuEnabled = enabled; }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void Mounter::qmpiConnectedHandler(QJsonObject version, QJsonArray capabilities)
{
    QJsonDocument formatter(version);
    QString versionStr = formatter.toJson(QJsonDocument::Compact);
    formatter.setArray(capabilities);
    QString capabilitiesStr = formatter.toJson(QJsonDocument::Compact);
    emit eventOccured(EVENT_QMP_WELCOME_MESSAGE.arg(versionStr, capabilitiesStr));
}

void Mounter::qmpiFinishedHandler()
{
    if(mErrorStatus.isSet())
        finish();
    else
        setMountOnServer();
}

void Mounter::qmpiReadyForCommandsHandler() { createMountPoint(); }

void Mounter::phpMountFinishedHandler(QNetworkReply* reply)
{
    assert(reply == mPhpMountReply.get());

    // FP (as of 11) is currently bugged and is expected to give an internal server error so it must be ignored
    if(reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::InternalServerError)
    {
        mErrorStatus = ErrorCode::PHP_MOUNT_FAIL;
        Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, reply->errorString());
        emit errorOccured(errMsg);
        finish();
    }
    else
    {
        QByteArray response = reply->readAll();
        notePhpMountResponse(QString::fromLatin1(response));
    }
}

void Mounter::qmpiConnectionErrorHandler(QAbstractSocket::SocketError error)
{
    mErrorStatus = ErrorCode::QMP_CONNECTION_FAIL;
    QString errStr = ENUM_NAME(error);
    Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, ERR_QMP_CONNECTION.arg(errStr));
    emit errorOccured(errMsg);
}

void Mounter::qmpiCommunicationErrorHandler(Qmpi::CommunicationError error)
{
    mErrorStatus = ErrorCode::QMP_COMMUNICATION_FAIL;
    QString errStr = ENUM_NAME(error);
    Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, ERR_QMP_COMMUNICATION.arg(errStr));
    emit errorOccured(errMsg);
}

void Mounter::qmpiCommandErrorHandler(QString errorClass, QString description, std::any context)
{
    mErrorStatus = ErrorCode::QMP_COMMAND_FAIL;
    QString commandErr = ERR_QMP_COMMAND.arg(std::any_cast<QString>(context), errorClass, description);
    Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, commandErr);
    emit errorOccured(errMsg);
    mQemuMounter.abort();
}

void Mounter::qmpiCommandResponseHandler(QJsonValue value, std::any context)
{
    mCompletedQemuCommands++;
    emit eventOccured(EVENT_QMP_COMMAND_RESPONSE.arg(std::any_cast<QString>(context), Qx::Json::asString(value)));

    if(mCompletedQemuCommands == 2)
        mQemuMounter.disconnectFromHost();
}

void Mounter::qmpiEventOccurredHandler(QString name, QJsonObject data, QDateTime timestamp)
{
    QJsonDocument formatter(data);
    QString dataStr = formatter.toJson(QJsonDocument::Compact);
    QString timestampStr = timestamp.toString("hh:mm:s s.zzz");
    emit eventOccured(EVENT_QMP_EVENT.arg(name, dataStr, timestampStr));
}

//Public Slots:
void Mounter::mount(QUuid titleId, QString filePath)
{
    // Update state
    mMounting = true;
    emit mountProgress(0);
    emit mountProgressMaximumChanged(0); // Cause busy state

    //-Determine mount info-------------------------------------------------

    // Set file path
    mCurrentMountInfo.filePath = filePath;

    // Generate random sequence of 16 lowercase alphabetical characters to act as Drive ID
    QByteArray alphaBytes;
    alphaBytes.resize(16);
    std::generate(alphaBytes.begin(), alphaBytes.end(), [](){
        // Funnel numbers into 0-25, use ASCI char 'a' (0x61) as a base value
        return (QRandomGenerator::global()->generate() % 26) + 0x61;
    });
    mCurrentMountInfo.driveId = QString::fromLatin1(alphaBytes);

    // Convert UUID to 20 char drive serial by encoding to Base85
    Qx::Base85Encoding encoding(Qx::Base85Encoding::Btoa);
    encoding.resetZeroGroupCharacter(); // No shortcut characters
    QByteArray rawTitleId = titleId.toRfc4122(); // Binary representation of UUID
    Qx::Base85 driveSerial = Qx::Base85::encode(rawTitleId, &encoding);
    mCurrentMountInfo.driveSerial = driveSerial.toString();

    // Connect to QEMU instance, or go straight to web server portion if bypassing
    if(mQemuMounter.port() != 0)
    {
        emit eventOccured(EVENT_CONNECTING_TO_QEMU);
        mQemuMounter.connectToHost();
        // Await readyForCommands() signal...
    }
    else
        setMountOnServer();
}

void Mounter::abort()
{
    if(mQemuMounter.isConnectionActive())
    {
        // Aborting this doesn't cause an error so we must set one here manually.
        mErrorStatus = ErrorCode::QMP_CONNECTION_FAIL;
        Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, ERR_QMP_CONNECTION.arg(ERR_QMP_CONNECTION_ABORT));
        emit errorOccured(errMsg);
        mQemuMounter.abort(); // Call last here because it causes finished signal to emit immediately
    }
    if(mPhpMountReply && mPhpMountReply->isRunning())
        mPhpMountReply->abort();
}
