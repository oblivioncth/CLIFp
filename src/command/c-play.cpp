// Unit Include
#include "c-play.h"

// Qt Includes
#include <QApplication>

/* TODO: Allow this command to launch additional apps by their title,
 * likely by a second switch provided in addition to '-t' that checks
 * for that Add App title under the main title. Could allow it to also
 * be used with '-i' when the ID passed is a main title, but this would
 * require checking for that and it may be best to only allow it with
 * '-t' since if '-i' is being used the ID of the Add App can be given
 * directly anyway.
 */

//===============================================================================================================
// CPLAY
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CPlay::CPlay(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Private:
ErrorCode CPlay::enqueueAutomaticTasks(bool& wasStandalone, QUuid targetID)
{
    // Clear standalone check
    wasStandalone = false;

    mCore.logEvent(NAME, LOG_EVENT_ENQ_AUTO);
    // Search FP database for entry via ID
    QSqlError searchError;
    Fp::Db::QueryBuffer searchResult;
    ErrorCode enqueueError;

    // Get database
    Fp::Db* database = mCore.getFlashpointInstall().database();

    searchError = database->queryEntryById(searchResult, targetID);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::ErrorCodes::SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(searchResult.size == 0)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND));
        return Core::ErrorCodes::ID_NOT_FOUND;
    }
    else if(searchResult.size > 1)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ID_DUPLICATE_ENTRY_P, ERR_ID_DUPLICATE_ENTRY_S));
        return Core::ErrorCodes::ID_DUPLICATE;
    }

    // Advance result to only record
    searchResult.result.next();

    // Enqueue if result is additional app
    if(searchResult.source == Fp::Db::Table_Add_App::NAME)
    {
        mCore.logEvent(NAME, LOG_EVENT_ID_MATCH_ADDAPP.arg(searchResult.result.value(Fp::Db::Table_Add_App::COL_NAME).toString(),
                                                     searchResult.result.value(Fp::Db::Table_Add_App::COL_PARENT_ID).toUuid().toString(QUuid::WithoutBraces)));

        // Clear queue if this entry is a message or extra
        QString appPath = searchResult.result.value(Fp::Db::Table_Add_App::COL_APP_PATH).toString();
        if(appPath == Fp::Db::Table_Add_App::ENTRY_MESSAGE || appPath == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
        {
            mCore.clearTaskQueue();
            mCore.logEvent(NAME, LOG_EVENT_QUEUE_CLEARED);
            wasStandalone = true;
        }

        // Check if parent entry uses a data pack
        QSqlError packCheckError;
        bool parentUsesDataPack;
        QUuid parentId = QUuid::fromString(searchResult.result.value(Fp::Db::Table_Add_App::COL_PARENT_ID).toString());

        if(parentId.isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_PARENT_INVALID, searchResult.result.value(Fp::Db::Table_Add_App::COL_ID).toString()));
            return ErrorCodes::PARENT_INVALID;
        }

        if((packCheckError = database->entryUsesDataPack(parentUsesDataPack, parentId)).isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, packCheckError.text()));
            return Core::ErrorCodes::SQL_ERROR;
        }

        if(parentUsesDataPack)
        {
            mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = mCore.enqueueDataPackTasks(parentId);

            if(enqueueError)
                return enqueueError;
        }

        enqueueError = enqueueAdditionalApp(searchResult, Core::TaskStage::Primary);
        mCore.setStatus(STATUS_PLAY, searchResult.result.value(Fp::Db::Table_Add_App::COL_NAME).toString());

        if(enqueueError)
            return enqueueError;
    }
    else if(searchResult.source == Fp::Db::Table_Game::NAME) // Get autorun additional apps if result is game
    {
        mCore.logEvent(NAME, LOG_EVENT_ID_MATCH_TITLE.arg(searchResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString()));

        // Check if entry uses a data pack
        QSqlError packCheckError;
        bool entryUsesDataPack;
        QUuid entryId = QUuid::fromString(searchResult.result.value(Fp::Db::Table_Game::COL_ID).toString());

        if((packCheckError = database->entryUsesDataPack(entryUsesDataPack, entryId)).isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, packCheckError.text()));
            return Core::ErrorCodes::SQL_ERROR;
        }

        if(entryUsesDataPack)
        {
            mCore.logEvent(NAME, LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = mCore.enqueueDataPackTasks(entryId);

            if(enqueueError)
                return enqueueError;
        }

        // Get game's additional apps
        QSqlError addAppSearchError;
        Fp::Db::QueryBuffer addAppSearchResult;

        addAppSearchError = database->queryEntryAddApps(addAppSearchResult, targetID);
        if(addAppSearchError.isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, addAppSearchError.text()));
            return Core::ErrorCodes::SQL_ERROR;
        }

        // Enqueue autorun before apps
        for(int i = 0; i < addAppSearchResult.size; i++)
        {
            // Go to next record
            addAppSearchResult.result.next();

            // Enqueue if autorun before
            if(addAppSearchResult.result.value(Fp::Db::Table_Add_App::COL_AUTORUN).toInt() != 0)
            {
                mCore.logEvent(NAME, LOG_EVENT_FOUND_AUTORUN.arg(addAppSearchResult.result.value(Fp::Db::Table_Add_App::COL_NAME).toString()));
                enqueueError = enqueueAdditionalApp(addAppSearchResult, Core::TaskStage::Auxiliary);
                if(enqueueError)
                    return enqueueError;
            }
        }

        // Enqueue game
        QString gamePath = searchResult.result.value(Fp::Db::Table_Game::COL_APP_PATH).toString();
        QString gameArgs = searchResult.result.value(Fp::Db::Table_Game::COL_LAUNCH_COMMAND).toString();
        QFileInfo gameInfo(mCore.getFlashpointInstall().fullPath() + '/' + gamePath);

        std::shared_ptr<Core::ExecTask> gameTask = std::make_shared<Core::ExecTask>();
        gameTask->stage = Core::TaskStage::Primary;
        gameTask->path = gameInfo.absolutePath();
        gameTask->filename = gameInfo.fileName();
        gameTask->param = QStringList();
        gameTask->nativeParam = gameArgs;
        gameTask->processType = Core::ProcessType::Blocking;

        mCore.enqueueSingleTask(gameTask);
        mCore.setStatus(STATUS_PLAY, searchResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString());

        // Add wait task if required
        if((enqueueError = mCore.enqueueConditionalWaitTask(gameInfo)))
            return enqueueError;
    }
    else
        throw std::runtime_error("Auto ID search result source must be 'game' or 'additional_app'");

    // Return success
    return Core::ErrorCodes::NO_ERR;
}

ErrorCode CPlay::enqueueAdditionalApp(Fp::Db::QueryBuffer addAppResult, Core::TaskStage taskStage)
{
    // Ensure query result is additional app
    assert(addAppResult.source == Fp::Db::Table_Add_App::NAME);

    QString appPath = addAppResult.result.value(Fp::Db::Table_Add_App::COL_APP_PATH).toString();
    QString appArgs = addAppResult.result.value(Fp::Db::Table_Add_App::COL_LAUNCH_COMMAND).toString();
    bool waitForExit = addAppResult.result.value(Fp::Db::Table_Add_App::COL_WAIT_EXIT).toInt() != 0;

    if(appPath == Fp::Db::Table_Add_App::ENTRY_MESSAGE)
    {
        std::shared_ptr<Core::MessageTask> messageTask = std::make_shared<Core::MessageTask>();
        messageTask->stage = taskStage;
        messageTask->message = appArgs;
        messageTask->modal = waitForExit || taskStage == Core::TaskStage::Primary;

        mCore.enqueueSingleTask(messageTask);
    }
    else if(appPath == Fp::Db::Table_Add_App::ENTRY_EXTRAS)
    {
        std::shared_ptr<Core::ExtraTask> extraTask = std::make_shared<Core::ExtraTask>();
        extraTask->stage = taskStage;
        extraTask->dir = QDir(mCore.getFlashpointInstall().extrasDirectory().absolutePath() + "/" + appArgs);

        mCore.enqueueSingleTask(extraTask);
    }
    else
    {
        QFileInfo addAppInfo(mCore.getFlashpointInstall().fullPath() + '/' + appPath);

        std::shared_ptr<Core::ExecTask> addAppTask = std::make_shared<Core::ExecTask>();
        addAppTask->stage = taskStage;
        addAppTask->path = addAppInfo.absolutePath();
        addAppTask->filename = addAppInfo.fileName();
        addAppTask->param = QStringList();
        addAppTask->nativeParam = appArgs;
        addAppTask->processType = (waitForExit || taskStage == Core::TaskStage::Primary) ? Core::ProcessType::Blocking : Core::ProcessType::Deferred;

        mCore.enqueueSingleTask(addAppTask);

        // Add wait task if required
        ErrorCode enqueueError = mCore.enqueueConditionalWaitTask(addAppInfo);
        if(enqueueError)
            return enqueueError;
    }

    // Return success
    return Core::ErrorCodes::NO_ERR;
}

ErrorCode CPlay::randomlySelectID(QUuid& mainIDBuffer, QUuid& subIDBuffer, Fp::Db::LibraryFilter lbFilter)
{
    mCore.logEvent(NAME, LOG_EVENT_SEL_RAND);

    // Reset buffers
    mainIDBuffer = QUuid();
    subIDBuffer = QUuid();

    // SQL Error tracker
    QSqlError searchError;

    // Get database
    Fp::Db* database = mCore.getFlashpointInstall().database();

    // Query all main games
    Fp::Db::QueryBuffer mainGameIDQuery;
    searchError = database->queryAllGameIds(mainGameIDQuery, lbFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::ErrorCodes::SQL_ERROR;
    }

    QVector<QUuid> playableIDs;

    // Enumerate main game IDs
    for(int i = 0; i < mainGameIDQuery.size; i++)
    {
        // Go to next record
        mainGameIDQuery.result.next();

        // Add ID to list
        QString gameIDString = mainGameIDQuery.result.value(Fp::Db::Table_Game::COL_ID).toString();
        QUuid gameID = QUuid(gameIDString);
        if(!gameID.isNull())
            playableIDs.append(gameID);
        else
            mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(gameIDString)));
    }
    mCore.logEvent(NAME, LOG_EVENT_PLAYABLE_COUNT.arg(QLocale(QLocale::system()).toString(playableIDs.size())));

    // Select main game
    mainIDBuffer = playableIDs.value(QRandomGenerator::global()->bounded(playableIDs.size()));
    mCore.logEvent(NAME, LOG_EVENT_INIT_RAND_ID.arg(mainIDBuffer.toString(QUuid::WithoutBraces)));

    // Get entry's playable additional apps
    Fp::Db::QueryBuffer addAppQuery;
    searchError = database->queryEntryAddApps(addAppQuery, mainIDBuffer, true);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::ErrorCodes::SQL_ERROR;
    }
    mCore.logEvent(NAME, LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT.arg(addAppQuery.size));

    QVector<QUuid> playableSubIDs;

    // Enumerate entry's playable additional apps
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Go to next record
        addAppQuery.result.next();

        // Add ID to list
        QString addAppIDString = addAppQuery.result.value(Fp::Db::Table_Game::COL_ID).toString();
        QUuid addAppID = QUuid(addAppIDString);
        if(!addAppID.isNull())
            playableSubIDs.append(addAppID);
        else
            mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(addAppIDString)));
    }

    // Select final ID
    int randIndex = QRandomGenerator::global()->bounded(playableSubIDs.size() + 1);

    if(randIndex == 0)
        mCore.logEvent(NAME, LOG_EVENT_RAND_DET_PRIM);
    else
    {
        subIDBuffer = playableSubIDs.value(randIndex - 1);
        mCore.logEvent(NAME, LOG_EVENT_RAND_DET_ADD_APP.arg(subIDBuffer.toString(QUuid::WithoutBraces)));
    }

    // Return success
    return Core::ErrorCodes::NO_ERR;
}

ErrorCode CPlay::getRandomSelectionInfo(QString& infoBuffer, QUuid mainID, QUuid subID)
{
    mCore.logEvent(NAME, LOG_EVENT_RAND_GET_INFO);

    // Reset buffer
    infoBuffer = QString();

    // Template to fill
    QString infoFillTemplate = RAND_SEL_INFO;

    // SQL Error tracker
    QSqlError searchError;

    // Get database
    Fp::Db* database = mCore.getFlashpointInstall().database();

    // Get main entry info
    Fp::Db::QueryBuffer mainGameQuery;
    searchError = database->queryEntryById(mainGameQuery, mainID);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::ErrorCodes::SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(mainGameQuery.size == 0)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND));
        return Core::ErrorCodes::ID_NOT_FOUND;
    }
    else if(mainGameQuery.size > 1)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ID_DUPLICATE_ENTRY_P, ERR_ID_DUPLICATE_ENTRY_S));
        return Core::ErrorCodes::ID_DUPLICATE;
    }

    // Ensure selection is primary app
    if(mainGameQuery.source != Fp::Db::Table_Game::NAME)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_SQL_MISMATCH));
        return Core::ErrorCodes::SQL_MISMATCH;
    }

    // Advance result to only record
    mainGameQuery.result.next();

    // Populate buffer with primary info
    infoFillTemplate = infoFillTemplate.arg(mainGameQuery.result.value(Fp::Db::Table_Game::COL_TITLE).toString(),
                               mainGameQuery.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString(),
                               mainGameQuery.result.value(Fp::Db::Table_Game::COL_PUBLISHER).toString(),
                               mainGameQuery.result.value(Fp::Db::Table_Game::COL_LIBRARY).toString());

    // Determine variant
    if(subID.isNull())
        infoFillTemplate = infoFillTemplate.arg("N/A");
    else
    {
        // Get sub entry info
        Fp::Db::QueryBuffer addAppQuerry;
        searchError = database->queryEntryById(addAppQuerry, subID);
        if(searchError.isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
            return Core::ErrorCodes::SQL_ERROR;
        }

        // Check if ID was found and that only one instance was found
        if(addAppQuerry.size == 0)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND));
            return Core::ErrorCodes::ID_NOT_FOUND;
        }
        else if(addAppQuerry.size > 1)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ID_DUPLICATE_ENTRY_P, ERR_ID_DUPLICATE_ENTRY_S));
            return Core::ErrorCodes::ID_DUPLICATE;
        }

        // Ensure selection is additional app
        if(addAppQuerry.source != Fp::Db::Table_Add_App::NAME)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_SQL_MISMATCH));
            return Core::ErrorCodes::SQL_MISMATCH;
        }

        // Advance result to only record
        addAppQuerry.result.next();

        // Populate buffer with variant info
        infoFillTemplate = infoFillTemplate.arg(addAppQuerry.result.value(Fp::Db::Table_Add_App::COL_NAME).toString());
    }

    // Set filled template to buffer
    infoBuffer = infoFillTemplate;

    // Return success
    return Core::ErrorCodes::NO_ERR;
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
        return Core::ErrorCodes::NO_ERR;

    // Get ID of title to start
    QUuid titleID;
    QUuid secondaryID;

    if(mParser.isSet(CL_OPTION_ID))
    {
        if((titleID = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_INVALID));
            return Core::ErrorCodes::ID_NOT_VALID;
        }
    }
    else if(mParser.isSet(CL_OPTION_TITLE))
    {
        if((errorStatus = mCore.getGameIDFromTitle(titleID, mParser.value(CL_OPTION_TITLE))))
            return errorStatus;
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
            return ErrorCodes::RAND_FILTER_NOT_VALID;
        }

        // Get ID
        if((errorStatus = randomlySelectID(titleID, secondaryID, randFilter)))
            return errorStatus;

        // Show selection info
        if(mCore.notifcationVerbosity() == Core::NotificationVerbosity::Full)
        {
            QString selInfo;
            if((errorStatus = getRandomSelectionInfo(selInfo, titleID, secondaryID)))
                return errorStatus;

            // Display selection info
            mCore.postMessage(selInfo);
        }
    }
    else
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_TITLE));
        return Core::ErrorCodes::INVALID_ARGS;
    }

    // Enqueue required tasks
    if((errorStatus = mCore.enqueueStartupTasks()))
        return errorStatus;

    bool standaloneTask;
    if((errorStatus = enqueueAutomaticTasks(standaloneTask, secondaryID.isNull() ? titleID : secondaryID)))
        return errorStatus;

    if(!standaloneTask)
        mCore.enqueueShutdownTasks();

    // Return success
    return Core::ErrorCodes::NO_ERR;
}
