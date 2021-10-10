#include "core.h"
#include "command.h"

#include <QApplication>
#include <QInputDialog>

//===============================================================================================================
// CORE
//===============================================================================================================

//-Constructor-------------------------------------------------------------
Core::Core(QObject* parent, QString rawCommandLineParam) :
    QObject(parent),
    mRawCommandLine(rawCommandLineParam),
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
            for(const QString& name : qAsConstR(clOption->names()))
                dashedNames << ((name.length() > 1 ? "--" : "-") + name);

            // Add option
            optStr += HELP_OPT_TEMPL.arg(dashedNames.join(" | "), clOption->description());
        }

        // Help commands
        QString commandStr;
        for(const QString& command : qAsConstR(Command::registered()))
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
            // Add switch to interp string
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
    mLogFile = std::make_unique<QFile>(CLIFP_DIR_PATH + '/' + LOG_FILE_NAME);
    mLogger = std::make_unique<Logger>(mLogFile.get(), mRawCommandLine.isEmpty() ? LOG_NO_PARAMS : mRawCommandLine, globalOptions, LOG_HEADER, LOG_MAX_ENTRIES);

    // Open log
    Qx::IOOpReport logOpen = mLogger->openLog();
    if(!logOpen.wasSuccessful())
        postError(NAME, Qx::GenericError(Qx::GenericError::Warning, logOpen.getOutcome(), logOpen.getOutcomeInfo()), false);

    // Log initialization step
    logEvent(NAME, LOG_EVENT_INIT);

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
        return ErrorCodes::NO_ERR;
    }
    else
    {
        commandLine.clear(); // Clear remaining options since they are now irrelavent
        showHelp();
        logError(NAME, Qx::GenericError(Qx::GenericError::Error, LOG_ERR_INVALID_PARAM, clParser.errorText()));
        return ErrorCodes::INVALID_ARGS;
    }

}

void Core::attachFlashpoint(std::unique_ptr<FP::Install> flashpointInstall)
{
    // Capture install
    mFlashpointInstall = std::move(flashpointInstall);
}

ErrorCode Core::getGameIDFromTitle(QUuid& returnBuffer, QString title)
{
    // Clear return buffer
    returnBuffer = QUuid();

    // Search database for title
    QSqlError searchError;
    FP::DB::QueryBuffer searchResult;

    if((searchError = mFlashpointInstall->database()->queryEntriesByTitle(searchResult, title)).isValid())
    {
        postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCodes::SQL_ERROR;
    }

    logEvent(NAME, LOG_EVENT_TITLE_ID_COUNT.arg(searchResult.size).arg(title));

    if(searchResult.size < 1)
    {
        postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_TITLE_NOT_FOUND));
        return ErrorCodes::TITLE_NOT_FOUND;
    }
    else if(searchResult.size == 1)
    {
        // Advance result to only record
        searchResult.result.next();

        // Get ID
        returnBuffer = QUuid(searchResult.result.value(FP::DB::Table_Game::COL_ID).toString());
        logEvent(NAME, LOG_EVENT_TITLE_ID_DETERMINED.arg(title, returnBuffer.toString(QUuid::WithoutBraces)));

        return ErrorCodes::NO_ERR;
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
            QUuid id = QUuid(searchResult.result.value(FP::DB::Table_Game::COL_ID).toString());

            // Create choice string
            QString choice = MULTI_TITLE_SEL_TEMP.arg(searchResult.result.value(FP::DB::Table_Game::COL_PLATFORM).toString(),
                                                      title,
                                                      searchResult.result.value(FP::DB::Table_Game::COL_DEVELOPER).toString(),
                                                      id.toString(QUuid::WithoutBraces));

            // Add to map and choice list
            idMap[choice] = id;
            idChoices.append(choice);
        }

        // Get user choice
        QString userChoice = QInputDialog::getItem(nullptr, MULTI_TITLE_SEL_CAP, MULTI_TITLE_SEL_LABEL, idChoices);

        // Set return buffer
        returnBuffer = idMap.value(userChoice);
        logEvent(NAME, LOG_EVENT_TITLE_ID_DETERMINED.arg(title, returnBuffer.toString(QUuid::WithoutBraces)));

        return ErrorCodes::NO_ERR;
    }
}

ErrorCode Core::enqueueStartupTasks()
{
    logEvent(NAME, LOG_EVENT_ENQ_START);

    // Get settings
    FP::Json::Services fpServices = mFlashpointInstall->services();
    FP::Json::Config fpConfig = mFlashpointInstall->config();

    // Add Start entries from services
    for(const FP::Json::StartStop& startEntry : qAsConst(fpServices.starts))
    {
        std::shared_ptr<ExecTask> currentTask = std::make_shared<ExecTask>();
        currentTask->stage = TaskStage::Startup;
        currentTask->path = mFlashpointInstall->fullPath() + '/' + startEntry.path;
        currentTask->filename = startEntry.filename;
        currentTask->param = startEntry.arguments;
        currentTask->nativeParam = QString();
        currentTask->processType = ProcessType::Blocking;

        mTaskQueue.push(currentTask);
        logTask(NAME, currentTask.get());
    }

    // Add Server entry from services if applicable
    if(fpConfig.startServer)
    {
        if(!fpServices.servers.contains(fpConfig.server))
        {
            postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CONFIG_SERVER_MISSING));
            return ErrorCodes::CONFIG_SERVER_MISSING;
        }

        FP::Json::ServerDaemon configuredServer = fpServices.servers.value(fpConfig.server);

        std::shared_ptr<ExecTask> serverTask = std::make_shared<ExecTask>();
        serverTask->stage = TaskStage::Startup;
        serverTask->path = mFlashpointInstall->fullPath() + '/' + configuredServer.path;
        serverTask->filename = configuredServer.filename;
        serverTask->param = configuredServer.arguments;
        serverTask->nativeParam = QString();
        serverTask->processType = configuredServer.kill ? ProcessType::Deferred : ProcessType::Detached;

        mTaskQueue.push(serverTask);
        logTask(NAME, serverTask.get());
    }

    // Add Daemon entry from services
    QHash<QString, FP::Json::ServerDaemon>::const_iterator daemonIt;
    for (daemonIt = fpServices.daemons.constBegin(); daemonIt != fpServices.daemons.constEnd(); ++daemonIt)
    {
        std::shared_ptr<ExecTask> currentTask = std::make_shared<ExecTask>();
        currentTask->stage = TaskStage::Startup;
        currentTask->path = mFlashpointInstall->fullPath() + '/' + daemonIt.value().path;
        currentTask->filename = daemonIt.value().filename;
        currentTask->param = daemonIt.value().arguments;
        currentTask->nativeParam = QString();
        currentTask->processType = daemonIt.value().kill ? ProcessType::Deferred : ProcessType::Detached;

        mTaskQueue.push(currentTask);
        logTask(NAME, currentTask.get());
    }

    // Return success
    return ErrorCodes::NO_ERR;
}

void Core::enqueueShutdownTasks()
{
    logEvent(NAME, LOG_EVENT_ENQ_STOP);
    // Add Stop entries from services
    for(const FP::Json::StartStop& stopEntry : qAsConstR(mFlashpointInstall->services().stops))
    {
        std::shared_ptr<ExecTask> shutdownTask = std::make_shared<ExecTask>();
        shutdownTask->stage = TaskStage::Shutdown;
        shutdownTask->path = mFlashpointInstall->fullPath() + '/' + stopEntry.path;
        shutdownTask->filename = stopEntry.filename;
        shutdownTask->param = stopEntry.arguments;
        shutdownTask->nativeParam = QString();
        shutdownTask->processType = ProcessType::Blocking;

        mTaskQueue.push(shutdownTask);
        logTask(NAME, shutdownTask.get());
    }
}

ErrorCode Core::enqueueConditionalWaitTask(QFileInfo precedingAppInfo)
{
    // Add wait for apps that involve secure player
    bool involvesSecurePlayer;
    Qx::GenericError securePlayerCheckError = FP::Install::appInvolvesSecurePlayer(involvesSecurePlayer, precedingAppInfo);
    if(securePlayerCheckError.isValid())
    {
        postError(NAME, securePlayerCheckError);
        return ErrorCodes::CANT_READ_BAT_FILE;
    }

    if(involvesSecurePlayer)
    {
        std::shared_ptr<WaitTask> waitTask = std::make_shared<WaitTask>();
        waitTask->stage = TaskStage::Auxiliary;
        waitTask->processName = FP::Install::SECURE_PLAYER_INFO.fileName();

        mTaskQueue.push(waitTask);
        logTask(NAME, waitTask.get());
    }

    // Return success
    return ErrorCodes::NO_ERR;

    // Possible future waits...
}

ErrorCode Core::enqueueDataPackTasks(QUuid targetID)
{
    logEvent(NAME, LOG_EVENT_ENQ_DATA_PACK);

    // Get entry data
    QSqlError searchError;
    FP::DB::QueryBuffer searchResult;

    // Get database
    FP::DB* database = mFlashpointInstall->database();

    if((searchError = database->queryEntryDataByID(searchResult, targetID)).isValid())
    {
        postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return ErrorCodes::SQL_ERROR;
    }

    // Advance result to only record
    searchResult.result.next();

    // Extract relavent data
    QString packDestFolderPath = mFlashpointInstall->fullPath() + "/" + mFlashpointInstall->preferences().dataPacksFolderPath;
    QString packFileName = searchResult.result.value(FP::DB::Table_Game_Data::COL_PATH).toString();
    QString packSha256 = searchResult.result.value(FP::DB::Table_Game_Data::COL_SHA256).toString();
    QFile packFile(packDestFolderPath + "/" + packFileName);

    // Get current file checksum if it exists
    bool checksumMatches = false;

    if(packFile.exists())
    {
        Qx::IOOpReport checksumReport = Qx::fileMatchesChecksum(checksumMatches, packFile, packSha256, QCryptographicHash::Sha256);
        if(!checksumReport.wasSuccessful())
            logError(NAME, Qx::GenericError(Qx::GenericError::Error, checksumReport.getOutcome(), checksumReport.getOutcomeInfo()));

        if(!checksumMatches)
            postError(NAME, Qx::GenericError(Qx::GenericError::Warning, WRN_EXIST_PACK_SUM_MISMATCH));

        logEvent(NAME, LOG_EVENT_DATA_PACK_FOUND);
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
            return ErrorCodes::SQL_ERROR;
        }

        // Advance result to only record (or first if there are more than one in future versions)
        searchResult.result.next();

        // Get Data Pack source base URL
        QString sourceBaseUrl = searchResult.result.value(FP::DB::Table_Source::COL_BASE_URL).toString();

        // Get title specific Data Pack source info
        searchError = database->queryEntrySourceData(searchResult, packSha256);
        if(searchError.isValid())
        {
            postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
            return ErrorCodes::SQL_ERROR;
        }

        // Advance result to only record
        searchResult.result.next();

        // Get title's Data Pack sub-URL
        QString packSubUrl = searchResult.result.value(FP::DB::Table_Source_Data::COL_URL_PATH).toString().replace('\\','/');

        std::shared_ptr<DownloadTask> downloadTask = std::make_shared<DownloadTask>();
        downloadTask->stage = TaskStage::Auxiliary;
        downloadTask->destPath = packDestFolderPath;
        downloadTask->destFileName = packFileName;
        downloadTask->targetFile = sourceBaseUrl + packSubUrl;
        downloadTask->sha256 = packSha256;

        mTaskQueue.push(downloadTask);
        logTask(NAME, downloadTask.get());
    }

    // Enqeue pack mount
    QFileInfo mounterInfo(mFlashpointInstall->datapackMounterPath());

    std::shared_ptr<ExecTask> mountTask = std::make_shared<ExecTask>();
    mountTask->stage = TaskStage::Auxiliary;
    mountTask->path = mounterInfo.absolutePath();
    mountTask->filename = mounterInfo.fileName();
    mountTask->param = QStringList{targetID.toString(QUuid::WithoutBraces), packDestFolderPath + "/" + packFileName};
    mountTask->nativeParam = QString();
    mountTask->processType = ProcessType::Blocking;

    mTaskQueue.push(mountTask);
    logTask(NAME, mountTask.get());

    // Return success
    return ErrorCodes::NO_ERR;
}

void Core::enqueueSingleTask(std::shared_ptr<Task> task) { mTaskQueue.push(task); logTask(NAME, task.get()); }
void Core::clearTaskQueue() { mTaskQueue = {}; }

void Core::logCommand(QString src, QString commandName)
{
    Qx::IOOpReport logReport = mLogger->recordGeneralEvent(src, COMMAND_LABEL.arg(commandName));
    if(!logReport.wasSuccessful())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);
}

void Core::logCommandOptions(QString src, QString commandOptions)
{
    Qx::IOOpReport logReport = mLogger->recordGeneralEvent(src, COMMAND_OPT_LABEL.arg(commandOptions));
    if(!logReport.wasSuccessful())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);
}

void Core::logError(QString src, Qx::GenericError error)
{
    Qx::IOOpReport logReport = mLogger->recordErrorEvent(src, error);

    if(!logReport.wasSuccessful())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);

    if(error.errorLevel() == Qx::GenericError::Critical)
        mCriticalErrorOccured = true;
}

void Core::logEvent(QString src, QString event)
{
    Qx::IOOpReport logReport = mLogger->recordGeneralEvent(src, event);
    if(!logReport.wasSuccessful())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);
}

void Core::logTask(QString src, const Task* const task) { logEvent(src, LOG_EVENT_TASK_ENQ.arg(task->name(), task->members().join(", "))); }

int Core::logFinish(QString src, int exitCode)
{
    if(mCriticalErrorOccured)
        logEvent(src, LOG_ERR_CRITICAL);

    Qx::IOOpReport logReport = mLogger->finish(exitCode);
    if(!logReport.wasSuccessful())
        postError(src, Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);

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

FP::Install& Core::getFlashpointInstall() { return *mFlashpointInstall; }
Core::NotificationVerbosity Core::notifcationVerbosity() const { return mNotificationVerbosity; }
size_t Core::taskCount() const { return mTaskQueue.size(); }
bool Core::hasTasks() const { return mTaskQueue.size() > 0; }

std::shared_ptr<Core::Task> Core::takeFrontTask()
{
    std::shared_ptr<Task> frontTask = mTaskQueue.front();
    mTaskQueue.pop();
    return frontTask;
}

QString Core::statusHeading() { return mStatusHeading; }
QString Core::statusMessage() { return mStatusMessage;}
void Core::setStatus(QString heading, QString message)
{
    mStatusHeading = heading;
    mStatusMessage = message;
    emit statusChanged(heading, message);
}
