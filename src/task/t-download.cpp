// Unit Include
#include "t-download.h"

//===============================================================================================================
// TDownload
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
TDownload::TDownload()
{
    // Setup download manager
    mDownloadManager.setOverwrite(true);

    // Download event handlers
    connect(&mDownloadManager, &Qx::AsyncDownloadManager::sslErrors, this, [this](Qx::GenericError errorMsg, bool* ignore) {
        int choice;
        emit blockingErrorOccured(NAME, &choice, errorMsg, QMessageBox::Yes | QMessageBox::No);
        *ignore = choice == QMessageBox::Yes;
    });

    connect(&mDownloadManager, &Qx::AsyncDownloadManager::authenticationRequired, this, [this](QString prompt) {
        emit eventOccurred(NAME, LOG_EVENT_DOWNLOAD_AUTH.arg(prompt));
    });

    connect(&mDownloadManager, &Qx::AsyncDownloadManager::preSharedKeyAuthenticationRequired, this, [this](QString prompt) {
        emit eventOccurred(NAME, LOG_EVENT_DOWNLOAD_AUTH.arg(prompt));
    });

    connect(&mDownloadManager, &Qx::AsyncDownloadManager::proxyAuthenticationRequired, this, [this](QString prompt) {
        emit eventOccurred(NAME, LOG_EVENT_DOWNLOAD_AUTH.arg(prompt));
    });

    connect(&mDownloadManager, &Qx::AsyncDownloadManager::downloadTotalChanged, this, &TDownload::longTaskTotalChanged);
    connect(&mDownloadManager, &Qx::AsyncDownloadManager::downloadProgress, this, &TDownload::longTaskProgressChanged);
    connect(&mDownloadManager, &Qx::AsyncDownloadManager::finished, this, &TDownload::postDownload);
}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TDownload::name() const { return NAME; }
QStringList TDownload::members() const
{
    QStringList ml = Task::members();
    ml.append(".destinationPath() = \"" + QDir::toNativeSeparators(mDestinationPath) + "\"");
    ml.append(".destinationFilename() = \"" + mDestinationFilename + "\"");
    ml.append(".targetFile() = \"" + mTargetFile.toString() + "\"");
    ml.append(".sha256() = " + mSha256);
    return ml;
}

QString TDownload::destinationPath() const { return mDestinationPath; }
QString TDownload::destinationFilename() const { return mDestinationFilename; }
QUrl TDownload::targetFile() const { return mTargetFile; }
QString TDownload::sha256() const { return mSha256; }

void TDownload::setDestinationPath(QString path) { mDestinationPath = path; }
void TDownload::setDestinationFilename(QString filename) { mDestinationFilename = filename; }
void TDownload::setTargetFile(QUrl targetFile) { mTargetFile = targetFile; }
void TDownload::setSha256(QString sha256) { mSha256 = sha256; }

void TDownload::perform()
{
    // Setup download
    QFile packFile(mDestinationPath + "/" + mDestinationFilename);
    QFileInfo packFileInfo(packFile);
    Qx::DownloadTask download{
        .target = mTargetFile,
        .dest = packFileInfo.absoluteFilePath()
    };
    mDownloadManager.appendTask(download);

    // Log/label string
    QString label = LOG_EVENT_DOWNLOADING_DATA_PACK.arg(packFileInfo.fileName());
    emit eventOccurred(NAME, label);

    // Start download
    emit longTaskStarted(label);
    mDownloadManager.processQueue();
}

void TDownload::stop()
{
    if(mDownloadManager.isProcessing())
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_DOWNLOADS);
        mDownloadManager.abort();
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TDownload::postDownload(Qx::DownloadManagerReport downloadReport)
{
    // Status holder
    ErrorCode errorStatus = ErrorCode::NO_ERR;

    // Handle result
    emit longTaskFinished();
    if(downloadReport.wasSuccessful())
    {
        // Confirm checksum is correct
        QFile packFile(mDestinationPath + "/" + mDestinationFilename);
        bool checksumMatch;
        Qx::IoOpReport checksumResult = Qx::fileMatchesChecksum(checksumMatch, packFile, mSha256, QCryptographicHash::Sha256);
        if(checksumResult.isFailure() || !checksumMatch)
        {
            emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_PACK_SUM_MISMATCH));
            errorStatus = ErrorCode::DATA_PACK_INVALID;
        }
        else
            emit eventOccurred(NAME, LOG_EVENT_DOWNLOAD_SUCC);
    }
    else
    {
        downloadReport.errorInfo().setErrorLevel(Qx::GenericError::Critical);
        emit errorOccurred(NAME, downloadReport.errorInfo());
        errorStatus = ErrorCode::CANT_OBTAIN_DATA_PACK;
    }

    emit complete(errorStatus);
}
