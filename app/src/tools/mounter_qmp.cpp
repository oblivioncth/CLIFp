// Unit Includes
#include "mounter_qmp.h"

// Qx Includes
#include <qx/core/qx-json.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// MounterQmpError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
MounterQmpError::MounterQmpError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool MounterQmpError::isValid() const { return mType != NoError; }
QString MounterQmpError::specific() const { return mSpecific; }
MounterQmpError::Type MounterQmpError::type() const { return mType; }

//Private:
Qx::Severity MounterQmpError::deriveSeverity() const { return Qx::Critical; }
quint32 MounterQmpError::deriveValue() const { return mType; }
QString MounterQmpError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString MounterQmpError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// MounterQmp
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
MounterQmp::MounterQmp(QObject* parent, Director* director) :
    QObject(parent),
    Directorate(director),
    mMounting(false),
    mErrorStatus(MounterQmpError::NoError),
    mQemuMounter(QHostAddress::LocalHost, 0),
    mQemuProdder(QHostAddress::LocalHost, 0) // Currently not used
{
    // Setup QMPI
    mQemuMounter.setTransactionTimeout(QMP_TRANSACTION_TIMEOUT);

    // Connections - Work
    connect(&mQemuMounter, &Qmpi::readyForCommands, this, &MounterQmp::qmpiReadyForCommandsHandler);
    connect(&mQemuMounter, &Qmpi::commandQueueExhausted, this, &MounterQmp::qmpiCommandsExhaustedHandler);
    connect(&mQemuMounter, &Qmpi::finished, this, &MounterQmp::qmpiFinishedHandler);

    // Connections - Error
    connect(&mQemuMounter, &Qmpi::connectionErrorOccurred, this, &MounterQmp::qmpiConnectionErrorHandler);
    connect(&mQemuMounter, &Qmpi::communicationErrorOccurred, this, &MounterQmp::qmpiCommunicationErrorHandler);
    connect(&mQemuMounter, &Qmpi::errorResponseReceived, this, &MounterQmp::qmpiCommandErrorHandler);

    // Connections - Log
    connect(&mQemuMounter, &Qmpi::connected, this, &MounterQmp::qmpiConnectedHandler);
    connect(&mQemuMounter, &Qmpi::responseReceived, this, &MounterQmp::qmpiCommandResponseHandler);
    connect(&mQemuMounter, &Qmpi::eventReceived, this, &MounterQmp::qmpiEventOccurredHandler);
}

//-Instance Functions---------------------------------------------------------------------------------------------------------
//Private:
void MounterQmp::finish()
{
    MounterQmpError err = mErrorStatus.value();
    mErrorStatus.reset();
    mMounting = false;
    emit mountFinished(err);
}

void MounterQmp::createMountPoint()
{
    logEvent(EVENT_CREATING_MOUNT_POINT);

    // Build commands
    QString blockDevAddCmd = u"blockdev-add"_s;
    QString deviceAddCmd = u"device_add"_s;

    QJsonObject blockDevAddArgs;
    blockDevAddArgs[u"node-name"_s] = mDriveId;
    blockDevAddArgs[u"driver"_s] = u"raw"_s;
    blockDevAddArgs[u"read-only"_s] = true;
    QJsonObject fileArgs;
    fileArgs[u"driver"_s] = u"file"_s;
    fileArgs[u"filename"_s] = mFilePath;
    blockDevAddArgs[u"file"_s] = fileArgs;

    QJsonObject deviceAddArgs;
    deviceAddArgs[u"driver"_s] = u"virtio-blk-pci"_s;
    deviceAddArgs[u"drive"_s] = mDriveId;
    deviceAddArgs[u"id"_s] = mDriveId;
    deviceAddArgs[u"serial"_s] = mDriveSerial;

    // Log formatter
    QJsonDocument formatter;
    QString cmdLog;

    // Queue/Log commands
    formatter.setObject(blockDevAddArgs);
    cmdLog = formatter.toJson(QJsonDocument::Compact);
    mQemuMounter.execute(blockDevAddCmd, blockDevAddArgs,
                         blockDevAddCmd + ' ' + cmdLog);

    formatter.setObject(deviceAddArgs);
    cmdLog = formatter.toJson(QJsonDocument::Compact);
    mQemuMounter.execute(deviceAddCmd, deviceAddArgs,
                         deviceAddCmd + ' ' + cmdLog);

    // Await finished() signal...
}

//Public:
QString MounterQmp::name() const { return NAME; }
bool MounterQmp::isMounting() { return mMounting; }

QString MounterQmp::driveId() const { return mDriveId; }
QString MounterQmp::driveSerial() const { return mDriveSerial; }
QString MounterQmp::filePath() const { return mFilePath; }
quint16 MounterQmp::qemuMountPort() const { return mQemuMounter.port(); }
quint16 MounterQmp::qemuProdPort() const { return mQemuProdder.port(); }

void MounterQmp::setDriveId(const QString& id) { mDriveId = id; }
void MounterQmp::setDriveSerial(const QString& serial){ mDriveSerial = serial; }
void MounterQmp::setFilePath(const QString& path) { mFilePath = path; }
void MounterQmp::setQemuMountPort(quint16 port) { mQemuMounter.setPort(port); }
void MounterQmp::setQemuProdPort(quint16 port) { mQemuProdder.setPort(port); }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void MounterQmp::qmpiConnectedHandler(QJsonObject version, QJsonArray capabilities)
{
    QJsonDocument formatter(version);
    QString versionStr = formatter.toJson(QJsonDocument::Compact);
    formatter.setArray(capabilities);
    QString capabilitiesStr = formatter.toJson(QJsonDocument::Compact);
    logEvent(EVENT_QMP_WELCOME_MESSAGE.arg(versionStr, capabilitiesStr));
}

void MounterQmp::qmpiCommandsExhaustedHandler()
{
    logEvent(EVENT_DISCONNECTING_FROM_QEMU);
    mQemuMounter.disconnectFromHost();
}

void MounterQmp::qmpiFinishedHandler()
{
    finish();
}

void MounterQmp::qmpiReadyForCommandsHandler() { createMountPoint(); }

void MounterQmp::qmpiConnectionErrorHandler(QAbstractSocket::SocketError error)
{
    MounterQmpError err(MounterQmpError::QemuConnection, ENUM_NAME(error));
    mErrorStatus = err;

    postDirective<DError>(err);
}

void MounterQmp::qmpiCommunicationErrorHandler(Qmpi::CommunicationError error)
{
    MounterQmpError err(MounterQmpError::QemuCommunication, ENUM_NAME(error));
    mErrorStatus = err;

    postDirective<DError>(err);
}

void MounterQmp::qmpiCommandErrorHandler(QString errorClass, QString description, std::any context)
{
    QString commandErr = ERR_QMP_COMMAND.arg(std::any_cast<QString>(context), errorClass, description);

    MounterQmpError err(MounterQmpError::QemuCommand, commandErr);
    mErrorStatus = err;

    postDirective<DError>(err);
    mQemuMounter.abort();
}

void MounterQmp::qmpiCommandResponseHandler(QJsonValue value, std::any context)
{
    logEvent(EVENT_QMP_COMMAND_RESPONSE.arg(std::any_cast<QString>(context), Qx::asString(value)));
}

void MounterQmp::qmpiEventOccurredHandler(QString name, QJsonObject data, QDateTime timestamp)
{
    QJsonDocument formatter(data);
    QString dataStr = formatter.toJson(QJsonDocument::Compact);
    QString timestampStr = timestamp.toString(u"hh:mm:s s.zzz"_s);
    logEvent(EVENT_QMP_EVENT.arg(name, dataStr, timestampStr));
}

//Public Slots:
void MounterQmp::mount()
{
    // Connect to QEMU instance
    logEvent(EVENT_CONNECTING_TO_QEMU);
    mQemuMounter.connectToHost();

    // Await readyForCommands() signal...
}

void MounterQmp::abort()
{
    if(mQemuMounter.isConnectionActive())
    {
        // Aborting this doesn't cause an error so we must set one here manually.
        MounterQmpError err(MounterQmpError::QemuConnection, ERR_QMP_CONNECTION_ABORT);
        mErrorStatus = err;

        postDirective<DError>(err);
        mQemuMounter.abort(); // Call last here because it causes finished signal to emit immediately
    }
}
