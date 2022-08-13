#ifndef STATUSRELAY_H
#define STATUSRELAY_H

// Qt Includes
#include <QObject>
#include <QProgressDialog>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAuthenticator>

// Project Includes
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
    QMenu mTrayIconContextMenu;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    explicit StatusRelay(QObject* parent = nullptr);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    void setupTrayIcon();

//-Signals & Slots------------------------------------------------------------------------------------------------------
public slots:
    // Status messages
    void statusChangeHandler(const QString& statusHeading, const QString& statusMessage);
    void errorHandler(Core::Error error);
    void blockingErrorHandler(QSharedPointer<int> response, Core::BlockingError blockingError);
    void messageHandler(const QString& message);

    // Network
    void authenticationHandler(QString prompt, QAuthenticator* authenticator);
    void downloadProgressHandler(quint64 progress);
    void downloadTotalHandler(quint64 total);
    void downloadStartedHandler(QString task);
    void downloadFinishedHandler(bool canceled);

signals:
    void downloadCanceled();
    void quitRequested();
};

#endif // STATUSRELAY_H
