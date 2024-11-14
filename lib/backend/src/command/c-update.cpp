// Unit Include
#include "c-update.h"

// Qt Includes
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>

// Qx Includes
#include <qx/core/qx-genericerror.h>

// Project Includes
#include "kernel/core.h"
#include "task/t-download.h"
#include "task/t-extract.h"
#include "task/t-exec.h"
#include "utility.h"

//===============================================================================================================
// CUpdateError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CUpdateError::CUpdateError(Type t, const QString& s, Qx::Severity sv) :
    mType(t),
    mSpecific(s),
    mSeverity(sv)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CUpdateError::isValid() const { return mType != NoError; }
QString CUpdateError::specific() const { return mSpecific; }
CUpdateError::Type CUpdateError::type() const { return mType; }

//Private:
Qx::Severity CUpdateError::deriveSeverity() const { return mSeverity; }
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

QString CUpdate::substitutePathNames(const QString& path, QStringView binName, QStringView appName)
{
    static const QString stdBin = u"bin"_s;
    static const QString installerName = CLIFP_CUR_APP_FILENAME;

    QString subbed = path;

    if(subbed.startsWith(stdBin))
        subbed = subbed.mid(stdBin.size()).prepend(binName);
    if(subbed.endsWith(installerName))
    {
        subbed.chop(installerName.size());
        subbed.append(appName);
    }

    return subbed;
}

Qx::IoOpReport CUpdate::determineNewFiles(QStringList& files, const QDir& sourceRoot)
{
    files = {};

    QStringList subDirs = {u"bin"_s};
    for(const auto& sd : {u"lib"_s, u"plugins"_s})
        if(sourceRoot.exists(sd))
            subDirs += sd;

    for(const auto& sd : qAsConst(subDirs))
    {
        QStringList subFiles;
        Qx::IoOpReport r = Qx::dirContentList(subFiles, sourceRoot.absoluteFilePath(sd), {}, QDir::NoDotAndDotDot | QDir::Files,
                                              QDirIterator::Subdirectories, Qx::PathType::Relative);
        if(r.isFailure())
            return r;

        for(auto& p : subFiles)
            p.prepend(sd + '/');

        files += subFiles;
    }

    files.removeIf([](const QString& s){ return s == u"bin/"_s + CLIFP_CUR_APP_BASENAME + u".log"_s; });

    return Qx::IoOpReport(Qx::IO_OP_ENUMERATE, Qx::IO_SUCCESS, sourceRoot);
}

CUpdate::UpdateTransfers CUpdate::determineTransfers(const QStringList& files, const TransferSpecs& specs)
{
    UpdateTransfers transfers;

    for(const QString& file : files)
    {
        QString installPath = specs.installRoot.filePath(substitutePathNames(file, specs.binName, specs.appName));
        QString updatePath = specs.updateRoot.filePath(file);
        QString backupPath = specs.backupRoot.filePath(file);
        transfers.install << FileTransfer{.source = updatePath, .dest = installPath};
        transfers.backup << FileTransfer{.source = installPath, .dest = backupPath};
    }

    return transfers;
}

//Public:
bool CUpdate::isUpdateCacheClearable() { return updateCacheDir().exists() && !smPersistCache; }

CUpdateError CUpdate::clearUpdateCache()
{
    QDir uc = updateCacheDir();
    bool removed = uc.exists() && uc.removeRecursively();
    return CUpdateError(removed ? CUpdateError::NoError : CUpdateError::CacheClearFail, {}, Qx::Warning);
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

CUpdateError CUpdate::handleTransfers(const UpdateTransfers& transfers) const
{
    auto doTransfer = [&](const FileTransfer& ft, bool mkpath, bool move, bool overwrite){
        logEvent(LOG_EVENT_FILE_TRANSFER.arg(ft.source, ft.dest));

        if(mkpath)
        {
            QDir destDir(QFileInfo(ft.dest).absolutePath());
            if(!destDir.mkpath(u"."_s))
                return false;
        }

        if(overwrite && QFile::exists(ft.dest) && !QFile::remove(ft.dest))
            return false;

        return move ? QFile::rename(ft.source, ft.dest) : QFile::copy(ft.source, ft.dest);
    };

    // Backup, and note for restore
    logEvent(LOG_EVENT_BACKUP_FILES);
    QList<FileTransfer> restoreTransfers;
    QScopeGuard restoreOnFail([&]{
        if(!restoreTransfers.isEmpty())
        {
            logEvent(LOG_EVENT_RESTORE_FILES);
            for(const auto& t : restoreTransfers) doTransfer(t, false, true, true);
        }
    });

    for(const auto& ft : transfers.backup)
    {
        if(!doTransfer(ft, true, true, true))
        {
            CUpdateError err(CUpdateError::TransferFail, ft.dest);
            postDirective<DError>(err);
            return err;
        }
        restoreTransfers << FileTransfer{.source = ft.dest, .dest = ft.source};
    }

    // Install
    logEvent(LOG_EVENT_INSTALL_FILES);
    for(const auto& ft : transfers.install)
    {
        if(!doTransfer(ft, true, false, false))
        {
            CUpdateError err(CUpdateError::TransferFail, ft.dest);
            postDirective<DError>(err);
            return err;
        }
    }
    restoreOnFail.dismiss();

    return CUpdateError();
}

CUpdateError CUpdate::checkAndPrepareUpdate() const
{
     // This command had auto instance block disabled so we do this manually here
    if(!mCore.blockNewInstances())
    {
        CUpdateError err(CUpdateError::AlreadyOpen);
        postDirective<DError>(err);
        return err;
    }

    // Check for update
    postDirective<DStatusUpdate>(STATUS, STATUS_CHECKING);
    logEvent(LOG_EVENT_CHECKING_FOR_NEWER_VERSION);

    // Get new release data
    ReleaseData rd;
    if(CUpdateError ue = getLatestReleaseData(rd); ue.isValid())
    {
        postDirective<DError>(ue);
        return ue;
    }

    // Check if newer
    QVersionNumber currentVersion = QVersionNumber::fromString(QCoreApplication::applicationVersion().mid(1)); // Drops 'v'
    Q_ASSERT(!currentVersion.isNull());
    QVersionNumber newVersion = QVersionNumber::fromString(rd.tag_name.mid(1)); // Drops 'v'
    if(newVersion.isNull())
    {
        CUpdateError err(CUpdateError::InvalidReleaseVersion);
        postDirective<DError>(err);
        return err;
    }

    if(newVersion <= currentVersion)
    {
        postDirective<DMessage>(MSG_NO_UPDATES);
        logEvent(MSG_NO_UPDATES);
        return CUpdateError();
    }
    logEvent(LOG_EVENT_UPDATE_AVAILABLE.arg(rd.tag_name));

    // Get current build info
    BuildInfo bi = mCore.buildInfo();

    // Check for applicable artifact
    QString targetAsset = getTargetAssetName(rd.tag_name);

    auto aItr = std::find_if(rd.assets.cbegin(), rd.assets.cend(), [&](const auto& a){
        return a.name == targetAsset;
    });

    if(aItr == rd.assets.cend())
    {
        postDirective<DError>(Qx::GenericError(Qx::Warning, 12181, WRN_NO_MATCHING_BUILD_P, WRN_NO_MATCHING_BUILD_S));
        return CUpdateError();
    }

    bool shouldUpdate = postDirective<DYesOrNo>(QUES_UPDATE.arg(rd.name));
    if(shouldUpdate)
    {
        logEvent(LOG_EVENT_UPDATE_ACCEPED);

        // Queue update
        QDir uDownloadDir = updateDownloadDir();
        QDir uDataDir = updateDataDir();

        QString tempName = u"clifp_update.zip"_s;
        TDownload* downloadTask = new TDownload(mCore);
        downloadTask->setStage(Task::Stage::Primary);
        downloadTask->setDescription(u"update"_s);
        downloadTask->addFile({.target = aItr->browser_download_url, .dest = uDownloadDir.absoluteFilePath(tempName)});
        mCore.enqueueSingleTask(downloadTask);

        TExtract* extractTask = new TExtract(mCore);
        extractTask->setStage(Task::Stage::Primary);
        extractTask->setPackPath(uDownloadDir.absoluteFilePath(tempName));
        extractTask->setDestinationPath(uDataDir.absolutePath());
        mCore.enqueueSingleTask(extractTask);

        TExec* execTask = new TExec(mCore);
        execTask->setStage(Task::Stage::Primary);
        execTask->setIdentifier(UPDATE_STAGE_NAME);
        QString newAppExecPath = uDataDir.absolutePath() + u"/bin"_s;
        execTask->setExecutable(newAppExecPath + '/' + CLIFP_CANONICAL_APP_FILNAME);
        execTask->setDirectory(newAppExecPath);
        execTask->setParameters(QStringList{u"update"_s, u"--install"_s, CLIFP_PATH});
        execTask->setProcessType(TExec::ProcessType::Detached);
        mCore.enqueueSingleTask(execTask);

        // Maintain update cache until installer runs
        smPersistCache = true;
    }
    else
        logEvent(LOG_EVENT_UPDATE_REJECTED);

    return CUpdateError();
}

Qx::Error CUpdate::installUpdate(const QFileInfo& existingAppInfo) const
{
    postDirective<DStatusUpdate>(STATUS, STATUS_INSTALLING);

    // Wait for previous process to close, lock instance afterwards
    static const int totalGrace = 2000;
    static const int step = 500;
    int currentGrace = 0;
    bool haveLock = false;

    do
    {
        logEvent(LOG_EVENT_WAITING_ON_OLD_CLOSE.arg(totalGrace - currentGrace));
        QThread::msleep(step);
        currentGrace += step;
        haveLock = mCore.blockNewInstances();
    }
    while(!haveLock && currentGrace < totalGrace);

    // TODO: Allow user retry here (i.e. they close the process manually)
    if(!haveLock)
    {
        CUpdateError err(CUpdateError::OldProcessNotFinished, "Aborting update.");
        postDirective<DError>(err);
        return err;
    }

    //-Install update------------------------------------------------------------
    logEvent(LOG_EVENT_INSTALLING_UPDATE);

    // Ensure old executable exists where expected
    if(!existingAppInfo.exists())
    {
        CUpdateError err(CUpdateError::InvalidPath, "Missing " + existingAppInfo.absoluteFilePath());
        postDirective<DError>(err);
        return err;
    }

    // Note structure
    TransferSpecs ts{
        .updateRoot = updateDataDir(),
        .installRoot = QDir(QDir::cleanPath(existingAppInfo.absoluteFilePath() + "/../..")),
        .backupRoot = updateBackupDir(),
        .appName = existingAppInfo.fileName(),
        .binName = existingAppInfo.absoluteDir().dirName()
    };

    // Determine transfers
    QStringList updateFiles;
    if(Qx::IoOpReport rep = determineNewFiles(updateFiles, ts.updateRoot); rep.isFailure())
    {
        postDirective<DError>(rep);
        return rep;
    }

    UpdateTransfers updateTransfers = determineTransfers(updateFiles, ts);

    // Transfer
    if(CUpdateError err = handleTransfers(updateTransfers); err.isValid())
        return err;

    // Success
    logEvent(MSG_UPDATE_COMPLETE);
    postDirective<DMessage>(MSG_UPDATE_COMPLETE);
    return CUpdateError();
}

//Protected:
QList<const QCommandLineOption*> CUpdate::options() const { return CL_OPTIONS_SPECIFIC + Command::options(); }
QString CUpdate::name() const { return NAME; }

Qx::Error CUpdate::perform()
{
    /* Persist cache during setup so that files remain for install, and during install (so always)
     * since the installer cannot delete itself while it's running
     */
    smPersistCache = true;
    return mParser.isSet(CL_OPTION_INSTALL) ? installUpdate(QFileInfo(mParser.value(CL_OPTION_INSTALL))) :
                                              checkAndPrepareUpdate();
}

//Public:
bool CUpdate::requiresFlashpoint() const { return false; }
bool CUpdate::autoBlockNewInstances() const { return false; }
