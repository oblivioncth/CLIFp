#ifndef MOUNTER_H
#define MOUNTER_H

// Qt Includes
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

// Qx Includes
#include <qx/core/qx-setonce.h>
#include <qx/core/qx-genericerror.h>
#include <qx/utility/qx-macros.h>

// QI-QMP Includes
#include <qi-qmp/qmpi.h>

// Project Includes
#include "kernel/errorcode.h"

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
    // Error
    static inline const QString MOUNT_ERROR_TEXT = QSL("An error occurred while mounting a data pack.");
    static inline const QString ERR_QMP_CONNECTION = QSL("QMPI connection error - \"%1\"");
    static inline const QString ERR_QMP_CONNECTION_ABORT = QSL("The connection was aborted.");
    static inline const QString ERR_QMP_COMMUNICATION = QSL("QMPI communication error - \"%1\"");
    static inline const QString ERR_QMP_COMMAND = QSL("QMPI command %1 error - [%2] \"%3\"");

    // Events - External
    static inline const QString EVENT_QMP_WELCOME_MESSAGE = QSL("QMPI connected to QEMU Version: %1 | Capabilities: %2");
    static inline const QString EVENT_QMP_COMMAND_RESPONSE = QSL("QMPI command %1 returned - \"%2\"");
    static inline const QString EVENT_QMP_EVENT = QSL("QMPI event occurred at %1 - [%2] \"%3\"");
    static inline const QString EVENT_PHP_RESPONSE = QSL("Mount.php Response: \"%1\"");

    // Events - Internal
    static inline const QString EVENT_CONNECTING_TO_QEMU = QSL("Connecting to FP QEMU instance...");
    static inline const QString EVENT_CREATING_MOUNT_POINT = QSL("Creating data pack mount point on QEMU instance...");
    static inline const QString EVENT_MOUNTING_THROUGH_SERVER = QSL("Mounting data pack via PHP server...");
    static inline const QString EVENT_REQUEST_SENT = QSL("Sent request (%1): %2}");

    // Connections
    static const int QMP_TRANSACTION_TIMEOUT = 5000; // ms
    static const int PHP_TRANSFER_TIMEOUT = 30000; // ms

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mMounting;
    Qx::SetOnce<ErrorCode> mErrorStatus;
    MountInfo mCurrentMountInfo;

    int mWebServerPort;
    Qmpi mQemuMounter;
    Qmpi mQemuProdder; // Not actually used; no, need unless issues with mounting are reported
    bool mQemuEnabled;
    int mCompletedQemuCommands;

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
    void errorOccured(Qx::GenericError errorMessage);
    void mountFinished(ErrorCode errorCode);

    // For now these just cause a busy state
    void mountProgress(qint64 progress);
    void mountProgressMaximumChanged(qint64 maximum);
};

#endif // MOUNTER_H
