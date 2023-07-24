#ifndef MOUNTER_H
#define MOUNTER_H

// Qt Includes
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

// Qx Includes
#include <qx/core/qx-setonce.h>
#include <qx/core/qx-error.h>
#include <qx/utility/qx-macros.h>

// QI-QMP Includes
#include <qi-qmp/qmpi.h>

class QX_ERROR_TYPE(MounterError, "MounterError", 1232)
{
    friend class Mounter;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        PhpMount = 1,
        QemuConnection = 2,
        QemuCommunication = 3,
        QemuCommand = 4
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {PhpMount, u"Failed to mount data pack (PHP)."_s},
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
    MounterError(Type t = NoError, const QString& s = {});

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

class Mounter : public QObject
{
    Q_OBJECT
//-Class Structs
private:
    struct MountInfo
    {
        QString filePath;
        QString driveId;
        QString driveSerial;
    };

//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Error Status Helper
    static inline const auto ERROR_STATUS_CMP = [](const MounterError& a, const MounterError& b){
        return a.type() == b.type();
    };

    // Error
    static inline const QString ERR_QMP_CONNECTION_ABORT = u"The connection was aborted."_s;
    static inline const QString ERR_QMP_COMMAND = u"Command %1 - [%2] \"%3\""_s;

    // Events - External
    static inline const QString EVENT_QMP_WELCOME_MESSAGE = u"QMPI connected to QEMU Version: %1 | Capabilities: %2"_s;
    static inline const QString EVENT_QMP_COMMAND_RESPONSE = u"QMPI command %1 returned - \"%2\""_s;
    static inline const QString EVENT_QMP_EVENT = u"QMPI event occurred at %1 - [%2] \"%3\""_s;
    static inline const QString EVENT_PHP_RESPONSE = u"Mount.php Response: \"%1\""_s;

    // Events - Internal
    static inline const QString EVENT_CONNECTING_TO_QEMU = u"Connecting to FP QEMU instance..."_s;
    static inline const QString EVENT_MOUNT_INFO_DETERMINED = u"Mount Info: {.filePath = \"%1\", .driveId = \"%2\", .driveSerial = \"%3\"}"_s;
    static inline const QString EVENT_QEMU_DETECTION = u"QEMU %1 in use."_s;
    static inline const QString EVENT_CREATING_MOUNT_POINT = u"Creating data pack mount point on QEMU instance..."_s;
    static inline const QString EVENT_DISCONNECTING_FROM_QEMU = u"Disconnecting from FP QEMU instance..."_s;
    static inline const QString EVENT_MOUNTING_THROUGH_SERVER = u"Mounting data pack via PHP server..."_s;
    static inline const QString EVENT_REQUEST_SENT = u"Sent request (%1): %2}"_s;

    // Connections
    static const int QMP_TRANSACTION_TIMEOUT = 5000; // ms
    static const int PHP_TRANSFER_TIMEOUT = 30000; // ms

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mMounting;
    Qx::SetOnce<MounterError, decltype(ERROR_STATUS_CMP)> mErrorStatus;
    MountInfo mCurrentMountInfo;

    int mWebServerPort;
    Qmpi mQemuMounter;
    Qmpi mQemuProdder; // Not actually used; no, need unless issues with mounting are reported
    bool mQemuEnabled;

    QNetworkAccessManager mNam;
    QPointer<QNetworkReply> mPhpMountReply;
    QString mPhpMountReplyResponse;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    explicit Mounter(QObject* parent = nullptr);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    void finish();
    void createMountPoint();
    void setMountOnServer();
    void notePhpMountResponse(const QString& response);
    void logMountInfo(const MountInfo& info);

public:
    bool isMounting();

    quint16 webServerPort() const;
    quint16 qemuMountPort() const;
    quint16 qemuProdPort() const;
    bool isQemuEnabled() const;

    void setWebServerPort(quint16 port);
    void setQemuMountPort(quint16 port);
    void setQemuProdPort(quint16 port);
    void setQemuEnabled(bool enabled);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void qmpiConnectedHandler(QJsonObject version, QJsonArray capabilities);
    void qmpiCommandsExhaustedHandler();
    void qmpiFinishedHandler();
    void qmpiReadyForCommandsHandler();
    void phpMountFinishedHandler(QNetworkReply* reply);

    void qmpiConnectionErrorHandler(QAbstractSocket::SocketError error);
    void qmpiCommunicationErrorHandler(Qmpi::CommunicationError error);
    void qmpiCommandErrorHandler(QString errorClass, QString description, std::any context);
    void qmpiCommandResponseHandler(QJsonValue value, std::any context);
    void qmpiEventOccurredHandler(QString name, QJsonObject data, QDateTime timestamp);

public slots:
    void mount(QUuid titleId, QString filePath);
    void abort();

signals:
    void eventOccured(QString event);
    void errorOccured(MounterError errorMessage);
    void mountFinished(MounterError errorState);

    // For now these just cause a busy state
    void mountProgress(qint64 progress);
    void mountProgressMaximumChanged(qint64 maximum);
};

#endif // MOUNTER_H
