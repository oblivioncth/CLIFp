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
    mDownloadManager.setVerificationMethod(QCryptographicHash::Sha256);

    // Download event handlers
    connect(&mDownloadManager, &Qx::AsyncDownloadManager::sslErrors, this, [this](Qx::Error errorMsg, bool* ignore) {
        int choice;
        emit blockingErrorOccurred(NAME, &choice, errorMsg, QMessageBox::Yes | QMessageBox::No);
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

    QString files = u".files() = {\n"_s;
    for(auto i = 0; i < 10 && i < mFiles.size(); i++)
    {
        auto f = mFiles.at(i);
        bool sum = !f.checksum.isEmpty();
        files += u"\t"_s + FILE_DOWNLOAD_TEMPLATE.arg(f.target.toString(), f.dest, sum ? f.checksum : FILE_NO_CHECKSUM) + '\n';
    }
    if(mFiles.size() > 10)
        files += FILE_DOWNLOAD_ELIDE.arg(mFiles.size() - 10) + '\n';
    files += u"}"_s;
    ml.append(files);
    ml.append(u".description() = \""_s + mDescription + u"\""_s);

    return ml;
}

bool TDownload::isEmpty() const { return mFiles.isEmpty(); }
qsizetype TDownload::fileCount() const { return mFiles.size(); }
QList<Qx::DownloadTask> TDownload::files() const { return mFiles; }
QString TDownload::description() const { return mDescription; }

void TDownload::addFile(const Qx::DownloadTask file) { mFiles.append(file); }
void TDownload::setDescription(const QString& desc) { mDescription = desc; }

void TDownload::perform()
{
    // Add files
    for(const auto& f : mFiles)
        mDownloadManager.appendTask(f);

    // Log/label string
    QString label = LOG_EVENT_DOWNLOAD.arg(mDescription);
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
        emit eventOccurred(NAME, LOG_EVENT_DOWNLOAD_SUCC);
    else
    {
        errorStatus = TDownloadError(TDownloadError::Incomeplete, downloadReport.outcomeString());
        emit errorOccurred(NAME, errorStatus);
    }

    emit complete(errorStatus);
}
