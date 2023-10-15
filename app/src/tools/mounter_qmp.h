#ifndef MOUNTER_QMP_H
#define MOUNTER_QMP_H

// Qt Includes
#include <QObject>

// Qx Includes
#include <qx/core/qx-error.h>
#include <qx/utility/qx-macros.h>
#include <qx/core/qx-setonce.h>

// QI-QMP Includes
#include <qi-qmp/qmpi.h>

class QX_ERROR_TYPE(MounterQmpError, "MounterQmpError", 1233)
{
    friend class MounterQmp;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        QemuConnection,
        QemuCommunication,
        QemuCommand
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {QemuConnection, u"QMPI connection error."_s},
        {QemuCommunication, u"QMPI communication error."_s},
        {QemuCommand, u"QMPI command error."_s},
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    MounterQmpError(Type t = NoError, const QString& s = {});

    //-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class MounterQmp : public QObject
{
    Q_OBJECT

//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"Mounter"_s;

    // Error Status Helper
    static inline const auto ERROR_STATUS_CMP = [](const MounterQmpError& a, const MounterQmpError& b){
        return a.type() == b.type();
    };

    // Error
    static inline const QString ERR_QMP_CONNECTION_ABORT = u"The connection was aborted."_s;
    static inline const QString ERR_QMP_COMMAND = u"Command %1 - [%2] \"%3\""_s;

    // Events - External
    static inline const QString EVENT_QMP_WELCOME_MESSAGE = u"QMPI connected to QEMU Version: %1 | Capabilities: %2"_s;
    static inline const QString EVENT_QMP_COMMAND_RESPONSE = u"QMPI command %1 returned - \"%2\""_s;
    static inline const QString EVENT_QMP_EVENT = u"QMPI event occurred at %1 - [%2] \"%3\""_s;

    // Events - Internal
    static inline const QString EVENT_CONNECTING_TO_QEMU = u"Connecting to FP QEMU instance..."_s;
    static inline const QString EVENT_MOUNT_INFO_DETERMINED = u"Mount Info: {.filePath = \"%1\", .driveId = \"%2\", .driveSerial = \"%3\"}"_s;
    static inline const QString EVENT_QEMU_DETECTION = u"QEMU %1 in use."_s;
    static inline const QString EVENT_CREATING_MOUNT_POINT = u"Creating data pack mount point on QEMU instance..."_s;
    static inline const QString EVENT_DISCONNECTING_FROM_QEMU = u"Disconnecting from FP QEMU instance..."_s;

    // Connections
    static const int QMP_TRANSACTION_TIMEOUT = 5000; // ms

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mMounting;
    Qx::SetOnce<MounterQmpError, decltype(ERROR_STATUS_CMP)> mErrorStatus;

    QString mDriveId;
    QString mDriveSerial;
    QString mFilePath;

    Qmpi mQemuMounter;
    Qmpi mQemuProdder; // Not actually used; no, need unless issues with mounting are reported

//-Constructor-------------------------------------------------------------------------------------------------
public:
    explicit MounterQmp(QObject* parent = nullptr);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    void finish();
    void createMountPoint();

public:
    bool isMounting();

    QString driveId() const;
    QString driveSerial() const;
    QString filePath() const;
    quint16 qemuMountPort() const;
    quint16 qemuProdPort() const;

    void setDriveId(const QString& id);
    void setDriveSerial(const QString& serial);
    void setFilePath(const QString& path);
    void setQemuMountPort(quint16 port);
    void setQemuProdPort(quint16 port);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void qmpiConnectedHandler(QJsonObject version, QJsonArray capabilities);
    void qmpiCommandsExhaustedHandler();
    void qmpiFinishedHandler();
    void qmpiReadyForCommandsHandler();

    void qmpiConnectionErrorHandler(QAbstractSocket::SocketError error);
    void qmpiCommunicationErrorHandler(Qmpi::CommunicationError error);
    void qmpiCommandErrorHandler(QString errorClass, QString description, std::any context);
    void qmpiCommandResponseHandler(QJsonValue value, std::any context);
    void qmpiEventOccurredHandler(QString name, QJsonObject data, QDateTime timestamp);

public slots:
    void mount();
    void abort();

signals:
    void eventOccurred(QString name, QString event);
    void errorOccurred(QString name, MounterQmpError errorMessage);
    void mountFinished(MounterQmpError errorState);
};

#endif // MOUNTER_QMP_H
