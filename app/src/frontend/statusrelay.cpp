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
    mLongTaskProgressDialog.setWindowModality(Qt::NonModal);
    mLongTaskProgressDialog.setMinimumDuration(0);
    mLongTaskProgressDialog.setAutoClose(true);
    mLongTaskProgressDialog.setAutoReset(false);
    mLongTaskProgressDialog.reset(); // Stops the auto-show timer that is started by QProgressDialog's ctor
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
    if(response)
        *response = Qx::postBlockingError(blockingError.errorInfo, blockingError.choices, blockingError.defaultChoice);
    else
        qFatal("No response argument provided!");
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

void StatusRelay::saveFileRequestHandler(QSharedPointer<QString> file, Core::SaveFileRequest request)
{
    if(file)
    {
        *file = QFileDialog::getSaveFileName(nullptr, request.caption, request.dir,
                                             request.filter, request.selectedFilter, request.options);
    }
    else
        qFatal("No response argument provided!");
}

void StatusRelay::itemSelectionRequestHandler(QSharedPointer<QString> item, const Core::ItemSelectionRequest& request)
{
    if(item)
    {
        bool accepted;
        QString selected = QInputDialog::getItem(nullptr, request.caption, request.label, request.items, 0, false, &accepted);
        *item = accepted ? selected : QString();
    }
    else
        qFatal("No response argument provided!");
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
    /* Always reset the dialog regardless of whether it is visible or not as it may not be currently visible,
     * but queued to be visible on the next event loop iteration and therefore still needs to be hidden
     * immediately after.
     */
     mLongTaskProgressDialog.reset();
}
