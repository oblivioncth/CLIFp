// Unit Include
#include "c-play.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>
#include <qx/utility/qx-helpers.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "kernel/core.h"
#include "task/t-exec.h"
#include "task/t-titleexec.h"
#include "task/t-message.h"
#include "task/t-extra.h"

/* TODO: Add switch to force ruffle (for flash games, either error or ignore if not flash) like the vanilla launcher
 * https://github.com/FlashpointProject/launcher/blob/1d14bc753397c40a4e7c1869cd8e214eee5de843/extensions/core-ruffle/src/extension.ts#L73
 */

//===============================================================================================================
// CPlayError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CPlayError::CPlayError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CPlayError::isValid() const { return mType != NoError; }
QString CPlayError::specific() const { return mSpecific; }
CPlayError::Type CPlayError::type() const { return mType; }

//Private:
Qx::Severity CPlayError::deriveSeverity() const { return Qx::Critical; }
quint32 CPlayError::deriveValue() const { return mType; }
QString CPlayError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CPlayError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CPLAY
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CPlay::CPlay(Core& coreRef, const QStringList& commandLine) : TitleCommand(coreRef, commandLine) {}

//-Instance Functions-------------------------------------------------------------
//Private:
void CPlay::addExtraExecParameters(TExec* execTask, Task::Stage taskStage)
{
    // Currently only affects primary execs
    if(taskStage != Task::Stage::Primary)
        return;

    QStringList extraParams;

    // Check for fullscreen
    if(mParser.isSet(CL_OPTION_FULLSCREEN))
    {
        logEvent(LOG_EVENT_FORCING_FULLSCREEN);
        auto lookupName = QFileInfo(execTask->executable()).baseName().toLower();
        auto fsItr = FULLSCREEN_PARAMS.constFind(lookupName);
        if(fsItr != FULLSCREEN_PARAMS.cend())
        {
            auto fsParam = *fsItr;
            logEvent(LOG_EVENT_FULLSCREEN_SUPPORTED.arg(fsParam));
            extraParams.append(fsParam);
        }
        else
            logEvent(LOG_EVENT_FULLSCREEN_UNSUPPORTED);
    }

    // Get passthrough args
    if(auto ptp = mParser.positionalArguments(); !ptp.isEmpty())
        extraParams.append(ptp);

    /* Add extra params, which may be
     * - Passthrough args (i.e. <rest_of_command> -- <passthrough_args>)
     * - Fullscreen params
     */
    auto params = execTask->parameters();
    std::visit(qxFuncAggregate{
        [&](QString& ps){
                ps.append(u' ');
                ps.append(TExec::joinArguments(extraParams));
        },
        [&](QStringList& ps){
            ps.append(extraParams);
        }
    }, params);
    execTask->setParameters(params);
}

QString CPlay::getServerOverride(const Fp::GameData& gd)
{
    QString override = gd.isNull() ? QString() : gd.parameters().server();
    if(!override.isNull())
        logEvent(LOG_EVENT_SERVER_OVERRIDE.arg(override));

    return override;
}

bool CPlay::useRuffle(const Fp::Game& game, Task::Stage stage)
{
    if(game.platformName() != u"Flash"_s || stage != Task::Stage::Primary)
        return false;

    auto& extCfg = mCore.fpInstall().extConfig();
    if(mParser.isSet(CL_OPTION_RUFFLE))
    {
        logEvent(LOG_EVENT_FORCING_RUFFLE);
        return true;
    }
    else if(mParser.isSet(CL_OPTION_FLASH))
    {
        logEvent(LOG_EVENT_FORCING_FLASH);
        return false;
    }
    else if(extCfg.com_ruffle_enabled && !game.ruffleSupport().isEmpty())
    {
        logEvent(LOG_EVENT_USING_RUFFLE_SUPPORTED);
        return true;
    }
    else if(extCfg.com_ruffle_enabled_all)
    {
        logEvent(LOG_EVENT_USING_RUFFLE_UNSUPPORTED);
        return true;
    }

    return false;
}

Qx::Error CPlay::handleEntry(const Fp::Game& game)
{
    logEvent(LOG_EVENT_ID_MATCH_TITLE.arg(game.title()));

    Qx::Error sError;
    Fp::Db* db = mCore.fpInstall().database();

    // Get game data (if present)
    Fp::GameData gameData;
    if(Fp::DbError gdErr = db->getGameData(gameData, game.id()); gdErr.isValid())
    {
        postDirective<DError>(gdErr);
        return gdErr;
    }
    bool hasDatapack = !gameData.isNull();

    // Get server override (if not present, will result in the default server being used)
    QString serverOverride = getServerOverride(gameData);

    // Enqueue services
    if(sError = mCore.enqueueStartupTasks(serverOverride); sError.isValid())
        return sError;

    // Handle datapack tasks
    if(hasDatapack)
    {
        logEvent(LOG_EVENT_DATA_PACK_TITLE);

        if(sError = mCore.enqueueDataPackTasks(gameData); sError.isValid())
            return sError;
    }

    // Get game's additional apps
    Fp::Db::AddAppFilter aaf{.parent = game.id()};
    QList<Fp::AddApp> addAppSearchResult;
    if(auto err = db->searchAddApps(addAppSearchResult, aaf); err.isValid())
    {
        postDirective<DError>(err);
        return err;
    }

    // Enqueue auto-run before apps
    for(const auto& aa : std::as_const(addAppSearchResult))
    {
        // Enqueue if auto-run before
        if(aa.isAutorunBefore())
        {
            logEvent(LOG_EVENT_FOUND_AUTORUN.arg(aa.name()));

            if(sError = enqueueEntry(aa, game, Task::Stage::Auxiliary); sError.isValid())
                return sError;
        }
    }

    // Enqueue game
    postDirective<DStatusUpdate>(STATUS_PLAY, game.title());
    if(sError = enqueueEntry(game, gameData, Task::Stage::Primary); sError.isValid())
        return sError;

    // Enqueue service shutdown
    mCore.enqueueShutdownTasks();

    return Qx::Error();
}

Qx::Error CPlay::handleEntry(const Fp::AddApp& addApp)
{
    logEvent(LOG_EVENT_ID_MATCH_ADDAPP.arg(addApp.name(),
                                           addApp.parentGameId().toString(QUuid::WithoutBraces)));

    Qx::Error sError;
    Fp::Db* db = mCore.fpInstall().database();

    // Get parent info
    QUuid parentId = addApp.parentGameId();
    Fp::Game parentGame;
    Fp::GameData parentGameData;

    if(Fp::DbError gdErr = db->getGame(parentGame, parentId); gdErr.isValid())
    {
        postDirective<DError>(gdErr);
        return gdErr;
    }

    if(Fp::DbError gdErr = db->getGameData(parentGameData, parentId); gdErr.isValid())
    {
        postDirective<DError>(gdErr);
        return gdErr;
    }

    // Check if parent entry uses a data pack
    bool hasDatapack = !parentGameData.isNull();

    // Get server override (if not present, will result in the default server being used)
    QString serverOverride = getServerOverride(parentGameData);

    // Enqueue services if needed
    if(addApp.isPlayable() && (sError = mCore.enqueueStartupTasks(serverOverride)).isValid())
        return sError;

    // Handle datapack tasks
    if(hasDatapack)
    {
        logEvent(LOG_EVENT_DATA_PACK_TITLE);

        if(sError = mCore.enqueueDataPackTasks(parentGameData); sError.isValid())
            return sError;
    }

    // Enqueue
    postDirective<DStatusUpdate>(STATUS_PLAY, addApp.name());
    if(sError = enqueueEntry(addApp, parentGame, Task::Stage::Primary); sError.isValid())
        return sError;

    // Enqueue service shutdown if needed
    if(addApp.isPlayable())
        mCore.enqueueShutdownTasks();

    return Qx::Error();
}

Qx::Error CPlay::enqueueEntry(const Fp::AddApp& addApp, const Fp::Game& parent, Task::Stage taskStage)
{
    if(addApp.isMessage())
    {
        TMessage* messageTask = new TMessage(mCore);
        messageTask->setStage(taskStage);
        messageTask->setText(addApp.launchCommand());
        messageTask->setBlocking(addApp.isWaitForExit());

        mCore.enqueueSingleTask(messageTask);
    }
    else if(addApp.isExtra())
    {
        TExtra* extraTask = new TExtra(mCore);
        extraTask->setStage(taskStage);
        extraTask->setDirectory(QDir(mCore.fpInstall().extrasDirectory().absolutePath() + '/' + addApp.launchCommand()));

        mCore.enqueueSingleTask(extraTask);
    }
    else
    {
        TExec* execTask = createExecTask(addApp, parent, taskStage);
        addExtraExecParameters(execTask, taskStage);
        mCore.enqueueSingleTask(execTask);
    }

    // Return success
    return Qx::Error();
}

Qx::Error CPlay::enqueueEntry(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage)
{
    TExec* execTask = createExecTask(game, gameData, taskStage);
    addExtraExecParameters(execTask, taskStage);
    mCore.enqueueSingleTask(execTask);

    // Return success
    return Qx::Error();
}

TExec* CPlay::createExecTask(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage)
{
    QString params = !gameData.isNull() ? gameData.launchCommand() : game.launchCommand();
    QString path = !gameData.isNull() ? gameData.applicationPath() : game.applicationPath();
    bool ruffle = useRuffle(game, taskStage);

    TTitleExec* gameTask = new TTitleExec(mCore);
    gameTask->setTrackingId(game.id());
    gameTask->setIdentifier(game.title());
    gameTask->setStage(taskStage);
    gameTask->setEnvironment(mCore.childTitleProcessEnvironment());
    gameTask->setProcessType(TExec::ProcessType::Blocking);

    if(ruffle)
        setupRuffle(gameTask, params);
    else
    {
        QFileInfo fullPathInfo(mCore.resolveFullAppPath(path, game.platformName()));
        gameTask->setExecutable(QDir::cleanPath(fullPathInfo.absoluteFilePath())); // Like canonical but doesn't care if path DNE
        gameTask->setDirectory(fullPathInfo.absoluteDir());
        gameTask->setParameters(params);
    }

    return gameTask;
}

TExec* CPlay::createExecTask(const Fp::AddApp& addApp, const Fp::Game& parent, Task::Stage taskStage)
{
    QString params = addApp.launchCommand();
    QString path = addApp.applicationPath();
    bool ruffle = useRuffle(parent, taskStage);

    TExec* addAppTask;
    if(taskStage == Task::Stage::Primary)
    {
        TTitleExec* titleExec = new TTitleExec(mCore);
        addAppTask = titleExec;
    }
    else
        addAppTask = new TExec(mCore);

    addAppTask->setIdentifier(addApp.name());
    addAppTask->setStage(taskStage);
    addAppTask->setEnvironment(mCore.childTitleProcessEnvironment());
    addAppTask->setProcessType(addApp.isWaitForExit() || taskStage == Task::Stage::Primary ? TExec::ProcessType::Blocking : TExec::ProcessType::Deferred);

    if(ruffle)
        setupRuffle(addAppTask, params);
    else
    {
        QFileInfo fullPathInfo(mCore.resolveFullAppPath(path, parent.platformName()));
        addAppTask->setExecutable(QDir::cleanPath(fullPathInfo.absoluteFilePath())); // Like canonical but doesn't care if path DNE
        addAppTask->setDirectory(fullPathInfo.absoluteDir());
        addAppTask->setParameters(params);
    }

    return addAppTask;
}

void CPlay::setupRuffle(TExec* exec, const QString& originalParams)
{
    /* Replicating:
     *
     * https://github.com/FlashpointProject/launcher/blob/54c5dd8357b9083271d36912ea9546bcd8d1cb18/extensions/core-ruffle/src/extension.ts#L65
     * https://github.com/FlashpointProject/launcher/blob/54c5dd8357b9083271d36912ea9546bcd8d1cb18/extensions/core-ruffle/src/middleware/standalone.ts#L181
     *
     * There is a whole system for supporting different versions of Ruffle, as well as game-specific config options, but current it's unused so we
     * can ignore it. Also if it isn't obvious we can only make use of standalone ruffle.
     *
     * If the ruffle config schema really starts getting used, we might want to move its setup to libfp so that it just comes with the game entry.
     *
     * The ruffle executable path should also likely be part of libfp. Then with this the chmod change should be done at startup using that path.
     *
     * TODO: Download ruffle if it's missing.
     */
#if defined(_WIN32)
    static const QString RUFFLE_EXE = u"Ruffle.exe"_s;
#elif defined(__linux__)
    static const QString RUFFLE_EXE = u"ruffle"_s;
#endif
    auto& fp = mCore.fpInstall();
    static QFileInfo ruffle = [&]{
        QFile file(mCore.fpInstall().dir().absoluteFilePath(u"Data/Ruffle/standalone/latest/"_s + RUFFLE_EXE));
        auto exec = QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther;
        file.setPermissions(file.permissions() | exec);
        return QFileInfo(file);
    }();

    // This likely will need to be dynamic per game/ruffle version at some point
    QStringList newParams = {
        u"--proxy"_s,
        fp.preferences().browserModeProxy
    };
    newParams.append(QProcess::splitCommand(originalParams));

    exec->setExecutable(ruffle.absoluteFilePath());
    exec->setDirectory(ruffle.absoluteDir());
    exec->setParameters(newParams);
}

//Protected:
QList<const QCommandLineOption*> CPlay::options() const { return CL_OPTIONS_SPECIFIC + TitleCommand::options(); }
QString CPlay::name() const { return NAME; }

//Public:
Qx::Error CPlay::perform()
{
    // Get ID of title to start, prioritizing URL
    QUuid titleId;
    if(mParser.isSet(CL_OPTION_URL))
    {
        QRegularExpressionMatch urlMatch = URL_REGEX.match(mParser.value(CL_OPTION_URL));
        if(!urlMatch.hasMatch())
        {
            CPlayError err(CPlayError::InvalidUrl);
            postDirective<DError>(err);
            return err;
        }

        titleId = QUuid(urlMatch.captured(u"id"_s));
    }
    else if(Qx::Error ide = getTitleId(titleId); ide.isValid())
        return ide;

    // Bail if ID is missing (user cancel)
    if(titleId.isNull())
        return CPlayError();

    logEvent(LOG_EVENT_HANDLING_AUTO);

    // Get entry via ID
    Fp::Db* db = mCore.fpInstall().database();

    Fp::Entry entry;
    if(Fp::DbError eErr = db->getEntry(entry, titleId); eErr.isValid())
    {
        postDirective<DError>(eErr);
        return eErr;
    }

    // Handle entry
    return entry.visit([this](auto arg) { return this->handleEntry(arg); });
}

//Public:
bool CPlay::requiresServices() const { return true; }
