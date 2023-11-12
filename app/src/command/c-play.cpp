// Unit Include
#include "c-play.h"

// Qt Includes
#include <QApplication>

// Qx Includes
#include <qx/core/qx-regularexpression.h>

// Project Includes
#include "../task/t-exec.h"
#include "../task/t-message.h"
#include "../task/t-extra.h"

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
CPlay::CPlay(Core& coreRef) : TitleCommand(coreRef) {}

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
QString CPlay::getServerOverride(const Fp::GameData& gd)
{
    QString override = gd.isNull() ? QString() : gd.parameters().server();
    if(!override.isNull())
        mCore.logEvent(NAME, LOG_EVENT_SERVER_OVERRIDE.arg(override));

    return override;
}

Qx::Error CPlay::handleEntry(const Fp::Game& game)
{
    mCore.logEvent(NAME, LOG_EVENT_ID_MATCH_TITLE.arg(game.title()));

    Qx::Error sError;
    Fp::Db* db = mCore.fpInstall().database();

    // Get game data (if present)
    Fp::GameData gameData;
    if(Fp::DbError gdErr = db->getGameData(gameData, game.id()); gdErr.isValid())
    {
        mCore.postError(NAME, gdErr);
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
        mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);

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
        mCore.postError(NAME, addAppSearchError);
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
            mCore.logEvent(NAME, LOG_EVENT_FOUND_AUTORUN.arg(addApp.name()));

            if(sError = enqueueAdditionalApp(addApp, game.platformName(), Task::Stage::Auxiliary); sError.isValid())
                return sError;
        }
    }

    // Enqueue game
    if(sError = enqueueGame(game, gameData, Task::Stage::Primary); sError.isValid())
        return sError;

    // Enqueue service shutdown
    mCore.enqueueShutdownTasks();

    return Qx::Error();
}

Qx::Error CPlay::handleEntry(const Fp::AddApp& addApp)
{
    mCore.logEvent(NAME, LOG_EVENT_ID_MATCH_ADDAPP.arg(addApp.name(),
                                                       addApp.parentId().toString(QUuid::WithoutBraces)));

    Qx::Error sError;
    Fp::Db* db = mCore.fpInstall().database();

    // Check if parent entry uses a data pack
    QUuid parentId = addApp.parentId();
    Fp::GameData parentGameData;
    if(Fp::DbError gdErr = db->getGameData(parentGameData, parentId); gdErr.isValid())
    {
        mCore.postError(NAME, gdErr);
        return gdErr;
    }
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
        mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);

        if(sError = mCore.enqueueDataPackTasks(parentGameData); sError.isValid())
            return sError;
    }

    // Get parent info to determine platform
    Fp::Db::EntryFilter parentFilter{.type = Fp::Db::EntryType::Primary, .id = parentId};

    Fp::Db::QueryBuffer parentResult;

    if(Fp::DbError pge = db->queryEntrys(parentResult, parentFilter); pge.isValid())
    {
        mCore.postError(NAME, pge);
        return pge;
    }

    // Advance result to only record
    parentResult.result.next();

    // Determine platform (don't bother building entire game object since only one value is needed)
    QString platformName = parentResult.result.value(Fp::Db::Table_Game::COL_PLATFORM_NAME).toString();

    // Enqueue
    mCore.setStatus(STATUS_PLAY, addApp.name());

    if(sError = enqueueAdditionalApp(addApp, platformName, Task::Stage::Primary); sError.isValid())
        return sError;

    // Enqueue service shutdown if needed
    if(needsServices)
        mCore.enqueueShutdownTasks();

    return Qx::Error();
}

Qx::Error CPlay::enqueueAdditionalApp(const Fp::AddApp& addApp, const QString& platform, Task::Stage taskStage)
{
    if(addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_MESSAGE)
    {
        TMessage* messageTask = new TMessage(&mCore);
        messageTask->setStage(taskStage);
        messageTask->setText(addApp.launchCommand());
        messageTask->setBlocking(addApp.isWaitExit());

        mCore.enqueueSingleTask(messageTask);
    }
    else if(addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
    {
        TExtra* extraTask = new TExtra(&mCore);
        extraTask->setStage(taskStage);
        extraTask->setDirectory(QDir(mCore.fpInstall().extrasDirectory().absolutePath() + '/' + addApp.launchCommand()));

        mCore.enqueueSingleTask(extraTask);
    }
    else
    {
        QString addAppPath = mCore.resolveFullAppPath(addApp.appPath(), platform);
        QFileInfo addAppPathInfo(addAppPath);

        TExec* addAppTask = new TExec(&mCore);
        addAppTask->setIdentifier(addApp.name());
        addAppTask->setStage(taskStage);
        addAppTask->setExecutable(addAppPathInfo.canonicalFilePath());
        addAppTask->setDirectory(addAppPathInfo.absoluteDir());
        addAppTask->setParameters(addApp.launchCommand());
        addAppTask->setEnvironment(mCore.childTitleProcessEnvironment());
        addAppTask->setProcessType(addApp.isWaitExit() || taskStage == Task::Stage::Primary ? TExec::ProcessType::Blocking : TExec::ProcessType::Deferred);

        mCore.enqueueSingleTask(addAppTask);

#ifdef _WIN32
        // Add wait task if required
        if(Qx::Error ee = mCore.conditionallyEnqueueBideTask(addAppPathInfo); ee.isValid())
            return ee;
#endif
    }

    // Return success
    return Qx::Error();
}

Qx::Error CPlay::enqueueGame(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage)
{
    QString gamePath = mCore.resolveFullAppPath(!gameData.isNull() ? gameData.appPath() : game.appPath(),
                                                game.platformName());
    QFileInfo gamePathInfo(gamePath);

    TExec* gameTask = new TExec(&mCore);
    gameTask->setIdentifier(game.title());
    gameTask->setStage(taskStage);
    gameTask->setExecutable(gamePathInfo.canonicalFilePath());
    gameTask->setDirectory(gamePathInfo.absoluteDir());
    gameTask->setParameters(!gameData.isNull() ? gameData.launchCommand() : game.launchCommand());
    gameTask->setEnvironment(mCore.childTitleProcessEnvironment());
    gameTask->setProcessType(TExec::ProcessType::Blocking);

    mCore.enqueueSingleTask(gameTask);
    mCore.setStatus(STATUS_PLAY, game.title());

#ifdef _WIN32
    // Add wait task if required
    if(Qx::Error ee = mCore.conditionallyEnqueueBideTask(gamePathInfo); ee.isValid())
        return ee;
#endif

    // Return success
    return Qx::Error();
}

//Protected:
QList<const QCommandLineOption*> CPlay::options() { return CL_OPTIONS_SPECIFIC + TitleCommand::options(); }
QString CPlay::name() { return NAME; }

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
            mCore.postError(NAME, err);
            return err;
        }

        titleId = QUuid(urlMatch.captured(u"id"_s));
    }
    else if(Qx::Error ide = getTitleId(titleId); ide.isValid())
        return ide;


    mCore.logEvent(NAME, LOG_EVENT_HANDLING_AUTO);

    // Get entry via ID
    Fp::Db* db = mCore.fpInstall().database();

    Fp::Entry entry;
    if(Fp::DbError eErr = db->getEntry(entry, titleId); eErr.isValid())
    {
        mCore.postError(NAME, eErr);
        return eErr;
    }

    // Handle entry
    return std::visit([this](auto arg) { return this->handleEntry(arg); }, entry);
}
