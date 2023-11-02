// Unit Include
#include "c-update.h"

// Qt Includes
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Qx Includes
#include <qx/core/qx-json.h>
#include <qx/core/qx-genericerror.h>

// Project Includes
#include "task/t-download.h"
#include "task/t-extract.h"
#include "task/t-exec.h"
#include "utility.h"

QX_JSON_STRUCT_OUTSIDE(CUpdate::ReleaseAsset,
    name,
    browser_download_url
)

QX_JSON_STRUCT_OUTSIDE(CUpdate::ReleaseData,
    name,
    tag_name,
    assets
)

//===============================================================================================================
// CUpdateError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CUpdateError::CUpdateError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CUpdateError::isValid() const { return mType != NoError; }
QString CUpdateError::specific() const { return mSpecific; }
CUpdateError::Type CUpdateError::type() const { return mType; }

//Private:
Qx::Severity CUpdateError::deriveSeverity() const { return Qx::Critical; }
quint32 CUpdateError::deriveValue() const { return mType; }
QString CUpdateError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CUpdateError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CUpdate
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CUpdate::CUpdate(Core& coreRef) : Command(coreRef) {}

//-Class Functions----------------------------------------------------------------
QDir CUpdate::updateCacheDir()
{
    static QDir ucd(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/update"_s);
    return ucd;
}

QDir CUpdate::updateDownloadDir() { return updateCacheDir().absoluteFilePath(u"download"_s); }
QDir CUpdate::updateDataDir() { return updateCacheDir().absoluteFilePath(u"data"_s); }
QDir CUpdate::updateBackupDir() { return updateCacheDir().absoluteFilePath(u"backup"_s); }

QString CUpdate::sanitizeCompiler(QString cmp)
{
    if(cmp.contains("clang", Qt::CaseInsensitive))
        cmp = "clang++";
    else if(cmp.contains("gnu", Qt::CaseInsensitive))
        cmp = "gcc++";
    else if(cmp.contains("msvc", Qt::CaseInsensitive))
        cmp = "msvc";
    else
        cmp = cmp.toLower();

    return cmp;
}

//-Instance Functions-------------------------------------------------------------
//Private:
CUpdateError CUpdate::getLatestReleaseData(ReleaseData& data) const
{
    // Get latest release data via GitHub REST

    // Prepare
    QEventLoop waiter; // Generally avoid nested event loops, but safe here as re-entry is impossible
    QNetworkAccessManager nm;
    nm.setAutoDeleteReplies(true);
    nm.setTransferTimeout(2000);
    QNetworkRequest req(u"https://api.github.com/repos/oblivioncth/CLIFp/releases/latest"_s);
    req.setRawHeader("Accept"_ba, "application/vnd.github+json"_ba);

    // Get
    QNetworkReply* reply = nm.get(req);

    // Result handler
    CUpdateError restError;
    QByteArray response;
    //clazy:excludeall=lambda-in-connect
    QObject::connect(reply, &QNetworkReply::finished, &mCore, [&]{
        if(reply->error() == QNetworkReply::NoError)
            response = reply->readAll();
        else
            restError = CUpdateError(CUpdateError::ConnectionError, reply->errorString());

        waiter.quit();
    });

    // Wait on result
    waiter.exec();
    if(restError.isValid())
        return restError;

    // Parse data
    QJsonParseError jpe;
    QJsonDocument jd = QJsonDocument::fromJson(response, &jpe);
    if(jpe.error != QJsonParseError::NoError)
        return CUpdateError(CUpdateError::InvalidUpdateData, jpe.errorString());
    if(Qx::JsonError je = Qx::parseJson(data, jd); je.isValid())
        return CUpdateError(CUpdateError::InvalidUpdateData, je.action());

    return CUpdateError();
}

QString CUpdate::getTargetAssetName(const QString& tagName) const
{
    BuildInfo bi = mCore.buildInfo();

    QString cmpStr = sanitizeCompiler(bi.compiler);
    if(bi.system == BuildInfo::Linux)
        cmpStr = RELEASE_ASSET_LINUX_CMP_TEMPLATE.arg(cmpStr, QString::number(bi.compilerVersion.majorVersion()));

    return RELEASE_ASSET_MAIN_TEMPLATE.arg(
        tagName,
        ENUM_NAME(bi.system),
        ENUM_NAME(bi.linkage),
        cmpStr
    );
}

CUpdateError CUpdate::checkAndPrepareUpdate() const
{
    // Check for update
    mCore.setStatus(STATUS, STATUS_CHECKING);
    mCore.logEvent(NAME, LOG_EVENT_CHECKING_FOR_NEWER_VERSION);

    // Get new release data
    ReleaseData rd;
    if(CUpdateError ue = getLatestReleaseData(rd); ue.isValid())
        return ue;

    // Check if newer
    QVersionNumber currentVersion = QVersionNumber::fromString(QCoreApplication::applicationVersion());
    Q_ASSERT(!currentVersion.isNull());
    QVersionNumber newVersion = QVersionNumber::fromString(rd.tag_name.mid(1)); // Drops 'v'
    if(newVersion.isNull())
        return CUpdateError(CUpdateError::InvalidReleaseVersion);

    if(newVersion <= currentVersion)
    {
        mCore.postMessage(Message{.text = MSG_NO_UPDATES});
        mCore.logEvent(NAME, MSG_NO_UPDATES);
        return CUpdateError();
    }
    mCore.logEvent(NAME, LOG_EVENT_UPDATE_AVAILABLE.arg(rd.tag_name));

    // Get current build info
    BuildInfo bi = mCore.buildInfo();

    // Check for applicable artifact
    QString targetAsset = getTargetAssetName(rd.tag_name);

    auto aItr = std::find_if(rd.assets.cbegin(), rd.assets.cend(), [&](const auto& a){
        return a.name == targetAsset;
    });

    if(aItr == rd.assets.cend())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::Warning, 12181, WRN_NO_MATCHING_BUILD_P, WRN_NO_MATCHING_BUILD_S));
        return CUpdateError();
    }

    if(mCore.requestQuestionAnswer(QUES_UPDATE.arg(rd.name)))
    {
        mCore.logEvent(NAME, LOG_EVENT_UPDATE_ACCEPED);

        // Queue update
        QDir uDownloadDir = updateDownloadDir();
        QDir uDataDir = updateDataDir();

        QString tempName = u"clifp_update.zip"_s;
        TDownload* downloadTask = new TDownload(&mCore);
        downloadTask->setStage(Task::Stage::Primary);
        downloadTask->setTargetFile(aItr->browser_download_url);
        downloadTask->setDestinationPath(uDownloadDir.absolutePath());
        downloadTask->setDestinationFilename(tempName);
        mCore.enqueueSingleTask(downloadTask);

        TExtract* extractTask = new TExtract(&mCore);
        extractTask->setStage(Task::Stage::Primary);
        extractTask->setPackPath(uDownloadDir.absoluteFilePath(tempName));
        extractTask->setDestinationPath(uDataDir.absolutePath());
        mCore.enqueueSingleTask(extractTask);

        TExec* execTask = new TExec(&mCore);
        execTask->setStage(Task::Stage::Primary);
        execTask->setIdentifier(UPDATE_STAGE_NAME);
        QString newAppExecPath = uDataDir.absolutePath() + u"/bin"_s;
        execTask->setExecutable(newAppExecPath + '/' + CLIFP_CANONICAL_APP_FILNAME);
        execTask->setDirectory(newAppExecPath);
        execTask->setParameters(QStringList{u"update"_s, u"--install"_s, CLIFP_PATH});
        execTask->setProcessType(TExec::ProcessType::Detached);
        mCore.enqueueSingleTask(execTask);
    }
    else
        mCore.logEvent(NAME, LOG_EVENT_UPDATE_REJECTED);

    return CUpdateError();
}

CUpdateError CUpdate::installUpdate(const QString& appPath) const
{
    //...
}

//Protected:
QList<const QCommandLineOption*> CUpdate::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
QString CUpdate::name() { return NAME; }

Qx::Error CUpdate::perform()
{
    // Install stage
    if(mParser.isSet(CL_OPTION_INSTALL))
        return installUpdate(mParser.value(CL_OPTION_INSTALL));

    // Prepare stage
    CoreError err = mCore.blockNewInstances(); // This command allows multi-instance so we do this manually here
    if(err.isValid())
        return err;
    else
        return checkAndPrepareUpdate();
}

//Public:
bool CUpdate::requiresFlashpoint() const { return false; }
