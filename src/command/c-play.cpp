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
    fpGb.wPlatform(gameResult.result.value(Fp::Db::Table_Game::COL_PLATFORM).toString());
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

    return fpGb.build();
}

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
    Fp::Db* database = mCore.fpInstall().database();

    // Find title
    searchError = database->queryEntryById(searchResult, targetID);
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
        Fp::Db::QueryBuffer parentResult;
        searchError = database->queryEntryById(parentResult, parentId);
        if(searchError.isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
            return ErrorCode::SQL_ERROR;
        }

        // Determine platform (don't bother building entire game object since only one value is needed)
        QString platform = parentResult.size != 1 ? "" : parentResult.result.value(Fp::Db::Table_Game::COL_PLATFORM).toString();

        // Enqueue
        enqueueError = enqueueAdditionalApp(addApp, platform, Task::Stage::Primary);
        mCore.setStatus(STATUS_PLAY, searchResult.result.value(Fp::Db::Table_Add_App::COL_NAME).toString());

        if(enqueueError)
            return enqueueError;
    }
    else if(searchResult.source == Fp::Db::Table_Game::NAME) // Get autorun additional apps if result is game
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
        QSqlError addAppSearchError;
        Fp::Db::QueryBuffer addAppSearchResult;

        addAppSearchError = database->queryEntryAddApps(addAppSearchResult, targetID);
        if(addAppSearchError.isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, addAppSearchError.text()));
            return ErrorCode::SQL_ERROR;
        }

        // Enqueue autorun before apps
        for(int i = 0; i < addAppSearchResult.size; i++)
        {
            // Go to next record
            addAppSearchResult.result.next();

            // Build
            Fp::AddApp addApp = buildAdditionalApp(addAppSearchResult);

            // Enqueue if autorun before
            if(addApp.isAutorunBefore())
            {
                mCore.logEvent(NAME, LOG_EVENT_FOUND_AUTORUN.arg(addApp.name()));
                enqueueError = enqueueAdditionalApp(addApp, game.platform(), Task::Stage::Auxiliary);
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
        throw std::runtime_error("Auto ID search result source must be 'game' or 'additional_app'");

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
        addAppTask->setStage(taskStage);
        addAppTask->setPath(fulladdAppPathInfo.absolutePath());
        addAppTask->setFilename(fulladdAppPathInfo.fileName());
        addAppTask->setParameters(addApp.launchCommand());
        addAppTask->setEnvironment(mCore.childTitleProcessEnvironment());
        addAppTask->setProcessType(addApp.isWaitExit() || taskStage == Task::Stage::Primary ? TExec::ProcessType::Blocking : TExec::ProcessType::Deferred);

        mCore.enqueueSingleTask(addAppTask);

#ifdef _WIN32
        // Add wait task if required
        ErrorCode enqueueError = mCore.enqueueConditionalWaitTask(fulladdAppPathInfo);
        if(enqueueError)
            return enqueueError;
#endif
    }

    // Return success
    return ErrorCode::NO_ERR;
}

ErrorCode CPlay::enqueueGame(const Fp::Game& game, Task::Stage taskStage)
{
    QString gamePath = mCore.resolveTrueAppPath(game.appPath(), game.platform());
    QFileInfo fullGamePathInfo(mCore.fpInstall().fullPath() + '/' + gamePath);

    TExec* gameTask = new TExec(&mCore);
    gameTask->setStage(taskStage);
    gameTask->setPath(fullGamePathInfo.absolutePath());
    gameTask->setFilename(fullGamePathInfo.fileName());
    gameTask->setParameters(game.launchCommand());
    gameTask->setEnvironment(mCore.childTitleProcessEnvironment());
    gameTask->setProcessType(TExec::ProcessType::Blocking);

    mCore.enqueueSingleTask(gameTask);
    mCore.setStatus(STATUS_PLAY, game.title());

#ifdef _WIN32
    // Add wait task if required
    ErrorCode enqueueError = mCore.enqueueConditionalWaitTask(fullGamePathInfo);
    if(enqueueError)
        return enqueueError;
#endif

    // Return success
    return ErrorCode::NO_ERR;
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
    Fp::Db* database = mCore.fpInstall().database();

    // Query all main games
    Fp::Db::QueryBuffer mainGameIDQuery;
    searchError = database->queryAllGameIds(mainGameIDQuery, lbFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCode::SQL_ERROR;
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
        return ErrorCode::SQL_ERROR;
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
    return ErrorCode::NO_ERR;
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
    Fp::Db* database = mCore.fpInstall().database();

    // Get main entry info
    Fp::Db::QueryBuffer mainGameQuery;
    searchError = database->queryEntryById(mainGameQuery, mainID);
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

    // Ensure selection is primary app
    if(mainGameQuery.source != Fp::Db::Table_Game::NAME)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_SQL_MISMATCH));
        return ErrorCode::SQL_MISMATCH;
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
            return ErrorCode::SQL_ERROR;
        }

        // Check if ID was found and that only one instance was found
        if(addAppQuerry.size == 0)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND));
            return ErrorCode::ID_NOT_FOUND;
        }
        else if(addAppQuerry.size > 1)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ID_DUPLICATE_ENTRY_P, ERR_ID_DUPLICATE_ENTRY_S));
            return ErrorCode::ID_DUPLICATE;
        }

        // Ensure selection is additional app
        if(addAppQuerry.source != Fp::Db::Table_Add_App::NAME)
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_SQL_MISMATCH));
            return ErrorCode::SQL_MISMATCH;
        }

        // Advance result to only record
        addAppQuerry.result.next();

        // Populate buffer with variant info
        infoFillTemplate = infoFillTemplate.arg(addAppQuerry.result.value(Fp::Db::Table_Add_App::COL_NAME).toString());
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
    QUuid titleID;
    QUuid secondaryID;

    if(mParser.isSet(CL_OPTION_ID))
    {
        if((titleID = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_INVALID));
            return ErrorCode::ID_NOT_VALID;
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
            return ErrorCode::RAND_FILTER_NOT_VALID;
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
        return ErrorCode::INVALID_ARGS;
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
    return ErrorCode::NO_ERR;
}
