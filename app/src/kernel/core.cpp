// Unit Include
#include "core.h"

// Qt Includes
#include <QApplication>

// Qx Includes
#include <qx/utility/qx-helpers.h>

// Magic Enum Includes
#include <magic_enum_flags.hpp>

// Project Includes
#include "command/command.h"
#include "task/t-download.h"
#include "task/t-exec.h"
#include "task/t-extract.h"
#include "task/t-mount.h"
#include "task/t-sleep.h"
#include "task/t-generic.h"
#ifdef _WIN32
    #include "task/t-bideprocess.h"
#endif
#ifdef __linux__
    #include "task/t-awaitdocker.h"
#endif
#include "utility.h"
#include "_buildinfo.h"

//===============================================================================================================
// CoreError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CoreError::CoreError(Type t, const QString& s, Qx::Severity sv) :
    mType(t),
    mSpecific(s),
    mSeverity(sv)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CoreError::isValid() const { return mType != NoError; }
QString CoreError::specific() const { return mSpecific; }
CoreError::Type CoreError::type() const { return mType; }

//Private:
Qx::Severity CoreError::deriveSeverity() const { return mSeverity; }
quint32 CoreError::deriveValue() const { return mType; }
QString CoreError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CoreError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CORE
//===============================================================================================================

//-Constructor-------------------------------------------------------------
Core::Core(QObject* parent) :
    QObject(parent),
    mCriticalErrorOccurred(false),
    mStatusHeading(u"Initializing"_s),
    mStatusMessage(u"..."_s)
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
                dashedNames << ((name.length() > 1 ? u"--"_s : u"-"_s) + name);

            // Add option
            optStr += HELP_OPT_TEMPL.arg(dashedNames.join(u" | "_s), clOption->description());
        }

        // Help commands
        QString commandStr;
        for(const QString& command : qxAsConst(Command::registered()))
            if(Command::hasDescription(command))
                commandStr += HELP_COMMAND_TEMPL.arg(command, Command::describe(command));

        // Complete string
        helpStr = HELP_TEMPL.arg(optStr, commandStr);
    }

    // Show help
    postMessage(Message{.text = helpStr});
}

void Core::showVersion()
{
    setStatus(STATUS_DISPLAY, STATUS_DISPLAY_VERSION);
    postMessage(Message{.text = CL_VERSION_MESSAGE});
}

Qx::Error Core::searchAndFilterEntity(QUuid& returnBuffer, QString name, bool exactName, QUuid parent)
{
    // Clear return buffer
    returnBuffer = QUuid();

    // Search database for title
    Fp::DbError searchError;
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
        postError(NAME, searchError);
        return searchError;
    }

    logEvent(NAME, LOG_EVENT_TITLE_ID_COUNT.arg(searchResult.size).arg(name));

    if(searchResult.size < 1)
    {
        CoreError err(CoreError::TitleNotFound, name);
        postError(NAME, err);
        return err;
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

        return CoreError();
    }
    else if (searchResult.size > FIND_ENTRY_LIMIT)
    {
        CoreError err(CoreError::TooManyResults, name);
        postError(NAME, err);
        return err;
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
                                                    MULTI_GAME_SEL_TEMP.arg(searchResult.result.value(Fp::Db::Table_Game::COL_PLATFORM_NAME).toString(),
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

        return CoreError();
    }
}

//Public:
Qx::Error Core::initialize(QStringList& commandLine)
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
                globalOptions += ' '; // Space after every switch except first one

            globalOptions += u"--"_s + (*clOption).names().at(1); // Always use long name

            // Add value of switch if it takes one
            if(!(*clOption).valueName().isEmpty())
                globalOptions += uR"(=")"_s + clParser.value(*clOption) + uR"(")"_s;
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
        postError(NAME, Qx::Error(logOpen).setSeverity(Qx::Warning), false);

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
        {
            QStringList pArgs = clParser.positionalArguments();
            if(pArgs.count() == 1 && pArgs.front().startsWith(FLASHPOINT_PROTOCOL_SCHEME))
            {
                logEvent(NAME, LOG_EVENT_PROTOCOL_FORWARD);
                commandLine = {"play", "-u", pArgs.front()};
            }
            else
                commandLine = pArgs; // Remove core options from command line list
        }

        // Return success
        return CoreError();
    }
    else
    {
        commandLine.clear(); // Clear remaining options since they are now irrelevant
        showHelp();

        CoreError err(CoreError::InvalidOptions, clParser.errorText());
        postError(NAME, err);
        return err;
    }

}

void Core::attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall)
{
    // Capture install
    mFlashpointInstall = std::move(flashpointInstall);

    // Note install details
    logEvent(NAME, LOG_EVENT_FLASHPOINT_VERSION_TXT.arg(mFlashpointInstall->nameVersionString()));
    logEvent(NAME, LOG_EVENT_FLASHPOINT_VERSION.arg(mFlashpointInstall->version().toString()));
    logEvent(NAME, LOG_EVENT_FLASHPOINT_EDITION.arg(ENUM_NAME(mFlashpointInstall->edition())));
    logEvent(NAME, LOG_EVENT_OUTFITTED_DAEMON.arg(ENUM_NAME(mFlashpointInstall->outfittedDaemon())));

    // Initialize child process env vars
    QProcessEnvironment de = QProcessEnvironment::systemEnvironment();
    QString fpPath = mFlashpointInstall->fullPath();

#ifdef __linux__
    // Add platform support environment variables
    if(mFlashpointInstall->outfittedDaemon() == Fp::Daemon::Qemu) // Appimage based build
    {
        QString pathValue = de.value(u"PATH"_s);
        pathValue.prepend(fpPath + u"/FPSoftware/FPWine/bin:"_s + fpPath + u"/FPSoftware/FPQemuPHP:"_s);
        de.insert(u"PATH"_s, pathValue);
        qputenv("PATH", pathValue.toLocal8Bit()); // Path needs to be updated for self as well

        de.insert(u"GTK_USE_PORTAL"_s, "1");
        de.remove(u"LD_PRELOAD"_s);
    }
    else // Regular Linux build
    {
        QString winFpPath = u"Z:"_s + fpPath;

        de.insert(u"DIR"_s, fpPath);
        de.insert(u"WINDOWS_DIR"_s, winFpPath);
        de.insert(u"FP_STARTUP_PATH"_s, winFpPath + u"\\FPSoftware"_s);
        de.insert(u"FP_BROWSER_PLUGINS"_s, winFpPath + u"\\FPSoftware\\BrowserPlugins"_s);
        de.insert(u"WINEPREFIX"_s, fpPath + u"/FPSoftware/Wine"_s);
    }
#endif

    TExec::setDefaultProcessEnvironment(de);

    // Initialize title specific child process env vars
    mChildTitleProcEnv = de;

    // Add FP root var
    mChildTitleProcEnv.insert(u"FP_PATH"_s, fpPath);

#ifdef __linux__
    // Add HTTTP proxy var
    QString baseProxy = mFlashpointInstall->preferences().browserModeProxy;
    if(!baseProxy.isEmpty())
    {
        QString fullProxy = u"http://"_s + baseProxy + '/';
        mChildTitleProcEnv.insert(u"http_proxy"_s, fullProxy);
        mChildTitleProcEnv.insert(u"HTTP_PROXY"_s, fullProxy);
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
    QString transformedPath = appPath;

    QString fpPath = mFlashpointInstall->fullPath();
    bool isFpAbsolute = transformedPath.startsWith(fpPath);
    if(isFpAbsolute)
    {
        // Remove FP root and separator
        transformedPath.remove(fpPath);
        if(!transformedPath.isEmpty() && (transformedPath.front() == '/' || transformedPath.front() == '\\'))
            transformedPath = transformedPath.mid(1);
    }

    /* TODO: If this is made into a libfp function, isolate this part of it so it stays here,
     * or add a map argument to the libfp function that allows for passing custom "swaps"
     * and then call it with one for the following switch.
     *
     * CLIFp doesn't support the Launcher's built in browser (obviously), so manually
     * override it with Basilisk. Basilisk was removed in FP11 but the app path overrides
     * contains an entry for it that's appropriate on both platforms.
     */
    if(transformedPath == u":browser-mode:"_s)
        transformedPath = u"FPSoftware\\Basilisk-Portable\\Basilisk-Portable.exe"_s;

    // Resolve both swap types
    transformedPath = mFlashpointInstall->resolveExecSwaps(transformedPath, platform);
    transformedPath = mFlashpointInstall->resolveAppPathOverrides(transformedPath);

    // Rebuild full path if applicable
    if(isFpAbsolute)
        transformedPath = fpPath + '/' + transformedPath;

    if(transformedPath != appPath)
        logEvent(NAME, LOG_EVENT_APP_PATH_ALT.arg(appPath, transformedPath));

    // Convert Windows separators to universal '/'
    return transformedPath.replace('\\','/');
}

Qx::Error Core::findGameIdFromTitle(QUuid& returnBuffer, QString title, bool exactTitle)
{
    logEvent(NAME, LOG_EVENT_GAME_SEARCH.arg(title));
    return searchAndFilterEntity(returnBuffer, title, exactTitle);
}

Qx::Error Core::findAddAppIdFromName(QUuid& returnBuffer, QUuid parent, QString name, bool exactName)
{
    logEvent(NAME, LOG_EVENT_ADD_APP_SEARCH.arg(name, parent.toString()));
    return searchAndFilterEntity(returnBuffer, name, exactName, parent);
}

CoreError Core::enqueueStartupTasks()
{
    logEvent(NAME, LOG_EVENT_ENQ_START);

#ifdef __linux__
    /* On Linux X11 Server needs to be temporarily be set to allow connections from root for docker,
     * if it's in use
     */

    if(mFlashpointInstall->outfittedDaemon() == Fp::Daemon::Docker)
    {
        TExec* xhostSet = new TExec(this);
        xhostSet->setIdentifier(u"xhost Set"_s);
        xhostSet->setStage(Task::Stage::Startup);
        xhostSet->setExecutable(u"xhost"_s);
        xhostSet->setDirectory(mFlashpointInstall->fullPath());
        xhostSet->setParameters({u"+SI:localuser:root"_s});
        xhostSet->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(xhostSet);
        logTask(NAME, xhostSet);
    }
#endif

    // Get settings
    Fp::Services fpServices = mFlashpointInstall->services();
    Fp::Config fpConfig = mFlashpointInstall->config();
    Fp::Preferences fpPreferences = mFlashpointInstall->preferences();

    // Add Start entries from services
    for(const Fp::StartStop& startEntry : qAsConst(fpServices.start))
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
        if(!fpServices.server.contains(fpPreferences.server))
        {
            CoreError err(CoreError::ConfiguredServerMissing);
            postError(NAME, err);
            return err;
        }

        Fp::ServerDaemon configuredServer = fpServices.server.value(fpPreferences.server);

        TExec* serverTask = new TExec(this);
        serverTask->setIdentifier(u"Server"_s);
        serverTask->setStage(Task::Stage::Startup);
        serverTask->setExecutable(configuredServer.filename);
        serverTask->setDirectory(mFlashpointInstall->fullPath() + '/' + configuredServer.path);
        serverTask->setParameters(configuredServer.arguments);
        serverTask->setProcessType(configuredServer.kill ? TExec::ProcessType::Deferred : TExec::ProcessType::Detached);

        mTaskQueue.push(serverTask);
        logTask(NAME, serverTask);
    }

    // Add Daemon entry from services
    for(const Fp::ServerDaemon& d : qAsConst(fpServices.daemon))
    {
        TExec* currentTask = new TExec(this);
        currentTask->setIdentifier(u"Daemon"_s);
        currentTask->setStage(Task::Stage::Startup);
        currentTask->setExecutable(d.filename);
        currentTask->setDirectory(mFlashpointInstall->fullPath() + '/' + d.path);
        currentTask->setParameters(d.arguments);
        currentTask->setProcessType(d.kill ? TExec::ProcessType::Deferred : TExec::ProcessType::Detached);

        mTaskQueue.push(currentTask);
        logTask(NAME, currentTask);
    }

#ifdef __linux__
    // On Linux the startup tasks take a while so make sure the docker image is actually running before proceeding
    if(mFlashpointInstall->outfittedDaemon() == Fp::Daemon::Docker)
    {
        TAwaitDocker* dockerWait = new TAwaitDocker(this);
        dockerWait->setStage(Task::Stage::Startup);
        // NOTE: Other than maybe picking it out of the 2nd argument of the stop docker StartStop, there's no clean way to get this name
        dockerWait->setImageName(u"gamezip"_s);
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
    return CoreError();
}

void Core::enqueueShutdownTasks()
{
    logEvent(NAME, LOG_EVENT_ENQ_STOP);
    // Add Stop entries from services
    for(const Fp::StartStop& stopEntry : qxAsConst(mFlashpointInstall->services().stop))
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
    // Undo xhost permissions modifications related to docker
    if(mFlashpointInstall->outfittedDaemon() == Fp::Daemon::Docker)
    {
        TExec* xhostClear = new TExec(this);
        xhostClear->setIdentifier(u"xhost Clear"_s);
        xhostClear->setStage(Task::Stage::Shutdown);
        xhostClear->setExecutable(u"xhost"_s);
        xhostClear->setDirectory(mFlashpointInstall->fullPath());
        xhostClear->setParameters({"-SI:localuser:root"});
        xhostClear->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(xhostClear);
        logTask(NAME, xhostClear);
    }
#endif
}

#ifdef _WIN32
Qx::Error Core::conditionallyEnqueueBideTask(QFileInfo precedingAppInfo)
{
    // Add wait for apps that involve secure player
    bool involvesSecurePlayer;
    Qx::Error securePlayerCheckError = Fp::Install::appInvolvesSecurePlayer(involvesSecurePlayer, precedingAppInfo);
    if(securePlayerCheckError.isValid())
    {
        postError(NAME, securePlayerCheckError);
        return securePlayerCheckError;
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
    return CoreError();

    // Possible future waits...
}
#endif

Qx::Error Core::enqueueDataPackTasks(const Fp::GameData& gameData)
{
    logEvent(NAME, LOG_EVENT_ENQ_DATA_PACK);

    // Extract relevant data
    QString packDestFolderPath = mFlashpointInstall->fullPath() + '/' + mFlashpointInstall->preferences().dataPacksFolderPath;
    QString packFileName = gameData.path();
    QString packSha256 = gameData.sha256();
    QString packParameters = gameData.parameters();
    QFile packFile(packDestFolderPath + '/' + packFileName);

    // Get current file checksum if it exists
    bool checksumMatches = false;

    if(gameData.presentOnDisk() && packFile.exists())
    {
        // Checking the sum in addition to the flag is somewhat overkill, but may help in situations
        // where the flag is set but the datapack's contents have changed
        Qx::IoOpReport checksumReport = Qx::fileMatchesChecksum(checksumMatches, packFile, packSha256, QCryptographicHash::Sha256);
        if(checksumReport.isFailure())
            logError(NAME, checksumReport);

        if(checksumMatches)
            logEvent(NAME, LOG_EVENT_DATA_PACK_FOUND);
        else
            postError(NAME, CoreError(CoreError::DataPackSumMismatch, packFileName, Qx::Warning));
    }
    else
        logEvent(NAME, LOG_EVENT_DATA_PACK_MISS);

    // Enqueue pack download if it doesn't exist or is different than expected
    if(!packFile.exists() || !checksumMatches)
    {
        if(!mFlashpointInstall->preferences().gameDataSources.contains(u"Flashpoint Project"_s))
        {
            CoreError err(CoreError::DataPackSourceMissing, u"Flashpoint Project"_s);
            postError(NAME, err);
            return err;
        }

        Fp::GameDataSource gameSource = mFlashpointInstall->preferences().gameDataSources.value(u"Flashpoint Project"_s);
        QString gameSourceBase = gameSource.arguments.value(0);

        TDownload* downloadTask = new TDownload(this);
        downloadTask->setStage(Task::Stage::Auxiliary);
        downloadTask->setDestinationPath(packDestFolderPath);
        downloadTask->setDestinationFilename(packFileName);
        downloadTask->setTargetFile(gameSourceBase + '/' + packFileName);
        downloadTask->setSha256(packSha256);

        mTaskQueue.push(downloadTask);
        logTask(NAME, downloadTask);

        // Add task to update DB with onDiskState
        int gameDataId = gameData.id();

        TGeneric* onDiskUpdateTask = new TGeneric(this);
        onDiskUpdateTask->setStage(Task::Stage::Auxiliary);
        onDiskUpdateTask->setDescription(u"Update GameData onDisk state."_s);
        onDiskUpdateTask->setAction([gameDataId, this]{
            return mFlashpointInstall->database()->updateGameDataOnDiskState(gameDataId, true);
        });

        mTaskQueue.push(onDiskUpdateTask);
        logTask(NAME, onDiskUpdateTask);
    }

    // Enqueue pack mount or extract
    if(packParameters.contains(u"-extract"_s))
    {
        logEvent(NAME, LOG_EVENT_DATA_PACK_NEEDS_EXTRACT);

        TExtract* extractTask = new TExtract(this);
        extractTask->setStage(Task::Stage::Auxiliary);
        extractTask->setPackPath(packDestFolderPath + '/' + packFileName);
        extractTask->setPathInPack(u"content"_s);
        extractTask->setDestinationPath(mFlashpointInstall->preferences().htdocsFolderPath);

        mTaskQueue.push(extractTask);
        logTask(NAME, extractTask);
    }
    else
    {
        logEvent(NAME, LOG_EVENT_DATA_PACK_NEEDS_MOUNT);

        // Create task
        TMount* mountTask = new TMount(this);
        mountTask->setStage(Task::Stage::Auxiliary);
        mountTask->setTitleId(gameData.gameId());
        mountTask->setPath(packDestFolderPath + '/' + packFileName);
        mountTask->setDaemon(mFlashpointInstall->outfittedDaemon());

        mTaskQueue.push(mountTask);
        logTask(NAME, mountTask);
    }

    // Return success
    return CoreError();
}

void Core::enqueueSingleTask(Task* task) { mTaskQueue.push(task); logTask(NAME, task); }
void Core::clearTaskQueue() { mTaskQueue = {}; }

void Core::logCommand(QString src, QString commandName)
{
    Qx::IoOpReport logReport = mLogger->recordGeneralEvent(src, COMMAND_LABEL.arg(commandName));
    if(logReport.isFailure())
        postError(src, Qx::Error(logReport).setSeverity(Qx::Warning), false);
}

void Core::logCommandOptions(QString src, QString commandOptions)
{
    Qx::IoOpReport logReport = mLogger->recordGeneralEvent(src, COMMAND_OPT_LABEL.arg(commandOptions));
    if(logReport.isFailure())
        postError(src, Qx::Error(logReport).setSeverity(Qx::Warning), false);
}

void Core::logError(QString src, Qx::Error error)
{
    Qx::IoOpReport logReport = mLogger->recordErrorEvent(src, error);

    if(logReport.isFailure())
        postError(src, Qx::Error(logReport).setSeverity(Qx::Warning), false);

    if(error.severity() == Qx::Critical)
        mCriticalErrorOccurred = true;
}

void Core::logEvent(QString src, QString event)
{
    Qx::IoOpReport logReport = mLogger->recordGeneralEvent(src, event);
    if(logReport.isFailure())
        postError(src, Qx::Error(logReport).setSeverity(Qx::Warning), false);
}

void Core::logTask(QString src, const Task* task) { logEvent(src, LOG_EVENT_TASK_ENQ.arg(task->name(), task->members().join(u", "_s))); }

ErrorCode Core::logFinish(QString src, Qx::Error errorState)
{
    if(mCriticalErrorOccurred)
        logEvent(src, LOG_ERR_CRITICAL);

    ErrorCode code = errorState.typeCode();

    Qx::IoOpReport logReport = mLogger->finish(code);
    if(logReport.isFailure())
        postError(src, Qx::Error(logReport).setSeverity(Qx::Warning), false);

    // Return exit code so main function can return with this one
    return code;
}

void Core::postError(QString src, Qx::Error error, bool log)
{
    // Logging
    if(log)
        logError(src, error);

    // Show error if applicable
    if(mNotificationVerbosity == NotificationVerbosity::Full ||
       (mNotificationVerbosity == NotificationVerbosity::Quiet && error.severity() == Qx::Critical))
    {
        // Box error
        Error e;
        e.source = src;
        e.errorInfo = error;

        // Emit
        emit errorOccurred(e);
    }
}

int Core::postBlockingError(QString src, Qx::Error error, bool log, QMessageBox::StandardButtons bs, QMessageBox::StandardButton def)
{
    // Logging
    if(log)
        logError(src, error);

    // Show error if applicable
    if(mNotificationVerbosity == NotificationVerbosity::Full ||
       (mNotificationVerbosity == NotificationVerbosity::Quiet && error.severity() == Qx::Critical))
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
        emit blockingErrorOccurred(response, be);

        // Return response
        return *response;
    }
    else
        return def;
}

void Core::postMessage(const Message& msg) { emit message(msg); }

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

void Core::requestClipboardUpdate(const QString& text) { emit clipboardUpdateRequested(text); }

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

BuildInfo Core::buildInfo() const
{
    constexpr auto sysOpt = magic_enum::enum_cast<BuildInfo::System>(BUILDINFO_SYSTEM);
    static_assert(sysOpt.has_value(), "Configured on unsupported system!");

    constexpr auto linkOpt = magic_enum::enum_cast<BuildInfo::Linkage>(BUILDINFO_LINKAGE);
    static_assert(linkOpt.has_value(), "Invalid BuildInfo linkage string!");

    static BuildInfo info{
        .system = sysOpt.value(),
        .linkage = linkOpt.value(),
        .compiler = BUILDINFO_COMPILER,
        .compilerVersion = QVersionNumber::fromString(BUILDINFO_COMPILER_VER_STR)
    };

    return info;
}
