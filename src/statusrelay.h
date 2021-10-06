#ifndef STATUSRELAY_H
#define STATUSRELAY_H

#include <QObject>
#include <QProgressDialog>
#include <QSystemTrayIcon>
#include "core.h"

class StatusRelay : public QObject
{
    Q_OBJECT
//-Instance Variables------------------------------------------------------------------------------------------------------
public:
    // System Messages
    static inline const QString SYS_TRAY_STATUS = "CLIFp is running";

//-Instance Variables------------------------------------------------------------------------------------------------------
public:
    QProgressDialog* mDownloadProgressDialog;
    QString mStatusHeading;
    QString mStatusMessage;

    QSystemTrayIcon mTrayIcon;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    explicit StatusRelay(QObject* parent = nullptr);

//-Signals & Slots------------------------------------------------------------------------------------------------------
public slots:
    // Status messages
    void statusChangeHandler(const QString& statusHeading, const QString& statusMessage);
    void errorHandler(Core::Error error);
    void blockingErrorHandler(QSharedPointer<int> response, Core::BlockingError blockingError);
    void messageHandler(const QString& message);

    // Network
    void authenticationHandler(QString prompt, QString* username, QString* password, bool* abort);
    void downloadProgressHandler(quint64 progress);
    void downloadTotalHandler(quint64 total);
    void downloadStartedHandler(QString task);
    void downloadFinishedHandler(bool canceled);

signals:
    void downloadCanceled();
};

#endif // STATUSRELAY_H
