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
    static inline const QString MOUNT_ERROR_TEXT = "An error occurred while mounting a data pack.";
    static inline const QString ERR_QMP_CONNECTION = "QMPI connection error - \"%1\"";
    static inline const QString ERR_QMP_CONNECTION_ABORT = "The connection was aborted.";
    static inline const QString ERR_QMP_COMMUNICATION = "QMPI communication error - \"%1\"";
    static inline const QString ERR_QMP_COMMAND = "QMPI command %1 error - [%2] \"%3\"";

    // Events - External
    static inline const QString EVENT_QMP_WELCOME_MESSAGE = "QMPI connected to QEMU Version: %1 | Capabilities: %2";
    static inline const QString EVENT_QMP_COMMAND_RESPONSE = "QMPI command %1 returned - \"%2\"";
    static inline const QString EVENT_QMP_EVENT = "QMPI event occurred at %1 - [%2] \"%3\"";
    static inline const QString EVENT_PHP_RESPONSE = "Mount.php Response: \"%1\"";

    // Events - Internal
    static inline const QString EVENT_CONNECTING_TO_QEMU = "Connecting to FP QEMU instance...";
    static inline const QString EVENT_CREATING_MOUNT_POINT = "Creating data pack mount point on QEMU instance...";
    static inline const QString EVENT_MOUNTING_THROUGH_SERVER = "Mounting data pack via PHP server...";

    // Connections
    static const int QMP_TRANSACTION_TIMEOUT = 5000; // ms
    static const int PHP_TRANSFER_TIMEOUT = 30000; // ms

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mMounting;
    Qx::SetOnce<ErrorCode> mErrorStatus;
    MountInfo mCurrentMountInfo;

    int mWebserverPort;
    Qmpi mQemuMounter;
    Qmpi mQemuProdder; // Not used until switching to FP 11
    int mCompletedQemuCommands;

    QNetworkAccessManager mNam;
    QPointer<QNetworkReply> mPhpMountReply;
    QString mPhpMountReplyResponse;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    explicit Mounter(quint16 webserverPort, quint16 qemuMountPort = 0, quint16 qemuProdPort  = 0, QObject* parent = nullptr);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    void finish();
    void createMountPoint();
    void setMountOnServer();
    void notePhpMountResponse(const QString& response);

public:
    bool isMounting();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void qmpiConnectedHandler(QJsonObject version, QJsonArray capabilities);
    void qmpiFinishedHandler();
    void qmpiReadyForCommandsHandler();
    void phpMountFinishedHandler(QNetworkReply *reply);

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
