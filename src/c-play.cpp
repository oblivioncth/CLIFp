#include "c-play.h"

#include <QApplication>

//===============================================================================================================
// CPLAY
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CPlay::CPlay(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Private:
Core::ErrorCode CPlay::enqueueAutomaticTasks(bool& wasStandalone, QUuid targetID)
{
    // Clear standalone check
    wasStandalone = false;

    mCore.logEvent(LOG_EVENT_ENQ_AUTO);
    // Search FP database for entry via ID
    QSqlError searchError;
    FP::Install::DBQueryBuffer searchResult;
    Core::ErrorCode enqueueError;

    searchError = mCore.getFlashpointInstall().queryEntryByID(searchResult, targetID);
    if(searchError.isValid())
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(searchResult.size == 0)
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_NOT_FOUND));
        return Core::ID_NOT_FOUND;
    }
    else if(searchResult.size > 1)
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_MORE_THAN_ONE_AUTO_P, ERR_MORE_THAN_ONE_AUTO_S));
        return Core::MORE_THAN_ONE_AUTO;
    }

    // Advance result to only record
    searchResult.result.next();

    // Enqueue if result is additional app
    if(searchResult.source == FP::Install::DBTable_Add_App::NAME)
    {
        mCore.logEvent(LOG_EVENT_ID_MATCH_ADDAPP.arg(searchResult.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString(),
                                                     searchResult.result.value(FP::Install::DBTable_Add_App::COL_PARENT_ID).toUuid().toString(QUuid::WithoutBraces)));

        // Clear queue if this entry is a message or extra
        QString appPath = searchResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
        if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE || appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
        {
            mCore.clearTaskQueue();
            mCore.logEvent(LOG_EVENT_QUEUE_CLEARED);
            wasStandalone = true;
        }

        // Check if parent entry uses a data pack
        QSqlError packCheckError;
        bool parentUsesDataPack;
        QUuid parentID = searchResult.result.value(FP::Install::DBTable_Add_App::COL_PARENT_ID).toString();

        if(parentID.isNull())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_PARENT_INVALID, searchResult.result.value(FP::Install::DBTable_Add_App::COL_ID).toString()));
            return Core::PARENT_INVALID;
        }

        if((packCheckError = mCore.getFlashpointInstall().entryUsesDataPack(parentUsesDataPack, parentID)).isValid())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, packCheckError.text()));
            return Core::SQL_ERROR;
        }

        if(parentUsesDataPack)
        {
            mCore.logEvent(LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = mCore.enqueueDataPackTasks(parentID);

            if(enqueueError)
                return enqueueError;
        }

        enqueueError = enqueueAdditionalApp(searchResult, Core::TaskStage::Primary);

        if(enqueueError)
            return enqueueError;
    }
    else if(searchResult.source == FP::Install::DBTable_Game::NAME) // Get autorun additional apps if result is game
    {
        mCore.logEvent(LOG_EVENT_ID_MATCH_TITLE.arg(searchResult.result.value(FP::Install::DBTable_Game::COL_TITLE).toString()));

        // Check if entry uses a data pack
        QSqlError packCheckError;
        bool entryUsesDataPack;
        QUuid entryId = searchResult.result.value(FP::Install::DBTable_Game::COL_ID).toString();

        if((packCheckError = mCore.getFlashpointInstall().entryUsesDataPack(entryUsesDataPack, entryId)).isValid())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, packCheckError.text()));
            return Core::SQL_ERROR;
        }

        if(entryUsesDataPack)
        {
            mCore.logEvent(LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = mCore.enqueueDataPackTasks(entryId);

            if(enqueueError)
                return enqueueError;
        }

        // Get game's additional apps
        QSqlError addAppSearchError;
        FP::Install::DBQueryBuffer addAppSearchResult;

        addAppSearchError = mCore.getFlashpointInstall().queryEntryAddApps(addAppSearchResult, targetID);
        if(addAppSearchError.isValid())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, addAppSearchError.text()));
            return Core::SQL_ERROR;
        }

        // Enqueue autorun before apps
        for(int i = 0; i < addAppSearchResult.size; i++)
        {
            // Go to next record
            addAppSearchResult.result.next();

            // Enqueue if autorun before
            if(addAppSearchResult.result.value(FP::Install::DBTable_Add_App::COL_AUTORUN).toInt() != 0)
            {
                mCore.logEvent(LOG_EVENT_FOUND_AUTORUN.arg(addAppSearchResult.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString()));
                enqueueError = enqueueAdditionalApp(addAppSearchResult, Core::TaskStage::Auxiliary);
                if(enqueueError)
                    return enqueueError;
            }
        }

        // Enqueue game
        QString gamePath = searchResult.result.value(FP::Install::DBTable_Game::COL_APP_PATH).toString();
        QString gameArgs = searchResult.result.value(FP::Install::DBTable_Game::COL_LAUNCH_COMMAND).toString();
        QFileInfo gameInfo(mCore.getFlashpointInstall().getPath() + '/' + gamePath);

        std::shared_ptr<Core::ExecTask> gameTask = std::make_shared<Core::ExecTask>();
        gameTask->stage = Core::TaskStage::Primary;
        gameTask->path = gameInfo.absolutePath();
        gameTask->filename = gameInfo.fileName();
        gameTask->param = QStringList();
        gameTask->nativeParam = gameArgs;
        gameTask->processType = Core::ProcessType::Blocking;

        mCore.enqueueSingleTask(gameTask);

        // Add wait task if required
        if((enqueueError = mCore.enqueueConditionalWaitTask(gameInfo)))
            return enqueueError;
    }
    else
        throw std::runtime_error("Auto ID search result source must be 'game' or 'additional_app'");

    // Return success
    return Core::NO_ERR;
}

Core::ErrorCode CPlay::enqueueAdditionalApp(FP::Install::DBQueryBuffer addAppResult, Core::TaskStage taskStage)
{
    // Ensure query result is additional app
    assert(addAppResult.source == FP::Install::DBTable_Add_App::NAME);

    QString appPath = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
    QString appArgs = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_LAUNCH_COMMAND).toString();
    bool waitForExit = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_WAIT_EXIT).toInt() != 0;

    if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE)
    {
        std::shared_ptr<Core::MessageTask> messageTask = std::make_shared<Core::MessageTask>();
        messageTask->stage = taskStage;
        messageTask->message = appArgs;
        messageTask->modal = waitForExit || taskStage == Core::TaskStage::Primary;

        mCore.enqueueSingleTask(messageTask);
    }
    else if(appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
    {
        std::shared_ptr<Core::ExtraTask> extraTask = std::make_shared<Core::ExtraTask>();
        extraTask->stage = taskStage;
        extraTask->dir = QDir(mCore.getFlashpointInstall().getExtrasDirectory().absolutePath() + "/" + appArgs);

        mCore.enqueueSingleTask(extraTask);
    }
    else
    {
        QFileInfo addAppInfo(mCore.getFlashpointInstall().getPath() + '/' + appPath);

        std::shared_ptr<Core::ExecTask> addAppTask = std::make_shared<Core::ExecTask>();
        addAppTask->stage = taskStage;
        addAppTask->path = addAppInfo.absolutePath();
        addAppTask->filename = addAppInfo.fileName();
        addAppTask->param = QStringList();
        addAppTask->nativeParam = appArgs;
        addAppTask->processType = (waitForExit || taskStage == Core::TaskStage::Primary) ? Core::ProcessType::Blocking : Core::ProcessType::Deferred;

        mCore.enqueueSingleTask(addAppTask);

        // Add wait task if required
        Core::ErrorCode enqueueError = mCore.enqueueConditionalWaitTask(addAppInfo);
        if(enqueueError)
            return enqueueError;
    }

    // Return success
    return Core::NO_ERR;
}

Core::ErrorCode CPlay::randomlySelectID(QUuid& mainIDBuffer, QUuid& subIDBuffer, FP::Install::LibraryFilter lbFilter)
{
    mCore.logEvent(LOG_EVENT_SEL_RAND);

    // Reset buffers
    mainIDBuffer = QUuid();
    subIDBuffer = QUuid();

    // SQL Error tracker
    QSqlError searchError;

    // Query all main games
    FP::Install::DBQueryBuffer mainGameIDQuery;
    searchError = mCore.getFlashpointInstall().queryAllGameIDs(mainGameIDQuery, lbFilter);
    if(searchError.isValid())
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::SQL_ERROR;
    }

    QList<QUuid> playableIDs;

    // Enumerate main game IDs
    for(int i = 0; i < mainGameIDQuery.size; i++)
    {
        // Go to next record
        mainGameIDQuery.result.next();

        // Add ID to list
        QString gameIDString = mainGameIDQuery.result.value(FP::Install::DBTable_Game::COL_ID).toString();
        QUuid gameID = QUuid(gameIDString);
        if(!gameID.isNull())
            playableIDs.append(gameID);
        else
            mCore.logError(Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(gameIDString)));
    }
    mCore.logEvent(LOG_EVENT_PLAYABLE_COUNT.arg(QLocale(QLocale::system()).toString(playableIDs.size())));

    // Select main game
    mainIDBuffer = playableIDs.value(QRandomGenerator::global()->bounded(playableIDs.size()));
    mCore.logEvent(LOG_EVENT_INIT_RAND_ID.arg(mainIDBuffer.toString(QUuid::WithoutBraces)));

    // Get entry's playable additional apps
    FP::Install::DBQueryBuffer addAppQuery;
    searchError = mCore.getFlashpointInstall().queryEntryAddApps(addAppQuery, mainIDBuffer, true);
    if(searchError.isValid())
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::SQL_ERROR;
    }
    mCore.logEvent(LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT.arg(addAppQuery.size));

    QList<QUuid> playableSubIDs;

    // Enumerate entry's playable additional apps
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Go to next record
        addAppQuery.result.next();

        // Add ID to list
        QString addAppIDString = addAppQuery.result.value(FP::Install::DBTable_Game::COL_ID).toString();
        QUuid addAppID = QUuid(addAppIDString);
        if(!addAppID.isNull())
            playableSubIDs.append(addAppID);
        else
            mCore.logError(Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(addAppIDString)));
    }

    // Select final ID
    int randIndex = QRandomGenerator::global()->bounded(playableSubIDs.size() + 1);

    if(randIndex == 0)
        mCore.logEvent(LOG_EVENT_RAND_DET_PRIM);
    else
    {
        subIDBuffer = playableSubIDs.value(randIndex - 1);
        mCore.logEvent(LOG_EVENT_RAND_DET_ADD_APP.arg(subIDBuffer.toString(QUuid::WithoutBraces)));
    }

    // Return success
    return Core::NO_ERR;
}

Core::ErrorCode CPlay::getRandomSelectionInfo(QString& infoBuffer, QUuid mainID, QUuid subID)
{
    mCore.logEvent(LOG_EVENT_RAND_GET_INFO);

    // Reset buffer
    infoBuffer = QString();

    // Template to fill
    QString infoFillTemplate = RAND_SEL_INFO;

    // SQL Error tracker
    QSqlError searchError;

    // Get main entry info
    FP::Install::DBQueryBuffer mainGameQuery;
    searchError = mCore.getFlashpointInstall().queryEntryByID(mainGameQuery, mainID);
    if(searchError.isValid())
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return Core::SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(mainGameQuery.size == 0)
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_NOT_FOUND));
        return Core::ID_NOT_FOUND;
    }
    else if(mainGameQuery.size > 1)
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_MORE_THAN_ONE_AUTO_P, ERR_MORE_THAN_ONE_AUTO_S));
        return Core::MORE_THAN_ONE_AUTO;
    }

    // Ensure selection is primary app
    if(mainGameQuery.source != FP::Install::DBTable_Game::NAME)
    {
        mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_SQL_MISMATCH));
        return Core::SQL_MISMATCH;
    }

    // Advance result to only record
    mainGameQuery.result.next();

    // Populate buffer with primary info
    infoFillTemplate = infoFillTemplate.arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_TITLE).toString())
                               .arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_DEVELOPER).toString())
                               .arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_PUBLISHER).toString())
                               .arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_LIBRARY).toString());

    // Determine variant
    if(subID.isNull())
        infoFillTemplate = infoFillTemplate.arg("N/A");
    else
    {
        // Get sub entry info
        FP::Install::DBQueryBuffer addAppQuerry;
        searchError = mCore.getFlashpointInstall().queryEntryByID(addAppQuerry, subID);
        if(searchError.isValid())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
            return Core::SQL_ERROR;
        }

        // Check if ID was found and that only one instance was found
        if(addAppQuerry.size == 0)
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_NOT_FOUND));
            return Core::ID_NOT_FOUND;
        }
        else if(addAppQuerry.size > 1)
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_MORE_THAN_ONE_AUTO_P, ERR_MORE_THAN_ONE_AUTO_S));
            return Core::MORE_THAN_ONE_AUTO;
        }

        // Ensure selection is additional app
        if(addAppQuerry.source != FP::Install::DBTable_Add_App::NAME)
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_SQL_MISMATCH));
            return Core::SQL_MISMATCH;
        }

        // Advance result to only record
        addAppQuerry.result.next();

        // Populate buffer with variant info
        infoFillTemplate = infoFillTemplate.arg(addAppQuerry.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString());
    }

    // Set filled template to buffer
    infoBuffer = infoFillTemplate;

    // Return success
    return Core::NO_ERR;
}

//Protected:
const QList<const QCommandLineOption *> CPlay::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
const QString CPlay::name() { return NAME; }

//Public:
Core::ErrorCode CPlay::process(const QStringList& commandLine)
{
    Core::ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return Core::NO_ERR;

    // Open database
    if((errorStatus = mCore.openAndVerifyProperDatabase()))
        return errorStatus;

    // Get ID of title to start
    QUuid titleID;
    QUuid secondaryID;

    if(mParser.isSet(CL_OPTION_ID))
    {
        if((titleID = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_INVALID));
            return Core::ID_NOT_VALID;
        }
    }
    else if(mParser.isSet(CL_OPTION_RAND))
    {
        QString rawRandFilter = mParser.value(CL_OPTION_RAND);
        FP::Install::LibraryFilter randFilter;

        // Check for valid filter
        if(RAND_ALL_FILTER_NAMES.contains(rawRandFilter))
            randFilter = FP::Install::LibraryFilter::Either;
        else if(RAND_GAME_FILTER_NAMES.contains(rawRandFilter))
            randFilter = FP::Install::LibraryFilter::Game;
        else if(RAND_ANIM_FILTER_NAMES.contains(rawRandFilter))
            randFilter = FP::Install::LibraryFilter::Anim;
        else
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_RAND_FILTER_INVALID));
            return Core::RAND_FILTER_NOT_VALID;
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
            QMessageBox::information(nullptr, QApplication::applicationName(), selInfo);
        }
    }
    else
    {
        mCore.logError(Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_TITLE));
        return Core::INVALID_ARGS;
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
    return Core::NO_ERR;
}
