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
Qx::Error CPlay::enqueueAutomaticTasks(bool& wasStandalone, QUuid targetId)
{
    // Clear standalone check
    wasStandalone = false;

    mCore.logEvent(NAME, LOG_EVENT_ENQ_AUTO);

    // Get entry via ID
    Fp::Db* database = mCore.fpInstall().database();

    std::variant<Fp::Game, Fp::AddApp> entry;
    if(Fp::DbError eErr = database->getEntry(entry, targetId); eErr.isValid())
    {
        mCore.postError(NAME, eErr);
        return eErr;
    }

    // Enqueue if result is additional app
    if(std::holds_alternative<Fp::AddApp>(entry))
    {
        // Get add app
        const Fp::AddApp& addApp = std::get<Fp::AddApp>(entry);

        mCore.logEvent(NAME, LOG_EVENT_ID_MATCH_ADDAPP.arg(addApp.name(),
                                                           addApp.parentId().toString(QUuid::WithoutBraces)));

        // Clear queue if this entry is a message or extra
        if(addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_MESSAGE || addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
        {
            mCore.clearTaskQueue();
            mCore.logEvent(NAME, LOG_EVENT_QUEUE_CLEARED);
            wasStandalone = true;
        }

        // Check if parent entry uses a data pack
        QUuid parentId = addApp.parentId();
        Fp::GameData parentGameData;
        if(Fp::DbError gdErr = database->getGameData(parentGameData, parentId); gdErr.isValid())
        {
            mCore.postError(NAME, gdErr);
            return gdErr;
        }

        if(!parentGameData.isNull())
        {
            mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);

            if(Qx::Error ee = mCore.enqueueDataPackTasks(parentGameData); ee.isValid())
                return ee;
        }

        // Get parent info to determine platform
        Fp::Db::EntryFilter parentFilter{.type = Fp::Db::EntryType::Primary, .id = parentId};

        Fp::Db::QueryBuffer parentResult;

        if(Fp::DbError pge = database->queryEntrys(parentResult, parentFilter); pge.isValid())
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

        if(Qx::Error ee = enqueueAdditionalApp(addApp, platformName, Task::Stage::Primary); ee.isValid())
            return ee;
    }
    else if(std::holds_alternative<Fp::Game>(entry)) // Get auto-run additional apps if result is game
    {
        // Get game
        const Fp::Game& game = std::get<Fp::Game>(entry);

        mCore.logEvent(NAME, LOG_EVENT_ID_MATCH_TITLE.arg(game.title()));

        // Get game data (if present)
        Fp::GameData gameData;
        if(Fp::DbError gdErr = database->getGameData(gameData, targetId); gdErr.isValid())
        {
            mCore.postError(NAME, gdErr);
            return gdErr;
        }

        // Check if entry uses a data pack
        if(!gameData.isNull())
        {
            mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);

            if(Qx::Error ee = mCore.enqueueDataPackTasks(gameData); ee.isValid())
                return ee;
        }

        // Get game's additional apps
        Fp::Db::EntryFilter addAppFilter{.type = Fp::Db::EntryType::AddApp, .parent = targetId};

        Fp::DbError addAppSearchError;
        Fp::Db::QueryBuffer addAppSearchResult;

        addAppSearchError = database->queryEntrys(addAppSearchResult, addAppFilter);
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

                if(Qx::Error ee = enqueueAdditionalApp(addApp, game.platformName(), Task::Stage::Auxiliary); ee.isValid())
                    return ee;
            }
        }

        // Enqueue game
        if(Qx::Error ee = enqueueGame(game, gameData, Task::Stage::Primary); ee.isValid())
            return ee;
    }
    else
        qFatal("Auto ID search result source must be 'game' or 'additional_app'");

    // Return success
    return Qx::Error();
}

Qx::Error CPlay::enqueueAdditionalApp(const Fp::AddApp& addApp, const QString& platform, Task::Stage taskStage)
{
    if(addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_MESSAGE)
    {
        TMessage* messageTask = new TMessage(&mCore);
        messageTask->setStage(taskStage);
        messageTask->setMessage(addApp.launchCommand());
        messageTask->setModal(addApp.isWaitExit() || taskStage == Task::Stage::Primary);

        mCore.enqueueSingleTask(messageTask);
    }
    else if(addApp.appPath() == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
    {
        TExtra* extraTask = new TExtra(&mCore);
        extraTask->setStage(taskStage);
        extraTask->setDirectory(QDir(mCore.fpInstall().extrasDirectory().absolutePath() + "/" + addApp.launchCommand()));

        mCore.enqueueSingleTask(extraTask);
    }
    else
    {
        QString addAppPath = mCore.resolveTrueAppPath(addApp.appPath(), platform);
        QFileInfo fulladdAppPathInfo(mCore.fpInstall().fullPath() + '/' + addAppPath);

        TExec* addAppTask = new TExec(&mCore);
        addAppTask->setIdentifier(addApp.name());
        addAppTask->setStage(taskStage);
        addAppTask->setExecutable(fulladdAppPathInfo.canonicalFilePath());
        addAppTask->setDirectory(fulladdAppPathInfo.absoluteDir());
        addAppTask->setParameters(addApp.launchCommand());
        addAppTask->setEnvironment(mCore.childTitleProcessEnvironment());
        addAppTask->setProcessType(addApp.isWaitExit() || taskStage == Task::Stage::Primary ? TExec::ProcessType::Blocking : TExec::ProcessType::Deferred);

        mCore.enqueueSingleTask(addAppTask);

#ifdef _WIN32
        // Add wait task if required
        if(Qx::Error ee = mCore.conditionallyEnqueueBideTask(fulladdAppPathInfo); ee.isValid())
            return ee;
#endif
    }

    // Return success
    return Qx::Error();
}

Qx::Error CPlay::enqueueGame(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage)
{
    QString gamePath = mCore.resolveTrueAppPath(!gameData.isNull() ? gameData.appPath() : game.appPath(),
                                                game.platformName());
    QFileInfo fullGamePathInfo(mCore.fpInstall().fullPath() + '/' + gamePath);

    TExec* gameTask = new TExec(&mCore);
    gameTask->setIdentifier(game.title());
    gameTask->setStage(taskStage);
    gameTask->setExecutable(fullGamePathInfo.canonicalFilePath());
    gameTask->setDirectory(fullGamePathInfo.absoluteDir());
    gameTask->setParameters(!gameData.isNull() ? gameData.launchCommand() : game.launchCommand());
    gameTask->setEnvironment(mCore.childTitleProcessEnvironment());
    gameTask->setProcessType(TExec::ProcessType::Blocking);

    mCore.enqueueSingleTask(gameTask);
    mCore.setStatus(STATUS_PLAY, game.title());

#ifdef _WIN32
    // Add wait task if required
    if(Qx::Error ee = mCore.conditionallyEnqueueBideTask(fullGamePathInfo); ee.isValid())
        return ee;
#endif

    // Return success
    return Qx::Error();
}

//Protected:
QString CPlay::name() { return NAME; }

Qx::Error CPlay::perform()
{
    // Get ID of title to start
    QUuid titleId;
    if(Qx::Error ide = getTitleId(titleId); ide.isValid())
        return ide;

    Qx::Error errorStatus;

    // Enqueue required tasks
    if((errorStatus = mCore.enqueueStartupTasks()).isValid())
        return errorStatus;

    bool standaloneTask;
    if((errorStatus = enqueueAutomaticTasks(standaloneTask, titleId)).isValid())
        return errorStatus;

    if(!standaloneTask)
        mCore.enqueueShutdownTasks();

    // Return success
    return Qx::Error();
}
