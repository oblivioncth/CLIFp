// Unit Include
#include "core.h"

// Qt Includes
#include <QApplication>

// Qx Includes
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "command/command.h"
#include "task/t-download.h"
#include "task/t-exec.h"
#include "task/t-extract.h"
#include "task/t-mount.h"
#include "task/t-sleep.h"
#ifdef _WIN32
    #include "task/t-bideprocess.h"
#endif
#ifdef __linux__
    #include "task/t-awaitdocker.h"
#endif
#include "utility.h"
#include "project_vars.h"

//===============================================================================================================
// CORE
//===============================================================================================================

//-Constructor-------------------------------------------------------------
Core::Core(QObject* parent) :
    QObject(parent),
    mCriticalErrorOccured(false),
    mStatusHeading("Initializing"),
    mStatusMessage("...")
{}

//-Instance Functions-------------------------------------------------------------
//Private:
bool Core::isActionableOptionSet(const QCommandLineParser& clParser) const
{
    QSet<const QCommandLineOption*>::const_iterator i;
    for(i = CL_OPTIONS_ACTIONABLE.constBegin(); i != CL_OPTIONS_ACTIONABLE.constEnd(); ++i)
        if(clParser.isSet(**i))
            return true;

    return false;
}

void Core::showHelp()
{
    // Help string
    static QString helpStr;

    // Update status
    setStatus(STATUS_DISPLAY, STATUS_DISPLAY_HELP);

    // One time setup
    if(helpStr.isNull())
    {
        // Help options
        QString optStr;
        for(const QCommandLineOption* clOption : CL_OPTIONS_ALL)
        {
            // Handle names
            QStringList dashedNames;
            for(const QString& name : qxAsConst(clOption->names()))
                dashedNames << ((name.length() > 1 ? "--" : "-") + name);

            // Add option
            optStr += HELP_OPT_TEMPL.arg(dashedNames.join(" | "), clOption->description());
        }

        // Help commands
        QString commandStr;
        for(const QString& command : qxAsConst(Command::registered()))
            commandStr += HELP_COMMAND_TEMPL.arg(command, Command::describe(command));

        // Complete string
        helpStr = HELP_TEMPL.arg(optStr, commandStr);
    }

    // Show help
    postMessage(helpStr);
}


void Core::showVersion()
{
    setStatus(STATUS_DISPLAY, STATUS_DISPLAY_VERSION);
    postMessage(CL_VERSION_MESSAGE);
}

ErrorCode Core::searchAndFilterEntity(QUuid& returnBuffer, QString name, bool exactName, QUuid parent)
{
    // Clear return buffer
    returnBuffer = QUuid();

    // Search database for title
    QSqlError searchError;
    Fp::Db::QueryBuffer searchResult;

    Fp::Db::EntryFilter filter{
        .type = parent.isNull() ? Fp::Db::EntryType::Primary : Fp::Db::EntryType::AddApp,
        .parent = parent,
        .name = name,
        .playableOnly = false,
        .exactName = exactName
    };

    if((searchError = mFlashpointInstall->database()->queryEntrys(searchResult, filter)).isValid())
    {
        postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCode::SQL_ERROR;
    }

    logEvent(NAME, LOG_EVENT_TITLE_ID_COUNT.arg(searchResult.size).arg(name));

    if(searchResult.size < 1)
    {
        postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_TITLE_NOT_FOUND));
        return ErrorCode::TITLE_NOT_FOUND;
    }
    else if(searchResult.size == 1)
    {
        // Advance result to only record
        searchResult.result.next();

        // Get ID
        QString idKey = searchResult.source == Fp::Db::Table_Game::NAME ?
                                               Fp::Db::Table_Game::COL_ID :
                                               Fp::Db::Table_Add_App::COL_ID;

        returnBuffer = QUuid(searchResult.result.value(idKey).toString());
        logEvent(NAME, LOG_EVENT_TITLE_ID_DETERMINED.arg(name, returnBuffer.toString(QUuid::WithoutBraces)));

        return ErrorCode::NO_ERR;
    }
    else if (searchResult.size > FIND_ENTRY_LIMIT)
    {
        postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_FIND_TOO_MANY_RESULTS));
        return ErrorCode::TOO_MANY_RESULTS;
    }
    else
    {
        logEvent(NAME, LOG_EVENT_TITLE_SEL_PROMNPT);
        QHash<QString, QUuid> idMap;
        QStringList idChoices;

        for(int i = 0; i < searchResult.size; ++i)
        {
            // Advance result to next record
            searchResult.result.next();

            // Get ID
            QString idKey = searchResult.source == Fp::Db::Table_Game::NAME ?
                                                   Fp::Db::Table_Game::COL_ID :
                                                   Fp::Db::Table_Add_App::COL_ID;

            QUuid id = QUuid(searchResult.result.value(idKey).toString());

            // Create choice string
            QString choice = searchResult.source == Fp::Db::Table_Game::NAME ?
                                                    MULTI_GAME_SEL_TEMP.arg(searchResult.result.value(Fp::Db::Table_Game::COL_PLATFORM).toString(),
                                                                            searchResult.result.value(Fp::Db::Table_Game::COL_TITLE).toString(),
                                                                            searchResult.result.value(Fp::Db::Table_Game::COL_DEVELOPER).toString(),
                                                                            id.toString(QUuid::WithoutBraces)) :
                                                    MULTI_ADD_APP_SEL_TEMP.arg(searchResult.result.value(Fp::Db::Table_Add_App::COL_NAME).toString(),
                                                                               searchResult.result.value(Fp::Db::Table_Add_App::COL_ID).toString());

            // Add to map and choice list
            idMap[choice] = id;
            idChoices.append(choice);
        }

        // Get user choice
        Core::ItemSelectionRequest isr{
            .caption = MULTI_TITLE_SEL_CAP,
            .label = MULTI_TITLE_SEL_LABEL,
            .items = idChoices
        };
        QString userChoice = requestItemSelection(isr);

        if(userChoice.isNull())
            logEvent(NAME, LOG_EVENT_TITLE_SEL_CANCELED);
        else
        {
            // Set return buffer
            returnBuffer = idMap.value(userChoice); // If user choice is null, this will
            logEvent(NAME, LOG_EVENT_TITLE_ID_DETERMINED.arg(name, returnBuffer.toString(QUuid::WithoutBraces)));
        }

        return ErrorCode::NO_ERR;
    }
}

//Public:
ErrorCode Core::initialize(QStringList& commandLine)
{
    // Setup CLI Parser
    QCommandLineParser clParser;
    clParser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    for(const QCommandLineOption* clOption : CL_OPTIONS_ALL)
        clParser.addOption(*clOption);

    // Parse
    bool validArgs = clParser.parse(commandLine);

    // Create global options string
    QString globalOptions;
    for(const QCommandLineOption* clOption : CL_OPTIONS_ALL)
    {
        if(clParser.isSet(*clOption))
        {
            // Add switch to interpreted string
            if(!globalOptions.isEmpty())
                globalOptions += " "; // Space after every switch except first one

            globalOptions += "--" + (*clOption).names().at(1); // Always use long name

            // Add value of switch if it takes one
            if(!(*clOption).valueName().isEmpty())
                globalOptions += R"(=")" + clParser.value(*clOption) + R"(")";
        }
    }
    if(globalOptions.isEmpty())
        globalOptions = LOG_NO_PARAMS;

    // Remove app name from command line string
    commandLine.removeFirst();

    // Create logger instance
    QString logPath = CLIFP_DIR_PATH + '/' + CLIFP_CUR_APP_BASENAME  + '.' + LOG_FILE_EXT;
    mLogger = std::make_unique<Qx::ApplicationLogger>(logPath);
    mLogger->setApplicationName(PROJECT_SHORT_NAME);
    mLogger->setApplicationVersion(PROJECT_VERSION_STR);
    mLogger->setApplicationArguments(commandLine);
    mLogger->setMaximumEntries(LOG_MAX_ENTRIES);

    // Open log
    Qx::IoOpReport logOpen = mLogger->openLog();
    if(logOpen.isFailure())
        postError(NAME, Qx::GenericError(Qx::GenericError::Warning, logOpen.outcome(), logOpen.outcomeInfo()), false);

    // Log initialization step
    logEvent(NAME, LOG_EVENT_INIT);

    // Log global options
    logEvent(NAME, LOG_EVENT_GLOBAL_OPT.arg(globalOptions));

    // Check for valid arguments
    if(validArgs)
    {
        // Handle each global option
        mNotificationVerbosity = clParser.isSet(CL_OPTION_SILENT) ? NotificationVerbosity::Silent :
                                 clParser.isSet(CL_OPTION_QUIET) ? NotificationVerbosity::Quiet : NotificationVerbosity::Full;
        logEvent(NAME, LOG_EVENT_NOTIFCATION_LEVEL.arg(ENUM_NAME(mNotificationVerbosity)));

        if(clParser.isSet(CL_OPTION_VERSION))
        {
            showVersion();
            commandLine.clear(); // Clear args so application terminates after Core setup
            logEvent(NAME, LOG_EVENT_VER_SHOWN);
        }
        else if(clParser.isSet(CL_OPTION_HELP) || (!isActionableOptionSet(clParser) && clParser.positionalArguments().count() == 0)) // Also when no parameters
        {
            showHelp();
            commandLine.clear(); // Clear args so application terminates after Core setup
            logEvent(NAME, LOG_EVENT_G_HELP_SHOWN);
        }
        else
            commandLine = clParser.positionalArguments(); // Remove core options from command line list

        // Return success
        return ErrorCode::NO_ERR;
    }
    else
    {
        commandLine.clear(); // Clear remaining options since they are now irrelevant
        showHelp();
        logError(NAME, Qx::GenericError(Qx::GenericError::Error, LOG_ERR_INVALID_PARAM, clParser.errorText()));
        return ErrorCode::INVALID_ARGS;
    }

}

void Core::attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall)
{
    // Capture install
    mFlashpointInstall = std::move(flashpointInstall);

    // Initialize child process env var
    mChildTitleProcEnv = QProcessEnvironment::systemEnvironment();

    // Add FP root var
    mChildTitleProcEnv.insert("FP_PATH", mFlashpointInstall->fullPath());

#ifdef __linux__
    // Add HTTTP proxy var
    QString baseProxy = mFlashpointInstall->preferences().browserModeProxy;
    if(!baseProxy.isEmpty())
    {
        QString fullProxy = "http://" + baseProxy + '/';
        mChildTitleProcEnv.insert("http_proxy", fullProxy);
        mChildTitleProcEnv.insert("HTTP_PROXY", fullProxy);
    }

    /* NOTE: It's called "browserModeProxy" but it seems to be used everywhere and not just
     * for the vanilla launcher's browser mode. The launcher comments refer to it being created
     * in "BackgroundServices.ts" but that file doesnt exist so it's either a leftover (hopefully)
     * or a future feature (hopefully not). Either way it just points to the PHP server port.
     */
#endif
}

// TODO: Might make sense to make this a function in libfp
QString Core::resolveTrueAppPath(const QString& appPath, const QString& platform)
{
    // If appPath is absolute, convert it to relative temporarily
    QString workingPath = appPath;

    QString fpPath = mFlashpointInstall->fullPath();
    bool isFpAbsolute = workingPath.startsWith(fpPath);
    if(isFpAbsolute)
    {
        // Remove FP root and separator
        workingPath.remove(fpPath);
        if(!workingPath.isEmpty() && workingPath.front() == '/' || workingPath.front() == '\\')
            workingPath = workingPath.mid(1);
    }

    /* TODO: If this is made into a libfp function, isolate this part of it so it stays here,
     * or add a map argument to the libfp function that allows for passing custom "swaps"
     * and then call it with one for the following switch.
     *
     * CLIFp doesn't support the Launcher's built in browser (obviously), so manually
     * override it with Basilisk. Basilisk was removed in FP11 but the app path overrides
     * contains an entry for it that's appropriate on both platforms.
     */
    if(workingPath == ":browser-mode:")
        workingPath = "FPSoftware\\Basilisk-Portable\\Basilisk-Portable.exe";

    // Resolve both swap types
    workingPath = mFlashpointInstall->resolveExecSwaps(workingPath, platform);
    workingPath = mFlashpointInstall->resolveAppPathOverrides(workingPath);

    // Rebuild full path if applicable
    if(isFpAbsolute)
        workingPath = fpPath + '/' + workingPath;

    if(workingPath != appPath)
        logEvent(NAME, LOG_EVENT_APP_PATH_ALT.arg(appPath, workingPath));

    // Convert Windows separators to universal '/'
    return workingPath.replace('\\','/');
}

ErrorCode Core::findGameIdFromTitle(QUuid& returnBuffer, QString title, bool exactTitle)
{
    logEvent(NAME, LOG_EVENT_GAME_SEARCH.arg(title));
    return searchAndFilterEntity(returnBuffer, title, exactTitle);
}

ErrorCode Core::findAddAppIdFromName(QUuid& returnBuffer, QUuid parent, QString name, bool exactName)
{
    logEvent(NAME, LOG_EVENT_ADD_APP_SEARCH.arg(name, parent.toString()));
    return searchAndFilterEntity(returnBuffer, name, exactName, parent);
}

ErrorCode Core::enqueueStartupTasks()
{
    logEvent(NAME, LOG_EVENT_ENQ_START);

#ifdef __linux__
    /* On Linux X11 Server needs to be temporarily be set to allow connections from root for docker
     *
     * TODO: It should be OK to skip this (and the corresponding shutdown task that reverses it),
     * if docker isn't used, but leaving for now.
     */
    TExec* xhostSet = new TExec(this);
    xhostSet->setIdentifier("xhost Set");
    xhostSet->setStage(Task::Stage::Startup);
    xhostSet->setExecutable("xhost");
    xhostSet->setDirectory(mFlashpointInstall->fullPath());
    xhostSet->setParameters({"+SI:localuser:root"});
    xhostSet->setProcessType(TExec::ProcessType::Blocking);

    mTaskQueue.push(xhostSet);
    logTask(NAME, xhostSet);
#endif

    // Get settings
    Fp::Json::Services fpServices = mFlashpointInstall->services();
    Fp::Json::Config fpConfig = mFlashpointInstall->config();

    // Add Start entries from services
    for(const Fp::Json::StartStop& startEntry : qAsConst(fpServices.starts))
    {
        TExec* currentTask = new TExec(this);
        currentTask->setIdentifier(startEntry.filename);
        currentTask->setStage(Task::Stage::Startup);
        currentTask->setExecutable(startEntry.filename);
        currentTask->setDirectory(mFlashpointInstall->fullPath() + '/' + startEntry.path);
        currentTask->setParameters(startEntry.arguments);
        currentTask->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(currentTask);
        logTask(NAME, currentTask);
    }

    // Add Server entry from services if applicable
    if(fpConfig.startServer)
    {
        if(!fpServices.servers.contains(fpConfig.server))
        {
            postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CONFIG_SERVER_MISSING));
            return ErrorCode::CONFIG_SERVER_MISSING;
        }

        Fp::Json::ServerDaemon configuredServer = fpServices.servers.value(fpConfig.server);

        TExec* serverTask = new TExec(this);
        serverTask->setIdentifier("Server");
        serverTask->setStage(Task::Stage::Startup);
        serverTask->setExecutable(configuredServer.filename);
        serverTask->setDirectory(mFlashpointInstall->fullPath() + '/' + configuredServer.path);
        serverTask->setParameters(configuredServer.arguments);
        serverTask->setProcessType(configuredServer.kill ? TExec::ProcessType::Deferred : TExec::ProcessType::Detached);

        mTaskQueue.push(serverTask);
        logTask(NAME, serverTask);
    }

    // Add Daemon entry from services
    QHash<QString, Fp::Json::ServerDaemon>::const_iterator daemonIt;
    for (daemonIt = fpServices.daemons.constBegin(); daemonIt != fpServices.daemons.constEnd(); ++daemonIt)
    {
        TExec* currentTask = new TExec(this);
        currentTask->setIdentifier("Daemon");
        currentTask->setStage(Task::Stage::Startup);
        currentTask->setExecutable(daemonIt.value().filename);
        currentTask->setDirectory(mFlashpointInstall->fullPath() + '/' + daemonIt.value().path);
        currentTask->setParameters(daemonIt.value().arguments);
        currentTask->setProcessType(daemonIt.value().kill ? TExec::ProcessType::Deferred : TExec::ProcessType::Detached);

        mTaskQueue.push(currentTask);
        logTask(NAME, currentTask);
    }

#ifdef __linux__
    // On Linux the startup tasks take a while so make sure the docker image is actually running before proceeding
    if(mFlashpointInstall->services().recognizedDaemons.testFlag(Fp::KnownDaemon::Docker))
    {
        TAwaitDocker* dockerWait = new TAwaitDocker(this);
        dockerWait->setStage(Task::Stage::Startup);
        // NOTE: Other than maybe picking it out of the 2nd argument of the stop docker StartStop, there's no clean way to get this name
        dockerWait->setImageName("gamezip");
        dockerWait->setTimeout(10000);

        mTaskQueue.push(dockerWait);
        logTask(NAME, dockerWait);
    }
#endif

    /* Make sure that all startup processes have fully initialized.
     *
     * This is especially important for docker, as the mount server inside seems to take an extra moment to initialize (gives
     * "Connection Closed" if a mount attempt is made right away).
     */
    TSleep* initDelay = new TSleep(this);
    initDelay->setStage(Task::Stage::Startup);
    initDelay->setDuration(1500); // NOTE: Might need to be made longer

    mTaskQueue.push(initDelay);
    logTask(NAME, initDelay);

    // Return success
    return ErrorCode::NO_ERR;
}

void Core::enqueueShutdownTasks()
{
    logEvent(NAME, LOG_EVENT_ENQ_STOP);
    // Add Stop entries from services
    for(const Fp::Json::StartStop& stopEntry : qxAsConst(mFlashpointInstall->services().stops))
    {
        TExec* shutdownTask = new TExec(this);
        shutdownTask->setIdentifier(stopEntry.filename);
        shutdownTask->setStage(Task::Stage::Shutdown);
        shutdownTask->setExecutable(stopEntry.filename);
        shutdownTask->setDirectory(mFlashpointInstall->fullPath() + '/' + stopEntry.path);
        shutdownTask->setParameters(stopEntry.arguments);
        shutdownTask->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(shutdownTask);
        logTask(NAME, shutdownTask);
    }

#ifdef __linux__
    // Undo xhost permissions modifications
    TExec* xhostClear = new TExec(this);
    xhostClear->setIdentifier("xhost Clear");
    xhostClear->setStage(Task::Stage::Shutdown);
    xhostClear->setExecutable("xhost");
    xhostClear->setDirectory(mFlashpointInstall->fullPath());
    xhostClear->setParameters({"-SI:localuser:root"});
    xhostClear->setProcessType(TExec::ProcessType::Blocking);

    mTaskQueue.push(xhostClear);
    logTask(NAME, xhostClear);
#endif
}

#ifdef _WIN32
ErrorCode Core::conditionallyEnqueueBideTask(QFileInfo precedingAppInfo)
{
    // Add wait for apps that involve secure player
    bool involvesSecurePlayer;
    Qx::GenericError securePlayerCheckError = Fp::Install::appInvolvesSecurePlayer(involvesSecurePlayer, precedingAppInfo);
    if(securePlayerCheckError.isValid())
    {
        postError(NAME, securePlayerCheckError);
        return ErrorCode::CANT_READ_BAT_FILE;
    }

    if(involvesSecurePlayer)
    {
        TBideProcess* waitTask = new TBideProcess(this);
        waitTask->setStage(Task::Stage::Auxiliary);
        waitTask->setProcessName(Fp::Install::SECURE_PLAYER_INFO.fileName());

        mTaskQueue.push(waitTask);
        logTask(NAME, waitTask);
    }

    // Return success
    return ErrorCode::NO_ERR;

    // Possible future waits...
}
#endif

ErrorCode Core::enqueueDataPackTasks(QUuid targetId)
{
    logEvent(NAME, LOG_EVENT_ENQ_DATA_PACK);

    // Get entry data
    QSqlError searchError;
    Fp::Db::QueryBuffer searchResult;

    // Get database
    Fp::Db* database = mFlashpointInstall->database();

    if((searchError = database->queryEntryDataById(searchResult, targetId)).isValid())
    {
        postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCode::SQL_ERROR;
    }

    // Advance result to only record
    searchResult.result.next();

    // Extract relevant data
    QString packDestFolderPath = mFlashpointInstall->fullPath() + "/" + mFlashpointInstall->preferences().dataPacksFolderPath;
    QString packFileName = searchResult.result.value(Fp::Db::Table_Game_Data::COL_PATH).toString();
    QString packSha256 = searchResult.result.value(Fp::Db::Table_Game_Data::COL_SHA256).toString();
    QString packParameters = searchResult.result.value(Fp::Db::Table_Game_Data::COL_PARAM).toString();
    QFile packFile(packDestFolderPath + "/" + packFileName);

    // Get current file checksum if it exists
    bool checksumMatches = false;

    if(packFile.exists())
    {
        Qx::IoOpReport checksumReport = Qx::fileMatchesChecksum(checksumMatches, packFile, packSha256, QCryptographicHash::Sha256);
        if(checksumReport.isFailure())
            logError(NAME, Qx::GenericError(Qx::GenericError::Error, checksumReport.outcome(), checksumReport.outcomeInfo()));

        if(checksumMatches)
            logEvent(NAME, LOG_EVENT_DATA_PACK_FOUND);
        else
            postError(NAME, Qx::GenericError(Qx::GenericError::Warning, WRN_EXIST_PACK_SUM_MISMATCH));
    }
    else
        logEvent(NAME, LOG_EVENT_DATA_PACK_MISS);

    // Enqueue pack download if it doesn't exist or is different than expected
    if(!packFile.exists() || !checksumMatches)
    {
        // Get Data Pack source info
        searchError = database->queryDataPackSource(searchResult);
        if(searchError.isValid())
        {
            postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
            return ErrorCode::SQL_ERROR;
        }

        // Advance result to only record (or first if there are more than one in future versions)
        searchResult.result.next();

        // Get Data Pack source base URL
        QString sourceBaseUrl = searchResult.result.value(Fp::Db::Table_Source::COL_BASE_URL).toString();

        // Get title specific Data Pack source info
        searchError = database->queryEntrySourceData(searchResult, packSha256);
        if(searchError.isValid())
        {
            postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
            return ErrorCode::SQL_ERROR;
        }

        // Advance result to only record
        searchResult.result.next();

        // Get title's Data Pack sub-URL (the replace here is because there once was an errant entry in DB using '\')
        QString packSubUrl = searchResult.result.value(Fp::Db::Table_Source_Data::COL_URL_PATH).toString().replace('\\','/');

        TDownload* downloadTask = new TDownload(this);
        downloadTask->setStage(Task::Stage::Auxiliary);
        downloadTask->setDestinationPath(packDestFolderPath);
        downloadTask->setDestinationFilename(packFileName);
        downloadTask->setTargetFile(sourceBaseUrl + packSubUrl);
        downloadTask->setSha256(packSha256);

        mTaskQueue.push(downloadTask);
        logTask(NAME, downloadTask);
    }

    // Enqueue pack mount or extract
    if(packParameters.contains("-extract"))
    {
        logEvent(NAME, LOG_EVENT_DATA_PACK_NEEDS_EXTRACT);

        TExtract* extractTask = new TExtract(this);
        extractTask->setStage(Task::Stage::Auxiliary);
        extractTask->setPackPath(packDestFolderPath + "/" + packFileName);
        extractTask->setPathInPack("content");
        extractTask->setDestinationPath(mFlashpointInstall->preferences().htdocsFolderPath);

        mTaskQueue.push(extractTask);
        logTask(NAME, extractTask);
    }
    else
    {
        logEvent(NAME, LOG_EVENT_DATA_PACK_NEEDS_MOUNT);

        // Determine if QEMU is involved
        bool qemuUsed = mFlashpointInstall->services().recognizedDaemons.testFlag(Fp::KnownDaemon::Qemu);

        // Create task
        TMount* mountTask = new TMount(this);
        mountTask->setStage(Task::Stage::Auxiliary);
        mountTask->setTitleId(targetId);
        mountTask->setPath(packDestFolderPath + "/" + packFileName);
        mountTask->setSkipQemu(!qemuUsed);

        mTaskQueue.push(mountTask);
        logTask(NAME, mountTask);
    }

    // Return success
    return ErrorCode::NO_ERR;
}

void Core::enqueueSingleTask(Task* task) { mTaskQueue.push(task); logTask(NAME, task); }
void Core::clearTaskQueue() { mTaskQueue = {}; }

void Core::logCommand(QString src, QString commandName)
{
    Qx::IoOpReport logReport = mLogger->recordGeneralEvent(src, COMMAND_LABEL.arg(commandName));
    if(logReport.isFailure())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.outcome(), logReport.outcomeInfo()), false);
}

void Core::logCommandOptions(QString src, QString commandOptions)
{
    Qx::IoOpReport logReport = mLogger->recordGeneralEvent(src, COMMAND_OPT_LABEL.arg(commandOptions));
    if(logReport.isFailure())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.outcome(), logReport.outcomeInfo()), false);
}

void Core::logError(QString src, Qx::GenericError error)
{
    Qx::IoOpReport logReport = mLogger->recordErrorEvent(src, error);

    if(logReport.isFailure())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.outcome(), logReport.outcomeInfo()), false);

    if(error.errorLevel() == Qx::GenericError::Critical)
        mCriticalErrorOccured = true;
}

void Core::logEvent(QString src, QString event)
{
    Qx::IoOpReport logReport = mLogger->recordGeneralEvent(src, event);
    if(logReport.isFailure())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.outcome(), logReport.outcomeInfo()), false);
}

void Core::logTask(QString src, const Task* task) { logEvent(src, LOG_EVENT_TASK_ENQ.arg(task->name(), task->members().join(", "))); }

ErrorCode Core::logFinish(QString src, ErrorCode exitCode)
{
    if(mCriticalErrorOccured)
        logEvent(src, LOG_ERR_CRITICAL);

    Qx::IoOpReport logReport = mLogger->finish(exitCode);
    if(logReport.isFailure())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.outcome(), logReport.outcomeInfo()), false);

    // Return exit code so main function can return with this one
    return exitCode;
}

void Core::postError(QString src, Qx::GenericError error, bool log)
{
    // Logging
    if(log)
        logError(src, error);

    // Show error if applicable
    if(mNotificationVerbosity == NotificationVerbosity::Full ||
       (mNotificationVerbosity == NotificationVerbosity::Quiet && error.errorLevel() == Qx::GenericError::Critical))
    {
        // Box error
        Error e;
        e.source = src;
        e.errorInfo = error;

        // Emit
        emit errorOccured(e);
    }
}

int Core::postBlockingError(QString src, Qx::GenericError error, bool log, QMessageBox::StandardButtons bs, QMessageBox::StandardButton def)
{
    // Logging
    if(log)
        logError(src, error);

    // Show error if applicable
    if(mNotificationVerbosity == NotificationVerbosity::Full ||
       (mNotificationVerbosity == NotificationVerbosity::Quiet && error.errorLevel() == Qx::GenericError::Critical))
    {
        // Box error
        BlockingError be;
        be.source = src;
        be.errorInfo = error;
        be.choices = bs;
        be.defaultChoice = def;

        // Response holder
        QSharedPointer<int> response = QSharedPointer<int>::create(def);

        // Emit and get response
        emit blockingErrorOccured(response, be);

        // Return response
        return *response;
    }
    else
        return def;
}

void Core::postMessage(QString msg) { emit message(msg); }

QString Core::requestSaveFilePath(const SaveFileRequest& request)
{
    // Response holder
    QSharedPointer<QString> file = QSharedPointer<QString>::create();

    // Emit and get response
    emit saveFileRequested(file, request);

    // Return response
    return *file;
}

QString Core::requestItemSelection(const ItemSelectionRequest& request)
{
    // Response holder
    QSharedPointer<QString> item = QSharedPointer<QString>::create();

    // Emit and get response
    emit itemSelectionRequested(item, request);

    // Return response
    return *item;
}

Fp::Install& Core::fpInstall() { return *mFlashpointInstall; }
const QProcessEnvironment& Core::childTitleProcessEnvironment() { return mChildTitleProcEnv; }
Core::NotificationVerbosity Core::notifcationVerbosity() const { return mNotificationVerbosity; }
size_t Core::taskCount() const { return mTaskQueue.size(); }
bool Core::hasTasks() const { return mTaskQueue.size() > 0; }
Task* Core::frontTask() { return mTaskQueue.front(); }
void Core::removeFrontTask() { mTaskQueue.pop(); }

QString Core::statusHeading() { return mStatusHeading; }
QString Core::statusMessage() { return mStatusMessage;}
void Core::setStatus(QString heading, QString message)
{
    mStatusHeading = heading;
    mStatusMessage = message;
    emit statusChanged(heading, message);
}