// Unit Include
#include "c-play.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "kernel/core.h"
#include "task/t-exec.h"
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

//-Class Functions---------------------------------------------------------------
//Private:
Fp::AddApp CPlay::buildAdditionalApp(const Fp::Db::QueryBuffer& addAppResult)
{
    // Ensure query result is additional app
    assert(addAppResult.source == Fp::Db::Table_Add_App::NAME);

    /* Form additional app from record
     *
     * Most of the descriptive fields here are not required for this app, but include them anyway in case it's later
     * desired to use them for something like logging.
     */
    Fp::AddApp::Builder fpAab;
    fpAab.wId(addAppResult.result.value(Fp::Db::Table_Add_App::COL_ID).toString());
    fpAab.wAppPath(addAppResult.result.value(Fp::Db::Table_Add_App::COL_APP_PATH).toString());
    fpAab.wAutorunBefore(addAppResult.result.value(Fp::Db::Table_Add_App::COL_AUTORUN).toString());
    fpAab.wLaunchCommand(addAppResult.result.value(Fp::Db::Table_Add_App::COL_LAUNCH_COMMAND).toString());
    fpAab.wName(addAppResult.result.value(Fp::Db::Table_Add_App::COL_NAME).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpAab.wWaitExit(addAppResult.result.value(Fp::Db::Table_Add_App::COL_WAIT_EXIT).toString());
    fpAab.wParentId(addAppResult.result.value(Fp::Db::Table_Add_App::COL_PARENT_ID).toString());

    return fpAab.build();
}

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
    Fp::Db::EntryFilter addAppFilter{.type = Fp::Db::EntryType::AddApp, .parent = game.id()};

    Fp::DbError addAppSearchError;
    Fp::Db::QueryBuffer addAppSearchResult;

    addAppSearchError = db->queryEntrys(addAppSearchResult, addAppFilter);
    if(addAppSearchError.isValid())
    {
        postDirective<DError>(addAppSearchError);
        return addAppSearchError;
    }

    // Enqueue auto-run before apps
    for(int i = 0; i < addAppSearchResult.size; i++)
    {
        // Go to next record
        addAppSearchResult.result.next();

        // Build
        Fp::AddApp addApp = buildAdditionalApp(addAppSearchResult);

        // Enqueue if auto-run before
        if(addApp.isAutorunBefore())
        {
            logEvent(LOG_EVENT_FOUND_AUTORUN.arg(addApp.name()));

            if(sError = enqueueAdditionalApp(addApp, game, Task::Stage::Auxiliary); sError.isValid())
                return sError;
        }
    }

    // Enqueue game
    postDirective<DStatusUpdate>(STATUS_PLAY, game.title());
    if(sError = enqueueGame(game, gameData, Task::Stage::Primary); sError.isValid())
        return sError;

    // Enqueue service shutdown
    mCore.enqueueShutdownTasks();

    return Qx::Error();
}

Qx::Error CPlay::handleEntry(const Fp::AddApp& addApp)
{
    logEvent(LOG_EVENT_ID_MATCH_ADDAPP.arg(addApp.name(),
                                           addApp.parentId().toString(QUuid::WithoutBraces)));

    Qx::Error sError;
    Fp::Db* db = mCore.fpInstall().database();

    // Get parent info
    QUuid parentId = addApp.parentId();
    Fp::Entry parentEntry;
    Fp::Game parentGame;
    Fp::GameData parentGameData;

    if(Fp::DbError gdErr = db->getEntry(parentEntry, parentId); gdErr.isValid())
    {
        postDirective<DError>(gdErr);
        return gdErr;
    }
    Q_ASSERT(std::holds_alternative<Fp::Game>(parentEntry));
    parentGame = std::get<Fp::Game>(parentEntry);

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
    bool needsServices = addApp.appPath() != Fp::Db::Table_Add_App::ENTRY_MESSAGE && addApp.appPath() != Fp::Db::Table_Add_App::ENTRY_EXTRAS;
    if(needsServices && (sError = mCore.enqueueStartupTasks(serverOverride)).isValid())
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
    if(sError = enqueueAdditionalApp(addApp, parentGame, Task::Stage::Primary); sError.isValid())
        return sError;

    // Enqueue service shutdown if needed
    if(needsServices)
        mCore.enqueueShutdownTasks();

    return Qx::Error();
}

Qx::Error CPlay::enqueueAdditionalApp(const Fp::AddApp& addApp, const Fp::Game& parent, Task::Stage taskStage)
{
    if(addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_MESSAGE)
    {
        TMessage* messageTask = new TMessage(mCore);
        messageTask->setStage(taskStage);
        messageTask->setText(addApp.launchCommand());
        messageTask->setBlocking(addApp.isWaitExit());

        mCore.enqueueSingleTask(messageTask);
    }
    else if(addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
    {
        TExtra* extraTask = new TExtra(mCore);
        extraTask->setStage(taskStage);
        extraTask->setDirectory(QDir(mCore.fpInstall().extrasDirectory().absolutePath() + '/' + addApp.launchCommand()));

        mCore.enqueueSingleTask(extraTask);
    }
    else
    {
        TExec* execTask = useRuffle(parent, taskStage) ? createRuffleTask(addApp.name(), addApp.launchCommand()) :
                                                         createStdAddAppExecTask(addApp, parent, taskStage);
        addExtraExecParameters(execTask, taskStage);
        mCore.enqueueSingleTask(execTask);

#ifdef _WIN32
        // Add wait task if required
        if(Qx::Error ee = mCore.conditionallyEnqueueBideTask(execTask); ee.isValid())
            return ee;
#endif
    }

    // Return success
    return Qx::Error();
}

Qx::Error CPlay::enqueueGame(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage)
{
    TExec* execTask = useRuffle(game, taskStage) ? createRuffleTask(game.title(), !gameData.isNull() ? gameData.launchCommand() : game.launchCommand()) :
                                                   createStdGameExecTask(game, gameData, taskStage);

    addExtraExecParameters(execTask, taskStage);
    mCore.enqueueSingleTask(execTask);

#ifdef _WIN32
    // Add wait task if required
    if(Qx::Error ee = mCore.conditionallyEnqueueBideTask(execTask); ee.isValid())
        return ee;
#endif

    // Return success
    return Qx::Error();
}

TExec* CPlay::createStdGameExecTask(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage)
{
    QString gamePath = mCore.resolveFullAppPath(!gameData.isNull() ? gameData.appPath() : game.appPath(),
                                                game.platformName());
    QFileInfo gamePathInfo(gamePath);
    QString param = !gameData.isNull() ? gameData.launchCommand() : game.launchCommand();

    TExec* gameTask = new TExec(mCore);
    gameTask->setIdentifier(game.title());
    gameTask->setStage(taskStage);
    gameTask->setExecutable(QDir::cleanPath(gamePathInfo.absoluteFilePath())); // Like canonical but doesn't care if path DNE
    gameTask->setDirectory(gamePathInfo.absoluteDir());
    gameTask->setParameters(param);
    gameTask->setEnvironment(mCore.childTitleProcessEnvironment());
    gameTask->setProcessType(TExec::ProcessType::Blocking);

    return gameTask;
}

TExec* CPlay::createStdAddAppExecTask(const Fp::AddApp& addApp, const Fp::Game& parent, Task::Stage taskStage)
{
    QString addAppPath = mCore.resolveFullAppPath(addApp.appPath(), parent.platformName());
    QFileInfo addAppPathInfo(addAppPath);
    QString param = addApp.launchCommand();

    TExec* addAppTask = new TExec(mCore);
    addAppTask->setIdentifier(addApp.name());
    addAppTask->setStage(taskStage);
    addAppTask->setExecutable(QDir::cleanPath(addAppPathInfo.absoluteFilePath())); // Like canonical but doesn't care if path DNE
    addAppTask->setDirectory(addAppPathInfo.absoluteDir());
    addAppTask->setParameters(param);
    addAppTask->setEnvironment(mCore.childTitleProcessEnvironment());
    addAppTask->setProcessType(addApp.isWaitExit() || taskStage == Task::Stage::Primary ? TExec::ProcessType::Blocking : TExec::ProcessType::Deferred);

    return addAppTask;
}

TExec* CPlay::createRuffleTask(const QString& name, const QString& originalParams)
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

    TExec* ruffleTask = new TExec(mCore);
    ruffleTask->setIdentifier(name);
    ruffleTask->setStage(Task::Stage::Primary);
    ruffleTask->setExecutable(ruffle.absoluteFilePath());
    ruffleTask->setDirectory(ruffle.absoluteDir());
    ruffleTask->setParameters(newParams);
    ruffleTask->setEnvironment(mCore.childTitleProcessEnvironment());
    ruffleTask->setProcessType(TExec::ProcessType::Blocking);

    return ruffleTask;
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
    return std::visit([this](auto arg) { return this->handleEntry(arg); }, entry);
}

//Public:
bool CPlay::requiresServices() const { return true; }
