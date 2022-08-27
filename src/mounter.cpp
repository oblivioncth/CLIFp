// Unit Includes
#include "mounter.h"

// Qx Includes
#include <qx/core/qx-json.h>
#include <qx/core/qx-base85.h>

//===============================================================================================================
// Mounter
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
Mounter::Mounter(quint16 qemuMountPort, quint16 qemuProdPort, quint16 webserverPort, QObject* parent) :
    QObject(parent),
    mMounting(false),
    mErrorStatus(Core::ErrorCodes::NO_ERR),
    mQemuMounter(QHostAddress::LocalHost, qemuMountPort, this),
    mQemuProdder(QHostAddress::LocalHost, qemuProdPort, this),
    mCompletedQemuCommands(0)
{
    // Setup Network Access Manager
    mNam.setAutoDeleteReplies(true);
    mNam.setTransferTimeout(10);

    // Setup QMPI
    mQemuMounter.setTransactionTimeout(5000);

    // Connections - Work
    connect(&mQemuMounter, &Qmpi::readyForCommands, this, &Mounter::qmpiReadyForCommandsHandler);
    connect(&mQemuMounter, &Qmpi::finished, this, &Mounter::qmpiFinishedHandler);
    connect(&mNam, &QNetworkAccessManager::finished, this, &Mounter::phpMountFinishedHandler);

    // Connections - Error
    connect(&mQemuMounter, &Qmpi::connectionErrorOccured, this, &Mounter::qmpiConnectionErrorHandler);
    connect(&mQemuMounter, &Qmpi::communicationErrorOccured, this, &Mounter::qmpiCommunicationErrorHandler);
    connect(&mQemuMounter, &Qmpi::errorResponseReceived, this, &Mounter::qmpiCommandErrorHandler);
    connect(&mQemuMounter, &Qmpi::errorResponseReceived, this, &Mounter::qmpiCommandErrorHandler);

    // Connections - Log
    connect(&mQemuMounter, &Qmpi::responseReceived, this, &Mounter::qmpiCommandResponseHandler);
    connect(&mQemuMounter, &Qmpi::eventReceived, this, &Mounter::qmpiEventOccurredHandler);
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
    // Create mount point in QEMU instance
    mQemuMounter.execute("blockdev-add", {
        {"node-name", mCurrentMountInfo.driveId},
        {"driver", "raw"},
        {"file", QJsonObject{
            {"driver", "file"},
            {"filename", mCurrentMountInfo.filePath}
        }}
    },
    QString("blockdev-add"));

    mQemuMounter.execute("device_add", {
        {"driver", "virtio-blk-pci"},
        {"drive", mCurrentMountInfo.driveId},
        {"id", mCurrentMountInfo.driveId},
        {"serial", mCurrentMountInfo.driveSerial}
    },
    QString("device_add"));

    // Await finished() signal...
}

void Mounter::setMountOnServer()
{
    // Create mount request
    QUrl mountUrl;
    mountUrl.setScheme("http");
    mountUrl.setHost("127.0.0.1");
    mountUrl.setPort(22500);
    mountUrl.setPath("/mount.php");
    mountUrl.setQuery({{"file", QUrl::toPercentEncoding(mCurrentMountInfo.driveSerial)}});
    QNetworkRequest mountReq(mountUrl);

    // GET request
    mPhpMountReply = mNam.get(mountReq);

    // Await finished() signal...
}

void Mounter::notePhpMountResponse(const QString& response)
{
    emit eventOccured(PHP_RESPONSE.arg(response));
    finish();
}

//Public:
bool Mounter::isMounting() { return mMounting; }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void Mounter::qmpiFinishedHandler()
{
    if(mErrorStatus.isSet())
        finish();
    else
        setMountOnServer();
}

void Mounter::qmpiReadyForCommandsHandler() { createMountPoint(); }

void Mounter::phpMountFinishedHandler(QNetworkReply *reply)
{
    assert(reply == mPhpMountReply.get());

    if(reply->error() != QNetworkReply::NoError)
    {
        mErrorStatus = Core::ErrorCodes::PHP_MOUNT_FAIL;
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
    mErrorStatus = Core::ErrorCodes::QMP_CONNECTION_FAIL;
    QString errStr = ENUM_NAME(error);
    Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, ERR_QMP_CONNECTION.arg(errStr));
    emit errorOccured(errMsg);
}

void Mounter::qmpiCommunicationErrorHandler(Qmpi::CommunicationError error)
{
    mErrorStatus = Core::ErrorCodes::QMP_COMMUNICATION_FAIL;
    QString errStr = ENUM_NAME(error);
    Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, ERR_QMP_COMMUNICATION.arg(errStr));
    emit errorOccured(errMsg);
}

void Mounter::qmpiCommandErrorHandler(QString errorClass, QString description, std::any context)
{
    mErrorStatus = Core::ErrorCodes::QMP_COMMAND_FAIL;
    QString commandErr = ERR_QMP_COMMAND.arg(std::any_cast<QString>(context), errorClass, description);
    Qx::GenericError errMsg(Qx::GenericError::Critical, MOUNT_ERROR_TEXT, commandErr);
    emit errorOccured(errMsg);
    mQemuMounter.disconnectFromHost();
}

void Mounter::qmpiCommandResponseHandler(QJsonValue value, std::any context)
{
    mCompletedQemuCommands++;
    emit eventOccured(COMMAND_RESPONSE.arg(std::any_cast<QString>(context), Qx::Json::asString(value)));

    if(mCompletedQemuCommands == 2)
        mQemuMounter.disconnectFromHost();
}

void Mounter::qmpiEventOccurredHandler(QString name, QJsonObject data, QDateTime timestamp)
{
    QJsonDocument formatter(data);
    QString dataStr = formatter.toJson(QJsonDocument::Compact);
    QString timestampStr = timestamp.toString("hh:mm:ss.zzz");
    emit eventOccured(EVENT_OCCURRED.arg(name, dataStr, timestampStr));
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

    // Connect to QEMU instance
    mQemuMounter.connectToHost();

    // Await readyForCommands() signal...
}

void Mounter::abort()
{
    if(mQemuMounter.isConnectionActive())
        mQemuMounter.disconnectFromHost();
    if(mPhpMountReply && mPhpMountReply->isRunning())
        mPhpMountReply->abort();
}
