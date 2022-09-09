// Unit Include
#include "statusrelay.h"

// Qt Includes
#include <QApplication>
#include <QScopedValueRollback>

// Qx Includes
#include <qx/widgets/qx-common-widgets.h>

//===============================================================================================================
// STATUS RELAY
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
StatusRelay::StatusRelay(QObject* parent) :
    QObject(parent)
{
    setupTrayIcon();
    setupProgressDialog();
}

//-Instance Functions--------------------------------------------------------------------------------------------------
//Private:
void StatusRelay::setupTrayIcon()
{
    // Set Icon
    mTrayIcon.setIcon(QIcon(":/app/CLIFp.ico"));

    // Set ToolTip Action
    mTrayIcon.setToolTip(SYS_TRAY_STATUS);
    connect(&mTrayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if(reason != QSystemTrayIcon::Context)
            mTrayIcon.showMessage(mStatusHeading, mStatusMessage);
    });

    // Set Context Menu
    QAction* quit = new QAction(&mTrayIconContextMenu);
    quit->setIcon(QIcon(":/tray/Exit.png"));
    quit->setText("Quit");
    connect(quit, &QAction::triggered, this, &StatusRelay::quitRequested);
    mTrayIconContextMenu.addAction(quit);
    mTrayIcon.setContextMenu(&mTrayIconContextMenu);

    // Display Icon
    mTrayIcon.show();
}

void StatusRelay::setupProgressDialog()
{
    // Initialize dialog
    mLongTaskProgressDialog.setCancelButtonText("Cancel");
    mLongTaskProgressDialog.setMinimum(0);
    mLongTaskProgressDialog.setMaximum(0);
    mLongTaskProgressDialog.setWindowModality(Qt::NonModal);
    mLongTaskProgressDialog.setMinimumDuration(0);
    mLongTaskProgressDialog.setAutoClose(true);
    mLongTaskProgressDialog.setAutoReset(false);
    connect(&mLongTaskProgressDialog, &QProgressDialog::canceled, this, &StatusRelay::longTaskCanceled);
}

//-Signals & Slots-------------------------------------------------------------
//Public Slots:
void StatusRelay::statusChangeHandler(const QString& statusHeading, const QString& statusMessage)
{
    mStatusHeading = statusHeading;
    mStatusMessage = statusMessage;
}

void StatusRelay::errorHandler(Core::Error error)
{
    Qx::postError(error.errorInfo);
}

void StatusRelay::blockingErrorHandler(QSharedPointer<int> response, Core::BlockingError blockingError)
{
    *response = Qx::postBlockingError(blockingError.errorInfo, blockingError.choices, blockingError.defaultChoice);
}

void StatusRelay::messageHandler(const QString& message)
{
    QMessageBox* msg  = new QMessageBox();
    msg->setIcon(QMessageBox::Information);
    msg->setWindowTitle(QApplication::applicationName()); // This should be the default, but hey being explicit never hurt
    msg->setText(message);
    msg->setAttribute(Qt::WA_DeleteOnClose);
    msg->show();
}

void StatusRelay::longTaskProgressHandler(quint64 progress)
{
    mLongTaskProgressDialog.setValue(progress);
}

void StatusRelay::longTaskTotalHandler(quint64 total)
{
    mLongTaskProgressDialog.setMaximum(total);
}

void StatusRelay::longTaskStartedHandler(QString task)
{
    // Set label
    mLongTaskProgressDialog.setLabelText(task);

    // Show right away
    mLongTaskProgressDialog.setValue(0);
}

void StatusRelay::longTaskFinishedHandler()
{
    if(mLongTaskProgressDialog.isVisible()) // Is already closed if canceled
        mLongTaskProgressDialog.reset();
}
