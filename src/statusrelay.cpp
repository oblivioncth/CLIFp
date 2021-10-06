#include "statusrelay.h"

#include "qx-widgets.h"

#include <QApplication>
#include <QScopedValueRollback>

//===============================================================================================================
// STATUS RELAY
//===============================================================================================================

//-Constructor----------------------------------------------------------------
StatusRelay::StatusRelay(QObject* parent) :
    QObject(parent),
    mTrayIcon(QIcon(":/res/icon/CLIFp.ico"))
{
    mTrayIcon.setToolTip(SYS_TRAY_STATUS);
    connect(&mTrayIcon, &QSystemTrayIcon::activated, [this](){
        mTrayIcon.showMessage(mStatusHeading, mStatusMessage);
    });
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
    error.errorInfo.exec(QMessageBox::Ok, QMessageBox::NoButton);
}

void StatusRelay::blockingErrorHandler(QSharedPointer<int> response, Core::BlockingError blockingError)
{
    *response = blockingError.errorInfo.exec(blockingError.choices, blockingError.defaultChoice);
}

void StatusRelay::messageHandler(const QString& message)
{
    QMessageBox::information(nullptr, QApplication::applicationName(), message);
}

void StatusRelay::authenticationHandler(QString prompt, QString* username, QString* password, bool* abort)
{
    Qx::LoginDialog ld;
    ld.setPrompt(prompt);

    int choice = ld.exec();

    if(choice == QDialog::Accepted)
    {
        *username = ld.getUsername();
        *password = ld.getPassword();
    }
    else
        *abort = true;
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
         NOTE: It may have been from accidentally running the app without copying the new build into the FP directory,
         and therefore deleteLater wasn't actually used on that run, but one time when testing this deleteLater still
         deleted the progress dialog too early (before previous call to setValue finished (see stack), causing an access
         violation crash. This only applies if mDownloadProgressDialog is set to be modal, which at this time is not the
         case, but be aware of this if that is ever changed.
        */
    }
}
