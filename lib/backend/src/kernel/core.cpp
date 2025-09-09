// Unit Include
#include "core.h"

// Qx Includes
#include <qx/utility/qx-helpers.h>
#include <qx/core/qx-system.h>
#include <qx/core/qx-integrity.h>

// libfp Includes
#include <fp/fp-install.h>

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
#include "tools/archiveaccess.h"
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
// Core
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
Core::Core() :
    Directorate(&mDirector),
    mServicesMode(ServicesMode::Standalone)
{}

//-Destructor----------------------------------------------------------------------------------------------------------
//Public:
Core::~Core() = default; // Required definition so we can delete the unique_ptrs using forward declared types in the header

//-Instance Functions-------------------------------------------------------------
//Private:
QString Core::name() const { return NAME; }

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
    postDirective<DStatusUpdate>(STATUS_DISPLAY, STATUS_DISPLAY_HELP);

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
    postDirective<DMessage>(helpStr);
}

void Core::showVersion()
{
    postDirective<DStatusUpdate>(STATUS_DISPLAY, STATUS_DISPLAY_VERSION);
    postDirective<DMessage>(CL_VERSION_MESSAGE);
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
        postDirective<DError>(searchError);
        return searchError;
    }

    logEvent(LOG_EVENT_TITLE_ID_COUNT.arg(searchResult.size).arg(name));

    if(searchResult.size < 1)
    {
        CoreError err(CoreError::TitleNotFound, name);
        postDirective<DError>(err);
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
        logEvent(LOG_EVENT_TITLE_ID_DETERMINED.arg(name, returnBuffer.toString(QUuid::WithoutBraces)));

        return CoreError();
    }
    else if (searchResult.size > FIND_ENTRY_LIMIT)
    {
        CoreError err(CoreError::TooManyResults, name);
        postDirective<DError>(err);
        return err;
    }
    else
    {
        logEvent(LOG_EVENT_TITLE_SEL_PROMNPT);
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
        QString userChoice = postDirective(DItemSelection{
            .caption = MULTI_TITLE_SEL_CAP,
            .label = MULTI_TITLE_SEL_LABEL,
            .items = idChoices,
        });

        if(userChoice.isNull()) // Default if diag is silenced
            logEvent(LOG_EVENT_TITLE_SEL_CANCELED);
        else
        {
            // Set return buffer
            returnBuffer = idMap.value(userChoice); // If user choice is null, this will
            logEvent(LOG_EVENT_TITLE_ID_DETERMINED.arg(name, returnBuffer.toString(QUuid::WithoutBraces)));
        }

        return CoreError();
    }
}

void Core::addOnDiskUpdateTask(int gameDataId)
{
    TGeneric* onDiskUpdateTask = new TGeneric(*this);
    onDiskUpdateTask->setStage(Task::Stage::Auxiliary);
    onDiskUpdateTask->setDescription(u"Update GameData onDisk state."_s);
    onDiskUpdateTask->setAction([gameDataId, this]{
        return mFlashpointInstall->database()->updateGameDataOnDiskState({gameDataId}, true);
    });

    mTaskQueue.push(onDiskUpdateTask);
    logTask(onDiskUpdateTask);
}

// TODO: Have task have a toString function/operator instead of "members()" (or make members() private and have toString() use that)
void Core::logTask(const Task* task) { logEvent(LOG_EVENT_TASK_ENQ.arg(task->name(), task->members().join(u", "_s))); }

//Public:
Qx::Error Core::initialize(QStringList& commandLine)
{
    // Send initial status
    postDirective<DStatusUpdate>(u"Initializing"_s, u"..."_s);

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

            globalOptions += u"--"_s + (*clOption).names().constLast(); // Always use long name

            // Add value of switch if it takes one
            if(!(*clOption).valueName().isEmpty())
                globalOptions += uR"(=")"_s + clParser.value(*clOption) + uR"(")"_s;
        }
    }
    if(globalOptions.isEmpty())
        globalOptions = LOG_NO_PARAMS;

    // Remove app name from command line string
    commandLine.removeFirst();

    // Open log
    mDirector.openLog(commandLine);

    // Log initialization step
    logEvent(LOG_EVENT_INIT);

    // Log global options
    logEvent(LOG_EVENT_GLOBAL_OPT.arg(globalOptions));

    // Check for valid arguments
    if(!validArgs)
    {
        commandLine.clear(); // Clear remaining options since they are now irrelevant
        showHelp();

        CoreError err(CoreError::InvalidOptions, clParser.errorText());
        postDirective<DError>(err);
        return err;
    }

    // Handle each global option
    Director::Verbosity v = clParser.isSet(CL_OPTION_SILENT) ? Director::Verbosity::Silent :
                            clParser.isSet(CL_OPTION_QUIET) ? Director::Verbosity::Quiet : Director::Verbosity::Full;
    mDirector.setVerbosity(v);

    if(clParser.isSet(CL_OPTION_VERSION))
    {
        showVersion();
        commandLine.clear(); // Clear args so application terminates after Core setup
        logEvent(LOG_EVENT_VER_SHOWN);
    }
    else if(clParser.isSet(CL_OPTION_HELP) || (!isActionableOptionSet(clParser) && clParser.positionalArguments().count() == 0)) // Also when no parameters
    {
        showHelp();
        commandLine.clear(); // Clear args so application terminates after Core setup
        logEvent(LOG_EVENT_G_HELP_SHOWN);
    }
    else
    {
        QStringList pArgs = clParser.positionalArguments();
        if(pArgs.count() == 1 && pArgs.front().startsWith(FLASHPOINT_PROTOCOL_SCHEME))
        {
            logEvent(LOG_EVENT_PROTOCOL_FORWARD);
            commandLine = {"play", "-u", pArgs.front()};
        }
        else
            commandLine = pArgs; // Remove core options from command line list
    }

    // Return success
    return CoreError();
}

void Core::setServicesMode(ServicesMode mode)
{
    logEvent(LOG_EVENT_MODE_SET.arg(ENUM_NAME(mode)));
    mServicesMode = mode;

    if(mode == ServicesMode::Companion)
        watchLauncher();
}

void Core::watchLauncher()
{
    logEvent(LOG_EVENT_LAUNCHER_WATCH);

    using namespace std::chrono_literals;
    mLauncherWatcher.setProcessName(Fp::Install::LAUNCHER_NAME);
#ifdef __linux__
    mLauncherWatcher.setPollRate(1s); // Generous rate since while we need to know quickly, we don't THAT quickly
#endif
    connect(&mLauncherWatcher, &Qx::ProcessBider::established, this, [this]{
        logEvent(LOG_EVENT_LAUNCHER_WATCH_HOOKED);
    });
    connect(&mLauncherWatcher, &Qx::ProcessBider::errorOccurred, this, [this](Qx::ProcessBiderError err){
        logError(err);
    });
    connect(&mLauncherWatcher, &Qx::ProcessBider::finished, this, [this]{
        // Launcher closed (or can't be hooked), need to bail
        CoreError err(CoreError::CompanionModeLauncherClose, LOG_EVENT_LAUNCHER_CLOSED_RESULT);
        postDirective<DError>(err);
        emit abort(err);
    });

    mLauncherWatcher.start();
    // The bide is automatically abandoned when core, and therefore the bider, is destroyed
}

void Core::attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall)
{
    // Capture install
    mFlashpointInstall = std::move(flashpointInstall);

    // Note install details
    auto info = mFlashpointInstall->versionInfo();
    logEvent(LOG_EVENT_FLASHPOINT_VERSION_TXT.arg(info->fullString()));
    logEvent(LOG_EVENT_FLASHPOINT_VERSION.arg(info->version().toString()));
    logEvent(LOG_EVENT_FLASHPOINT_EDITION.arg(ENUM_NAME(info->edition())));
    logEvent(LOG_EVENT_OUTFITTED_DAEMON.arg(ENUM_NAME(mFlashpointInstall->outfittedDaemon())));

    // Initialize child process env vars
    QProcessEnvironment de = QProcessEnvironment::systemEnvironment();
    const QString fpPath = mFlashpointInstall->dir().absolutePath();

#ifdef __linux__
    //-Mimic startup script setup-------------------------
    logEvent(LOG_EVENT_LINUX_SPECIFIC_STARTUP_STEPS);

    // NOTE: This check likely will need to be modified over time, though it's close to how the script does it
    QDir librariesDir(mFlashpointInstall->dir().absoluteFilePath(u"Libraries"_s));
    bool immutable = !librariesDir.entryList({"*.so"}, QDir::Files).isEmpty();
    logEvent(LOG_EVENT_LINUX_BUILD_TYPE.arg(immutable ? u"isn't"_s : u"is"_s));

    if(immutable)
    {
        de.insert(u"GDK_PIXBUF_MODULE_FILE"_s, fpPath + u"/Libraries/loader.cache"_s);
        de.insert(u"GSETTINGS_SCHEMA_DIR"_s, fpPath + u"/Libraries"_s);
        de.insert(u"LD_LIBRARY_PATH"_s, fpPath + u"/Libraries"_s);
        de.insert(u"LIBGL_DRIVERS_PATH"_s, fpPath + u"/Libraries"_s);

        /* Completely override PATH for child processes.
         * Currently we fully override our own path too, but if this breaks things, then we should only prepend the change to our own path.
         *
         * The addition of the WINE path is done just before game launch in the vanilla launcher, but there shouldn't be an issue doing it here.
         */
        QString pathChange = fpPath + u"/FPSoftware/Wine/bin:"_s +
                             fpPath + u"/Libraries"_s;
        de.insert(u"PATH"_s, pathChange);
        qputenv("PATH", pathChange.toLocal8Bit());
    }

    de.insert(u"WINEPREFIX"_s, fpPath + u"/FPSoftware/Wine"_s);
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
     * in "BackgroundServices.ts" but that file doesn't exist so it's either a leftover (hopefully)
     * or a future feature (hopefully not). Either way it just points to the PHP server port.
     */
#endif

    // Setup archive access
    if(info->edition() == Fp::Install::VersionInfo::Ultimate)
        mGamesArchive = std::make_unique<ArchiveAccess>(*this, ArchiveAccess::GameData);
}

QString Core::resolveFullAppPath(const QString& appPath, const QString& platform)
{
    // We don't have a browser mode. Since Electron bundles chromium, chrome should give the closest experience to the launcher's browser mode.
    static const QHash<QString, QString> clifpOverrides{
        {u":browser-mode:"_s, u"FPSoftware\\startChrome.bat"_s}
    };

    const Fp::Toolkit* tk = mFlashpointInstall->toolkit();

    QString swapPath = appPath;
    if(tk->resolveTrueAppPath(swapPath, platform, clifpOverrides))
        logEvent(LOG_EVENT_APP_PATH_ALT.arg(appPath, swapPath));

    return mFlashpointInstall->dir().absoluteFilePath(swapPath);
}

Qx::Error Core::findGameIdFromTitle(QUuid& returnBuffer, QString title, bool exactTitle)
{
    logEvent(LOG_EVENT_GAME_SEARCH.arg(title));
    return searchAndFilterEntity(returnBuffer, title, exactTitle);
}

Qx::Error Core::findAddAppIdFromName(QUuid& returnBuffer, QUuid parent, QString name, bool exactName)
{
    logEvent(LOG_EVENT_ADD_APP_SEARCH.arg(name, parent.toString()));
    return searchAndFilterEntity(returnBuffer, name, exactName, parent);
}

bool Core::blockNewInstances()
{
    bool b = Qx::enforceSingleInstance(SINGLE_INSTANCE_ID);
    logEvent(b ? LOG_EVENT_FURTHER_INSTANCE_BLOCK_SUCC : LOG_EVENT_FURTHER_INSTANCE_BLOCK_FAIL);
    return b;
}

CoreError Core::enqueueStartupTasks(const QString& serverOverride)
{
    logEvent(LOG_EVENT_ENQ_START);
    
    if(mServicesMode == ServicesMode::Companion)
    {
        logEvent(LOG_EVENT_SERVICES_FROM_LAUNCHER);
        // TODO: Allegedly apache and php are going away at some point so this hopefully isn't needed for long
        return !serverOverride.isEmpty() ? CoreError(CoreError::CompanionModeServerOverride) : CoreError();
    }

#ifdef __linux__
    /* On Linux X11 Server needs to be temporarily be set to allow connections from root for docker,
     * if it's in use
     */

    if(mFlashpointInstall->outfittedDaemon() == Fp::Daemon::Docker)
    {
        TExec* xhostSet = new TExec(*this);
        xhostSet->setIdentifier(u"xhost Set"_s);
        xhostSet->setStage(Task::Stage::Startup);
        xhostSet->setExecutable(u"xhost"_s);
        xhostSet->setDirectory(mFlashpointInstall->dir());
        xhostSet->setParameters({u"+SI:localuser:root"_s});
        xhostSet->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(xhostSet);
        logTask(xhostSet);
    }
#endif

    // Get settings
    const Fp::Toolkit* fpTk = mFlashpointInstall->toolkit();
    Fp::Services fpServices = mFlashpointInstall->services();
    Fp::Config fpConfig = mFlashpointInstall->config();
    Fp::Preferences fpPreferences = mFlashpointInstall->preferences();

    // Add Start entries from services
    for(const Fp::StartStop& startEntry : std::as_const(fpServices.start))
    {
        TExec* currentTask = new TExec(*this);
        currentTask->setIdentifier(startEntry.filename);
        currentTask->setStage(Task::Stage::Startup);
        currentTask->setExecutable(startEntry.filename);
        currentTask->setDirectory(mFlashpointInstall->dir().absoluteFilePath(startEntry.path));
        currentTask->setParameters(startEntry.arguments);
        currentTask->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(currentTask);
        logTask(currentTask);
    }

    // Add Server entry from services if applicable
    if(fpConfig.startServer)
    {
        std::optional<Fp::ServerDaemon> foundServer = fpTk->getServer(serverOverride); // Will pull fpPreferences.server if empty
        if(!foundServer)
        {
            CoreError err(CoreError::ConfiguredServerMissing);
            postDirective<DError>(err);
            return err;
        }

        Fp::ServerDaemon server = foundServer.value();

        // Some linux builds run the "read" builtin as a form of no-op so that no server runs
        if(server.filename == u"read"_s)
            logEvent(LOG_EVENT_LINUX_SERVER_NOOP);
        else
        {
            TExec* serverTask = new TExec(*this);
            serverTask->setIdentifier(u"Server"_s);
            serverTask->setStage(Task::Stage::Startup);
            serverTask->setExecutable(server.filename);
            serverTask->setDirectory(mFlashpointInstall->dir().absoluteFilePath(server.path));
            serverTask->setParameters(server.arguments);
            serverTask->setProcessType(server.kill ? TExec::ProcessType::Deferred : TExec::ProcessType::Detached);

            mTaskQueue.push(serverTask);
            logTask(serverTask);
        }
    }

    // Add Daemon entry from services
    for(const Fp::ServerDaemon& d : std::as_const(fpServices.daemon))
    {
        TExec* currentTask = new TExec(*this);
        currentTask->setIdentifier(u"Daemon"_s);
        currentTask->setStage(Task::Stage::Startup);
        currentTask->setExecutable(d.filename);
        currentTask->setDirectory(mFlashpointInstall->dir().absoluteFilePath(d.path));
        currentTask->setParameters(d.arguments);
        currentTask->setProcessType(d.kill ? TExec::ProcessType::Deferred : TExec::ProcessType::Detached);

        mTaskQueue.push(currentTask);
        logTask(currentTask);
    }

#ifdef __linux__
    // On Linux the startup tasks take a while so make sure the docker image is actually running before proceeding
    if(mFlashpointInstall->outfittedDaemon() == Fp::Daemon::Docker)
    {
        TAwaitDocker* dockerWait = new TAwaitDocker(*this);
        dockerWait->setStage(Task::Stage::Startup);
        // NOTE: Other than maybe picking it out of the 2nd argument of the stop docker StartStop, there's no clean way to get this name
        dockerWait->setImageName(u"gamezip"_s);
        dockerWait->setTimeout(10000);

        mTaskQueue.push(dockerWait);
        logTask(dockerWait);
    }
#endif

    /* Make sure that all startup processes have fully initialized.
     *
     * This is especially important for docker, as the mount server inside seems to take an extra moment to initialize (gives
     * "Connection Closed" if a mount attempt is made right away).
     */
    TSleep* initDelay = new TSleep(*this);
    initDelay->setStage(Task::Stage::Startup);
    initDelay->setDuration(1500); // NOTE: Might need to be made longer

    mTaskQueue.push(initDelay);
    logTask(initDelay);

    // Return success
    return CoreError();
}

void Core::enqueueShutdownTasks()
{
    logEvent(LOG_EVENT_ENQ_STOP);
    
    if(mServicesMode == ServicesMode::Companion)
    {
        logEvent(LOG_EVENT_SERVICES_FROM_LAUNCHER);
        return;
    }

    // Add Stop entries from services
    for(const Fp::StartStop& stopEntry : qxAsConst(mFlashpointInstall->services().stop))
    {
        TExec* shutdownTask = new TExec(*this);
        shutdownTask->setIdentifier(stopEntry.filename);
        shutdownTask->setStage(Task::Stage::Shutdown);
        shutdownTask->setExecutable(stopEntry.filename);
        shutdownTask->setDirectory(mFlashpointInstall->dir().absoluteFilePath(stopEntry.path));
        shutdownTask->setParameters(stopEntry.arguments);
        shutdownTask->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(shutdownTask);
        logTask(shutdownTask);
    }

#ifdef __linux__
    // Undo xhost permissions modifications related to docker
    if(mFlashpointInstall->outfittedDaemon() == Fp::Daemon::Docker)
    {
        TExec* xhostClear = new TExec(*this);
        xhostClear->setIdentifier(u"xhost Clear"_s);
        xhostClear->setStage(Task::Stage::Shutdown);
        xhostClear->setExecutable(u"xhost"_s);
        xhostClear->setDirectory(mFlashpointInstall->dir());
        xhostClear->setParameters({"-SI:localuser:root"});
        xhostClear->setProcessType(TExec::ProcessType::Blocking);

        mTaskQueue.push(xhostClear);
        logTask(xhostClear);
    }
#endif
}

#ifdef _WIN32
Qx::Error Core::conditionallyEnqueueBideTask(const TExec* precedingTask)
{
    const Fp::Toolkit* tk = mFlashpointInstall->toolkit();

    // Add wait for apps that involve secure player
    QFileInfo preeedingInfo(precedingTask->executable());
    bool involvesSecurePlayer;
    Qx::Error securePlayerCheckError = tk->appInvolvesSecurePlayer(involvesSecurePlayer, preeedingInfo);
    if(securePlayerCheckError.isValid())
    {
        postDirective<DError>(securePlayerCheckError);
        return securePlayerCheckError;
    }

    if(involvesSecurePlayer)
    {
        TBideProcess* waitTask = new TBideProcess(*this);
        waitTask->setStage(Task::Stage::Auxiliary);
        waitTask->setProcessName(tk->SECURE_PLAYER_INFO.fileName());

        mTaskQueue.push(waitTask);
        logTask(waitTask);
    }

    // Return success
    return CoreError();

    // Possible future waits...
}
#endif

Qx::Error Core::enqueueDataPackTasks(const Fp::GameData& gameData)
{
    logEvent(LOG_EVENT_ENQ_DATA_PACK);

    const Fp::Toolkit* tk = mFlashpointInstall->toolkit();

    QString packPath = tk->datapackPath(gameData);
    QString packFilename = tk->datapackFilename(gameData);
    logEvent(LOG_EVENT_DATA_PACK_PATH.arg(packPath));

    // Try to acquire data pack if it's not present
    if(!tk->datapackIsPresent(gameData))
    {
        logEvent(LOG_EVENT_DATA_PACK_MISS);

        auto edition = mFlashpointInstall->versionInfo()->edition();
        if(edition == Fp::Install::VersionInfo::Ultimate)
        {
            /* TODO: Might want to make all of this its own task, and have a static, singleton, manager
             * type (e.g. ArchiveAccessManager) that holds the archive instances, instead of core,
             * which is then used by that task, and then here we just setup the task. This way we
             * can also have more specific errors instead of just the one (with events).
             */

            /* TODO: Need to step through a debug build, as despite what the source looks like, the real
             * thing seems like it doesn't save the game data to disk, but instead uses it directly as-is.
             * Pulling it from the archive is fine for now though as it's unlikely to add up to much for
             * most users.
             */
            Q_ASSERT(mGamesArchive);
            logEvent(LOG_EVENT_DATA_PACK_FROM_ARCHIVE);

            // Check archive
            QByteArray datapackData = mGamesArchive->getFileContents(DATA_PACK_FROM_ARCHIVE_TEMPLATE.arg(packFilename));
            if(datapackData.isEmpty())
            {
                logEvent(LOG_EVENT_DATA_PACK_FROM_ARCHIVE_NOT_FOUND);
                CoreError err(CoreError::CannotObtainDatapack);
                postDirective<DError>(err);
                return err;
            }

            logEvent(LOG_EVENT_DATA_PACK_FROM_ARCHIVE_FOUND);
            if(gameData.sha256().compare(Qx::Integrity::generateChecksum(datapackData, QCryptographicHash::Sha256), Qt::CaseInsensitive) != 0)
            {
                logEvent(LOG_EVENT_DATA_PACK_FROM_ARCHIVE_CORRUPT);
                CoreError err(CoreError::CannotObtainDatapack);
                postDirective<DError>(err);
                return err;
            }

            // Save to disk
            QFile packFile(packPath);
            if(auto writeOp = Qx::writeBytesToFile(packFile, datapackData); writeOp.isFailure())
                return writeOp;

            logEvent(LOG_EVENT_DATA_PACK_FROM_ARCHIVE_SAVED);

            // Add task to update DB with onDiskState
            addOnDiskUpdateTask(gameData.id());
        }
        else // Basically just Infinity
        {
            TDownload* downloadTask = new TDownload(*this);
            downloadTask->setStage(Task::Stage::Auxiliary);
            downloadTask->setDescription(u"data pack "_s + packFilename);
            TDownloadError packError = downloadTask->addDatapack(tk, &gameData);
            if(packError.isValid())
            {
                postDirective<DError>(packError);
                return packError;
            }

            mTaskQueue.push(downloadTask);
            logTask(downloadTask);

            // Add task to update DB with onDiskState
            addOnDiskUpdateTask(gameData.id());
        }
    }
    else
        logEvent(LOG_EVENT_DATA_PACK_FOUND);

    // Handle datapack parameters
    Fp::GameDataParameters param = gameData.parameters();
    if(param.hasError())
        postDirective<DError>(CoreError(CoreError::UnknownDatapackParam, param.errorString(), Qx::Warning));

    if(param.isExtract())
    {
        logEvent(LOG_EVENT_DATA_PACK_NEEDS_EXTRACT);

        QDir extractRoot(mFlashpointInstall->dir().absoluteFilePath(mFlashpointInstall->preferences().htdocsFolderPath));
        QString marker = QDir::fromNativeSeparators(param.extractedMarkerFile());
        // Check if files are already present
        if(!marker.isEmpty() && QFile::exists(extractRoot.absoluteFilePath(marker)))
            logEvent(LOG_EVENT_DATA_PACK_ALREADY_EXTRACTED);
        else
        {
            TExtract* extractTask = new TExtract(*this);
            extractTask->setStage(Task::Stage::Auxiliary);
            extractTask->setPackPath(packPath);
            extractTask->setPathInPack(u"content"_s);
            extractTask->setDestinationPath(extractRoot.absolutePath());

            mTaskQueue.push(extractTask);
            logTask(extractTask);
        }
    }
    else
    {
        logEvent(LOG_EVENT_DATA_PACK_NEEDS_MOUNT);

        // Create task
        TMount* mountTask = new TMount(*this);
        mountTask->setStage(Task::Stage::Auxiliary);
        mountTask->setTitleId(gameData.gameId());
        mountTask->setPath(packPath);
        mountTask->setDaemon(mFlashpointInstall->outfittedDaemon());

        mTaskQueue.push(mountTask);
        logTask(mountTask);
    }

    // Return success
    return CoreError();
}

void Core::enqueueSingleTask(Task* task) { mTaskQueue.push(task); logTask(task); }

Director* Core::director() { return &mDirector; }
Core::ServicesMode Core::mode() const { return mServicesMode; }
Fp::Install& Core::fpInstall() { return *mFlashpointInstall; }
const QProcessEnvironment& Core::childTitleProcessEnvironment() { return mChildTitleProcEnv; }
size_t Core::taskCount() const { return mTaskQueue.size(); }
bool Core::hasTasks() const { return mTaskQueue.size() > 0; }
Task* Core::frontTask() { return mTaskQueue.front(); }
void Core::removeFrontTask() { mTaskQueue.pop(); }

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
