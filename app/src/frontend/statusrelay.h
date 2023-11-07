#ifndef STATUSRELAY_H
#define STATUSRELAY_H

// Qt Includes
#include <QObject>
#include <QProgressDialog>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAuthenticator>
#include <QInputDialog>
#include <QClipboard>

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
    QClipboard* mSystemClipboard;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    explicit StatusRelay(QObject* parent = nullptr);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    void setupTrayIcon();
    void setupProgressDialog();

//-Signals & Slots------------------------------------------------------------------------------------------------------
public slots:
    // Request/status handlers
    void statusChangeHandler(const QString& statusHeading, const QString& statusMessage);
    void errorHandler(Core::Error error);
    void blockingErrorHandler(QSharedPointer<int> response, Core::BlockingError blockingError);
    void messageHandler(const Message& message);
    void saveFileRequestHandler(QSharedPointer<QString> file, Core::SaveFileRequest request);
    void existingDirectoryRequestHandler(QSharedPointer<QString> dir, Core::ExistingDirRequest request);
    void itemSelectionRequestHandler(QSharedPointer<QString> item, const Core::ItemSelectionRequest& request);
    void clipboardUpdateRequestHandler(const QString& text);
    void questionAnswerRequestHandler(QSharedPointer<bool> response, const QString& question);

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
