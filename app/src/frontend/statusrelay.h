#ifndef STATUSRELAY_H
#define STATUSRELAY_H

// Qt Includes
#include <QObject>
#include <QProgressDialog>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QClipboard>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-bimap.h>
#include <qx/utility/qx-macros.h>
#include <qx/utility/qx-concepts.h>

// Project Includes
#include "kernel/directive.h"

class StatusRelay : public QObject
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // System Messages
    static inline const QString SYS_TRAY_STATUS = u"CLIFp is running"_s;

//-Class Variables--------------------------------------------------------------------------------------------------------
private:
    Qx::Bimap<DBlockingError::Choice, QMessageBox::StandardButton> smChoiceButtonMap{
        {DBlockingError::Choice::Ok, QMessageBox::Ok},
        {DBlockingError::Choice::Yes, QMessageBox::Yes},
        {DBlockingError::Choice::No, QMessageBox::No},
    };

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
    QMessageBox::StandardButtons choicesToButtons(DBlockingError::Choices cs);

    template<class MessageT>
        requires Qx::any_of<MessageT, DMessage, DBlockingMessage>
    QMessageBox* prepareMessageBox(const MessageT& dMsg);

//-Instance Functions--------------------------------------------------------------------------------------------------
private:
    void setupTrayIcon();
    void setupProgressDialog();

    // Async directive handlers
    void handleMessage(const DMessage& d);
    void handleError(const DError& d);
    void handleProcedureStart(const DProcedureStart& d);
    void handleProcedureStop(const DProcedureStop& d);
    void handleProcedureProgress(const DProcedureProgress& d);
    void handleProcedureScale(const DProcedureScale& d);
    void handleClipboardUpdate(const DClipboardUpdate& d);
    void handleStatusUpdate(const DStatusUpdate& d);

    // Sync directive handlers
    void handleBlockingMessage(const DBlockingMessage& d);
    void handleBlockingError(const DBlockingError& d);
    void handleSaveFilename(const DSaveFilename& d);
    void handleExistingDir(const DExistingDir& d);
    void handleItemSelection(const DItemSelection& d);
    void handleYesOrNo(const DYesOrNo& d);

//-Signals & Slots------------------------------------------------------------------------------------------------------
public slots:
    // Directive handlers
    void asyncDirectiveHandler(const AsyncDirective& aDirective);
    void syncDirectiveHandler(const SyncDirective& sDirective);

signals:
    void longTaskCanceled();
    void quitRequested();
};

#endif // STATUSRELAY_H
