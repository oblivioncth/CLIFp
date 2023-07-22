#ifndef STATUSRELAY_H
#define STATUSRELAY_H

// Qt Includes
#include <QObject>
#include <QProgressDialog>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAuthenticator>
#include <QInputDialog>

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "kernel/core.h"

class StatusRelay : public QObject
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // System Messages
    static inline const QString SYS_TRAY_STATUS = u"CLIFp is running"_s;

//-Instance Variables------------------------------------------------------------------------------------------------------
public:
    QProgressDialog mLongTaskProgressDialog;
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
    void setupProgressDialog();

//-Signals & Slots------------------------------------------------------------------------------------------------------
public slots:
    // Status messages
    void statusChangeHandler(const QString& statusHeading, const QString& statusMessage);
    void errorHandler(Core::Error error);
    void blockingErrorHandler(QSharedPointer<int> response, Core::BlockingError blockingError);
    void messageHandler(const QString& message);
    void saveFileRequestHandler(QSharedPointer<QString> file, Core::SaveFileRequest request);
    void itemSelectionRequestHandler(QSharedPointer<QString> item, const Core::ItemSelectionRequest& request);

    // Long Job
    void longTaskProgressHandler(quint64 progress);
    void longTaskTotalHandler(quint64 total);
    void longTaskStartedHandler(QString task);
    void longTaskFinishedHandler();

signals:
    void longTaskCanceled();
    void quitRequested();
};

#endif // STATUSRELAY_H
