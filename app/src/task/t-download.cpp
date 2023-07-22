// Unit Include
#include "t-download.h"

//===============================================================================================================
// TDownloadError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TDownloadError::TDownloadError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TDownloadError::isValid() const { return mType != NoError; }
QString TDownloadError::specific() const { return mSpecific; }
TDownloadError::Type TDownloadError::type() const { return mType; }

//Private:
Qx::Severity TDownloadError::deriveSeverity() const { return Qx::Critical; }
quint32 TDownloadError::deriveValue() const { return mType; }
QString TDownloadError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TDownloadError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// TDownload
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
TDownload::TDownload(QObject* parent) :
    Task(parent)
{
    // Setup download manager
    mDownloadManager.setOverwrite(true);

    // Since this is only for one download, the size will be adjusted to the correct total as soon as the download starts
    mDownloadManager.setSkipEnumeration(true);

    // Download event handlers
    connect(&mDownloadManager, &Qx::AsyncDownloadManager::sslErrors, this, [this](Qx::Error errorMsg, bool* ignore) {
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
    ml.append(u".destinationPath() = \""_s + QDir::toNativeSeparators(mDestinationPath) + u"\""_s);
    ml.append(u".destinationFilename() = \""_s + mDestinationFilename + u"\""_s);
    ml.append(u".targetFile() = \""_s + mTargetFile.toString() + u"\""_s);
    ml.append(u".sha256() = "_s + mSha256);
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
    QFile packFile(mDestinationPath + '/' + mDestinationFilename);
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
    Qx::Error errorStatus;

    // Handle result
    emit longTaskFinished();
    if(downloadReport.wasSuccessful())
    {
        // Confirm checksum is correct
        QFile packFile(mDestinationPath + '/' + mDestinationFilename);
        bool checksumMatch;
        Qx::IoOpReport cr = Qx::fileMatchesChecksum(checksumMatch, packFile, mSha256, QCryptographicHash::Sha256);
        if(cr.isFailure() || !checksumMatch)
        {
            TDownloadError err(TDownloadError::ChecksumMismatch, cr.isFailure() ? cr.outcomeInfo() : u""_s);
            errorStatus = err;
            emit errorOccurred(NAME, errorStatus);
        }
        else
            emit eventOccurred(NAME, LOG_EVENT_DOWNLOAD_SUCC);
    }
    else
    {
        TDownloadError err(TDownloadError::Incomeplete, downloadReport.outcomeString());
        errorStatus = err;
        emit errorOccurred(NAME, errorStatus);
    }

    emit complete(errorStatus);
}
