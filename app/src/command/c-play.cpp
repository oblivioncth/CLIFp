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
CPlay::CPlay(Core& coreRef) : Command(coreRef) {}

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

Fp::Game CPlay::buildGame(const Fp::Db::QueryBuffer& gameResult)
{
    // Ensure query result is game
    assert(gameResult.source == Fp::Db::Table_Game::NAME);

    /* Form game from record
     *
     * Most of the  descriptive fields here are not required for this app, but include them anyway in case it's later
     * desired to use them for something like logging.
     */
    Fp::Game::Builder fpGb;
    fpGb.wId(gameResult.result.value(Fp::Db::Table_Game::COL_ID).toString());
    fpGb.wTitle(gameResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wSeries(gameResult.result.value(Fp::Db::Table_Game::COL_SERIES).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wDeveloper(gameResult.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wPublisher(gameResult.result.value(Fp::Db::Table_Game::COL_PUBLISHER).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wDateAdded(gameResult.result.value(Fp::Db::Table_Game::COL_DATE_ADDED).toString());
    fpGb.wDateModified(gameResult.result.value(Fp::Db::Table_Game::COL_DATE_MODIFIED).toString());
    fpGb.wBroken(gameResult.result.value(Fp::Db::Table_Game::COL_BROKEN).toString());
    fpGb.wPlayMode(gameResult.result.value(Fp::Db::Table_Game::COL_PLAY_MODE).toString());
    fpGb.wStatus(gameResult.result.value(Fp::Db::Table_Game::COL_STATUS).toString());
    fpGb.wNotes(gameResult.result.value(Fp::Db::Table_Game::COL_NOTES).toString());
    fpGb.wSource(gameResult.result.value(Fp::Db::Table_Game::COL_SOURCE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wAppPath(gameResult.result.value(Fp::Db::Table_Game::COL_APP_PATH).toString());
    fpGb.wLaunchCommand(gameResult.result.value(Fp::Db::Table_Game::COL_LAUNCH_COMMAND).toString());
    fpGb.wReleaseDate(gameResult.result.value(Fp::Db::Table_Game::COL_RELEASE_DATE).toString());
    fpGb.wVersion(gameResult.result.value(Fp::Db::Table_Game::COL_VERSION).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wOriginalDescription(gameResult.result.value(Fp::Db::Table_Game::COL_ORIGINAL_DESC).toString());
    fpGb.wLanguage(gameResult.result.value(Fp::Db::Table_Game::COL_LANGUAGE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wOrderTitle(gameResult.result.value(Fp::Db::Table_Game::COL_ORDER_TITLE).toString().remove(Qx::RegularExpression::LINE_BREAKS));
    fpGb.wLibrary(gameResult.result.value(Fp::Db::Table_Game::COL_LIBRARY).toString());
    fpGb.wPlatformName(gameResult.result.value(Fp::Db::Table_Game::COL_PLATFORM_NAME).toString());

    return fpGb.build();
}

//-Instance Functions-------------------------------------------------------------
//Private:
ErrorCode CPlay::enqueueAutomaticTasks(bool& wasStandalone, QUuid targetId)
{
    // Clear standalone check
    wasStandalone = false;

    mCore.logEvent(NAME, LOG_EVENT_ENQ_AUTO);
    // Search FP database for entry via ID
    QSqlError searchError;
    Fp::Db::QueryBuffer searchResult;
    ErrorCode enqueueError;

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Find title
    Fp::Db::EntryFilter mainFilter{.type = Fp::Db::EntryType::PrimaryThenAddApp, .id = targetId};

    searchError = database->queryEntrys(searchResult, mainFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCode::SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(searchResult.size == 0)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND));
        return ErrorCode::ID_NOT_FOUND;
    }
    else if(searchResult.size > 1)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ID_DUPLICATE_ENTRY_P, ERR_ID_DUPLICATE_ENTRY_S));
        return ErrorCode::ID_DUPLICATE;
    }

    // Advance result to only record
    searchResult.result.next();

    // Enqueue if result is additional app
    if(searchResult.source == Fp::Db::Table_Add_App::NAME)
    {
        // Build add app
        Fp::AddApp addApp = buildAdditionalApp(searchResult);

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
        QSqlError packCheckError;
        bool parentUsesDataPack;
        QUuid parentId = addApp.parentId();

        if(parentId.isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_PARENT_INVALID, addApp.id().toString()));
            return ErrorCode::PARENT_INVALID;
        }

        if((packCheckError = database->entryUsesDataPack(parentUsesDataPack, parentId)).isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, packCheckError.text()));
            return ErrorCode::SQL_ERROR;
        }

        if(parentUsesDataPack)
        {
            mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = mCore.enqueueDataPackTasks(parentId);

            if(enqueueError)
                return enqueueError;
        }

        // Get parent info to determine platform
        Fp::Db::EntryFilter parentFilter{.type = Fp::Db::EntryType::Primary, .id = parentId};

        Fp::Db::QueryBuffer parentResult;
        searchError = database->queryEntrys(parentResult, parentFilter);
        if(searchError.isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
            return ErrorCode::SQL_ERROR;
        }

        // Advance result to only record
        parentResult.result.next();

        // Determine platform (don't bother building entire game object since only one value is needed)
        QString platformName = parentResult.result.value(Fp::Db::Table_Game::COL_PLATFORM_NAME).toString();

        // Enqueue
        enqueueError = enqueueAdditionalApp(addApp, platformName, Task::Stage::Primary);
        mCore.setStatus(STATUS_PLAY, searchResult.result.value(Fp::Db::Table_Add_App::COL_NAME).toString());

        if(enqueueError)
            return enqueueError;
    }
    else if(searchResult.source == Fp::Db::Table_Game::NAME) // Get auto-run additional apps if result is game
    {
        // Build game
        Fp::Game game = buildGame(searchResult);

        mCore.logEvent(NAME, LOG_EVENT_ID_MATCH_TITLE.arg(game.title()));

        // Check if entry uses a data pack
        QSqlError packCheckError;
        bool entryUsesDataPack;

        if((packCheckError = database->entryUsesDataPack(entryUsesDataPack, game.id())).isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, packCheckError.text()));
            return ErrorCode::SQL_ERROR;
        }

        if(entryUsesDataPack)
        {
            mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = mCore.enqueueDataPackTasks(game.id());

            if(enqueueError)
                return enqueueError;
        }

        // Get game's additional apps
        Fp::Db::EntryFilter addAppFilter{.type = Fp::Db::EntryType::AddApp, .parent = targetId};

        QSqlError addAppSearchError;
        Fp::Db::QueryBuffer addAppSearchResult;

        addAppSearchError = database->queryEntrys(addAppSearchResult, addAppFilter);
        if(addAppSearchError.isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, addAppSearchError.text()));
            return ErrorCode::SQL_ERROR;
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
                enqueueError = enqueueAdditionalApp(addApp, game.platformName(), Task::Stage::Auxiliary);
                if(enqueueError)
                    return enqueueError;
            }
        }

        // Enqueue game
        enqueueError = enqueueGame(game, Task::Stage::Primary);
        if(enqueueError)
            return enqueueError;
    }
    else
        qFatal("Auto ID search result source must be 'game' or 'additional_app'");

    // Return success
    return ErrorCode::NO_ERR;
}

ErrorCode CPlay::enqueueAdditionalApp(const Fp::AddApp& addApp, const QString& platform, Task::Stage taskStage)
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
        ErrorCode enqueueError = mCore.conditionallyEnqueueBideTask(fulladdAppPathInfo);
        if(enqueueError)
            return enqueueError;
#endif
    }

    // Return success
    return ErrorCode::NO_ERR;
}

ErrorCode CPlay::enqueueGame(const Fp::Game& game, Task::Stage taskStage)
{
    QString gamePath = mCore.resolveTrueAppPath(game.appPath(), game.platformName());
    QFileInfo fullGamePathInfo(mCore.fpInstall().fullPath() + '/' + gamePath);

    TExec* gameTask = new TExec(&mCore);
    gameTask->setIdentifier(game.title());
    gameTask->setStage(taskStage);
    gameTask->setExecutable(fullGamePathInfo.canonicalFilePath());
    gameTask->setDirectory(fullGamePathInfo.absoluteDir());
    gameTask->setParameters(game.launchCommand());
    gameTask->setEnvironment(mCore.childTitleProcessEnvironment());
    gameTask->setProcessType(TExec::ProcessType::Blocking);

    mCore.enqueueSingleTask(gameTask);
    mCore.setStatus(STATUS_PLAY, game.title());

#ifdef _WIN32
    // Add wait task if required
    ErrorCode enqueueError = mCore.conditionallyEnqueueBideTask(fullGamePathInfo);
    if(enqueueError)
        return enqueueError;
#endif

    // Return success
    return ErrorCode::NO_ERR;
}

ErrorCode CPlay::randomlySelectId(QUuid& mainIdBuffer, QUuid& subIdBuffer, Fp::Db::LibraryFilter lbFilter)
{
    mCore.logEvent(NAME, LOG_EVENT_SEL_RAND);

    // Reset buffers
    mainIdBuffer = QUuid();
    subIdBuffer = QUuid();

    // SQL Error tracker
    QSqlError searchError;

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Query all main games
    Fp::Db::QueryBuffer mainGameIdQuery;
    searchError = database->queryAllGameIds(mainGameIdQuery, lbFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCode::SQL_ERROR;
    }

    QVector<QUuid> playableIds;

    // Enumerate main game IDs
    for(int i = 0; i < mainGameIdQuery.size; i++)
    {
        // Go to next record
        mainGameIdQuery.result.next();

        // Add ID to list
        QString gameIdString = mainGameIdQuery.result.value(Fp::Db::Table_Game::COL_ID).toString();
        QUuid gameId = QUuid(gameIdString);
        if(!gameId.isNull())
            playableIds.append(gameId);
        else
            mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(gameIdString)));
    }
    mCore.logEvent(NAME, LOG_EVENT_PLAYABLE_COUNT.arg(QLocale(QLocale::system()).toString(playableIds.size())));

    // Select main game
    mainIdBuffer = playableIds.value(QRandomGenerator::global()->bounded(playableIds.size()));
    mCore.logEvent(NAME, LOG_EVENT_INIT_RAND_ID.arg(mainIdBuffer.toString(QUuid::WithoutBraces)));

    // Get entry's playable additional apps
    Fp::Db::EntryFilter addAppFilter{.type = Fp::Db::EntryType::AddApp, .parent = mainIdBuffer, .playableOnly = true};

    Fp::Db::QueryBuffer addAppQuery;
    searchError = database->queryEntrys(addAppQuery, addAppFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCode::SQL_ERROR;
    }
    mCore.logEvent(NAME, LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT.arg(addAppQuery.size));

    QVector<QUuid> playableSubIds;

    // Enumerate entry's playable additional apps
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Go to next record
        addAppQuery.result.next();

        // Add ID to list
        QString addAppIdString = addAppQuery.result.value(Fp::Db::Table_Game::COL_ID).toString();
        QUuid addAppId = QUuid(addAppIdString);
        if(!addAppId.isNull())
            playableSubIds.append(addAppId);
        else
            mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(addAppIdString)));
    }

    // Select final ID
    int randIndex = QRandomGenerator::global()->bounded(playableSubIds.size() + 1);

    if(randIndex == 0)
        mCore.logEvent(NAME, LOG_EVENT_RAND_DET_PRIM);
    else
    {
        subIdBuffer = playableSubIds.value(randIndex - 1);
        mCore.logEvent(NAME, LOG_EVENT_RAND_DET_ADD_APP.arg(subIdBuffer.toString(QUuid::WithoutBraces)));
    }

    // Return success
    return ErrorCode::NO_ERR;
}

ErrorCode CPlay::getRandomSelectionInfo(QString& infoBuffer, QUuid mainId, QUuid subId)
{
    mCore.logEvent(NAME, LOG_EVENT_RAND_GET_INFO);

    // Reset buffer
    infoBuffer = QString();

    // Template to fill
    QString infoFillTemplate = RAND_SEL_INFO;

    // SQL Error tracker
    QSqlError searchError;

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Get main entry info
    Fp::Db::EntryFilter mainFilter{.type = Fp::Db::EntryType::Primary, .id = mainId};

    Fp::Db::QueryBuffer mainGameQuery;
    searchError = database->queryEntrys(mainGameQuery, mainFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCode::SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(mainGameQuery.size == 0)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND));
        return ErrorCode::ID_NOT_FOUND;
    }
    else if(mainGameQuery.size > 1)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ID_DUPLICATE_ENTRY_P, ERR_ID_DUPLICATE_ENTRY_S));
        return ErrorCode::ID_DUPLICATE;
    }

    // Advance result to only record
    mainGameQuery.result.next();

    // Populate buffer with primary info
    infoFillTemplate = infoFillTemplate.arg(mainGameQuery.result.value(Fp::Db::Table_Game::COL_TITLE).toString(),
                               mainGameQuery.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString(),
                               mainGameQuery.result.value(Fp::Db::Table_Game::COL_PUBLISHER).toString(),
                               mainGameQuery.result.value(Fp::Db::Table_Game::COL_LIBRARY).toString());

    // Determine variant
    if(subId.isNull())
        infoFillTemplate = infoFillTemplate.arg("N/A");
    else
    {
        // Get sub entry info
        Fp::Db::EntryFilter addAppFilter{.type = Fp::Db::EntryType::AddApp, .id = subId};

        Fp::Db::QueryBuffer addAppQuery;
        searchError = database->queryEntrys(addAppQuery, addAppFilter);
        if(searchError.isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
            return ErrorCode::SQL_ERROR;
        }

        // Check if ID was found and that only one instance was found
        if(addAppQuery.size == 0)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND));
            return ErrorCode::ID_NOT_FOUND;
        }
        else if(addAppQuery.size > 1)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ID_DUPLICATE_ENTRY_P, ERR_ID_DUPLICATE_ENTRY_S));
            return ErrorCode::ID_DUPLICATE;
        }

        // Advance result to only record
        addAppQuery.result.next();

        // Populate buffer with variant info
        infoFillTemplate = infoFillTemplate.arg(addAppQuery.result.value(Fp::Db::Table_Add_App::COL_NAME).toString());
    }

    // Set filled template to buffer
    infoBuffer = infoFillTemplate;

    // Return success
    return ErrorCode::NO_ERR;
}

//Protected:
const QList<const QCommandLineOption *> CPlay::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
const QString CPlay::name() { return NAME; }

//Public:
ErrorCode CPlay::process(const QStringList& commandLine)
{
    ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return ErrorCode::NO_ERR;

    // Get ID of title to start
    QUuid titleId;
    QUuid addAppId;

    if(mParser.isSet(CL_OPTION_ID))
    {
        if((titleId = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_INVALID));
            return ErrorCode::ID_NOT_VALID;
        }
    }
    else if(mParser.isSet(CL_OPTION_TITLE) || mParser.isSet(CL_OPTION_TITLE_STRICT))
    {
        // Check title
        bool titleStrict = mParser.isSet(CL_OPTION_TITLE_STRICT);
        QString title = titleStrict ? mParser.value(CL_OPTION_TITLE_STRICT) : mParser.value(CL_OPTION_TITLE);

        if((errorStatus = mCore.findGameIdFromTitle(titleId, title, titleStrict)))
            return errorStatus;

        // Bail if canceled
        if(titleId.isNull())
            return ErrorCode::NO_ERR;

        // Check subtitle
        if(mParser.isSet(CL_OPTION_SUBTITLE) || mParser.isSet(CL_OPTION_SUBTITLE_STRICT))
        {
            bool subtitleStrict = mParser.isSet(CL_OPTION_SUBTITLE_STRICT);
            QString subtitle = subtitleStrict ? mParser.value(CL_OPTION_SUBTITLE_STRICT) : mParser.value(CL_OPTION_SUBTITLE);

            if((errorStatus = mCore.findAddAppIdFromName(addAppId, titleId, subtitle, subtitleStrict)))
                return errorStatus;

            // Bail if canceled
            if(addAppId.isNull())
                return ErrorCode::NO_ERR;
        }
    }
    else if(mParser.isSet(CL_OPTION_RAND))
    {
        QString rawRandFilter = mParser.value(CL_OPTION_RAND);
        Fp::Db::LibraryFilter randFilter;

        // Check for valid filter
        if(RAND_ALL_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Db::LibraryFilter::Either;
        else if(RAND_GAME_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Db::LibraryFilter::Game;
        else if(RAND_ANIM_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Db::LibraryFilter::Anim;
        else
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_RAND_FILTER_INVALID));
            return ErrorCode::RAND_FILTER_NOT_VALID;
        }

        // Get ID
        if((errorStatus = randomlySelectId(titleId, addAppId, randFilter)))
            return errorStatus;

        // Show selection info
        if(mCore.notifcationVerbosity() == Core::NotificationVerbosity::Full)
        {
            QString selInfo;
            if((errorStatus = getRandomSelectionInfo(selInfo, titleId, addAppId)))
                return errorStatus;

            // Display selection info
            mCore.postMessage(selInfo);
        }
    }
    else
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_TITLE));
        return ErrorCode::INVALID_ARGS;
    }

    // Enqueue required tasks
    if((errorStatus = mCore.enqueueStartupTasks()))
        return errorStatus;

    bool standaloneTask;
    if((errorStatus = enqueueAutomaticTasks(standaloneTask, addAppId.isNull() ? titleId : addAppId)))
        return errorStatus;

    if(!standaloneTask)
        mCore.enqueueShutdownTasks();

    // Return success
    return ErrorCode::NO_ERR;
}
