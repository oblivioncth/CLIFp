// Unit Include
#include "statusrelay.h"

// Qt Includes
#include <QApplication>
#include <QScopedValueRollback>

// Qx Includes
#include <qx/widgets/qx-common-widgets.h>
#include <qx/widgets/qx-logindialog.h>

//===============================================================================================================
// STATUS RELAY
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
StatusRelay::StatusRelay(QObject* parent) :
    QObject(parent)
{
    setupTrayIcon();
}

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

//-Signals & Slots-------------------------------------------------------------
//Public Slots:
void StatusRelay::statusChangeHandler(const QString& statusHeading, const QString& statusMessage)
{
    mStatusHeading = statusHeading;
    mStatusMessage = statusMessage;
}

void StatusRelay::errorHandler(Core::Error error)
{
    Qx::postError(error.errorInfo, QMessageBox::Ok, QMessageBox::Ok);
}

void StatusRelay::blockingErrorHandler(QSharedPointer<int> response, Core::BlockingError blockingError)
{
    *response = Qx::postError(blockingError.errorInfo, blockingError.choices, blockingError.defaultChoice);
}

void StatusRelay::messageHandler(const QString& message)
{
    QMessageBox::information(nullptr, QApplication::applicationName(), message);
}

void StatusRelay::authenticationHandler(QString prompt, QAuthenticator* authenticator)
{
    Qx::LoginDialog ld;
    ld.setPrompt(prompt);

    int choice = ld.exec();

    if(choice == QDialog::Accepted)
    {
        authenticator->setUser(ld.username());
        authenticator->setPassword(ld.password());
    }
}

void StatusRelay::downloadProgressHandler(quint64 progress)
{
    if(mDownloadProgressDialog)
    {
        mDownloadProgressDialog->setValue(progress);
    }
}

void StatusRelay::downloadTotalHandler(quint64 total)
{
    if(mDownloadProgressDialog)
    {
        mDownloadProgressDialog->setMaximum(total);
    }
}

void StatusRelay::downloadStartedHandler(QString task)
{
    // Create progress dialog
    mDownloadProgressDialog = new QProgressDialog(task, "Cancel", 0, 0);

    // Initialize dialog
    mDownloadProgressDialog->setWindowModality(Qt::NonModal);
    mDownloadProgressDialog->setMinimumDuration(0);
    mDownloadProgressDialog->setAutoClose(false);
    mDownloadProgressDialog->setAutoReset(false);
    connect(mDownloadProgressDialog, &QProgressDialog::canceled, this, &StatusRelay::downloadCanceled);

    // Show right away
    mDownloadProgressDialog->setValue(0);
}

void StatusRelay::downloadFinishedHandler(bool canceled)
{
    if(mDownloadProgressDialog)
    {
        if(!canceled) // Is already closed if canceled
            mDownloadProgressDialog->close();

        mDownloadProgressDialog->deleteLater(); // May still have pending events from setValue, so can't delete immediately
        mDownloadProgressDialog = nullptr;
        /*
         * NOTE: It may have been from accidentally running the app without copying the new build into the FP directory,
         * and therefore deleteLater wasn't actually used on that run, but one time when testing this deleteLater still
         * deleted the progress dialog too early (before previous call to setValue finished (see stack), causing an access
         * violation crash. This only applies if mDownloadProgressDialog is set to be modal, which at this time is not the
         * case, but be aware of this if that is ever changed.
        */
    }
}
