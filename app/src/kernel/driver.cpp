// Unit Include
#include "driver.h"

// Qt Includes
#include <QApplication>

// Qx Includes
#include <qx/core/qx-system.h>
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "command/command.h"
#include "command/c-update.h"
#include "task/t-exec.h"
#include "utility.h"

//===============================================================================================================
// DriverError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
DriverError::DriverError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool DriverError::isValid() const { return mType != NoError; }
QString DriverError::specific() const { return mSpecific; }
DriverError::Type DriverError::type() const { return mType; }

//Private:
Qx::Severity DriverError::deriveSeverity() const { return Qx::Critical; }
quint32 DriverError::deriveValue() const { return mType; }
QString DriverError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString DriverError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// DRIVER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
Driver::Driver(QStringList arguments) :
    mArguments(arguments),
    mCore(nullptr),
    mErrorStatus(),
    mCurrentTask(nullptr),
    mCurrentTaskNumber(-1),
    mQuitRequested(false)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
void Driver::init()
{
    // Create core
    mCore = new Core(this);

    //-Setup Core---------------------------
    connect(mCore, &Core::statusChanged, this, &Driver::statusChanged);
    connect(mCore, &Core::errorOccurred, this, &Driver::errorOccurred);
    connect(mCore, &Core::blockingErrorOccurred, this, &Driver::blockingErrorOccurred);
    connect(mCore, &Core::message, this, &Driver::message);
    connect(mCore, &Core::saveFileRequested, this, &Driver::saveFileRequested);
    connect(mCore, &Core::existingDirRequested, this, &Driver::existingDirRequested);
    connect(mCore, &Core::itemSelectionRequested, this, &Driver::itemSelectionRequested);
    connect(mCore, &Core::clipboardUpdateRequested, this, &Driver::clipboardUpdateRequested);
    connect(mCore, &Core::questionAnswerRequested, this, &Driver::questionAnswerRequested);
    connect(mCore, &Core::abort, this, [this](CoreError err){
        logEvent(LOG_EVENT_CORE_ABORT);
        mErrorStatus = err;
        quit();
    });

    //-Setup deferred process manager------
    /* NOTE: It looks like the manager should just be a stack member of TExec that is constructed
     * during program initialization, but it can't be, as since it's a QObject it would belong
     * to the default thread, when it needs to belong to the thread that Driver gets moved to. So,
     * instead we create it during Driver init (post thread move) and install it into TExec as a
     * static pointer member. An alternative could have been making TExec internally use a
     * "construct on first use" getter function and the "rule" simply being don't perform a TExec
     * task except from the correct thread (which would only ever happen anyway), but then that
     * would make deleting the object slightly tricky. This way it can just be parented to core
     */
    DeferredProcessManager* dpm = new DeferredProcessManager(mCore);
    // qOverload because it gets confused with the shorter versions within core even though they're private :/
    connect(dpm, &DeferredProcessManager::eventOccurred, mCore, qOverload<const QString&, const QString&>(&Core::logEvent));
    connect(dpm, &DeferredProcessManager::errorOccurred, mCore, qOverload<const QString&, const Qx::Error&>(&Core::logError));
    TExec::installDeferredProcessManager(dpm);
}

void Driver::startNextTask()
{
    // Ensure tasks exist
    if(!mCore->hasTasks())
        qFatal("Called with no tasks remaining.");

    // Take task at front of queue
    mCurrentTask = mCore->frontTask();
    mCore->removeFrontTask();
    mCurrentTaskNumber++;

    // Log task start
    logEvent(LOG_EVENT_TASK_START.arg(QString::number(mCurrentTaskNumber),
                                                   mCurrentTask->name(),
                                                   ENUM_NAME(mCurrentTask->stage())));

    // Only execute task after an error/quit if it is a Shutdown task
    bool isShutdown = mCurrentTask->stage() == Task::Stage::Shutdown;
    if(mErrorStatus.isSet() && !isShutdown)
    {
        logEvent(LOG_EVENT_TASK_SKIP_ERROR);

        // Queue up finished handler directly (executes on next event loop cycle) since task was skipped
        // Can't connect directly because newer connect syntax doesn't support default args
        QTimer::singleShot(0, this, [this](){ completeTaskHandler(); });
    }
    else if(mQuitRequested && !isShutdown)
    {
        logEvent(LOG_EVENT_TASK_SKIP_QUIT);

        // Queue up finished handler directly (executes on next event loop cycle) since task was skipped
        // Can't connect directly because newer connect syntax doesn't support default args
        QTimer::singleShot(0, this, [this](){ completeTaskHandler(); });
    }
    else
    {
        // Connect task notifiers
        connect(mCurrentTask, &Task::notificationReady, mCore, &Core::postMessage);
        connect(mCurrentTask, &Task::eventOccurred, mCore, qOverload<const QString&, const QString&>(&Core::logEvent));
        connect(mCurrentTask, &Task::errorOccurred, mCore, [this](QString taskName, Qx::Error error){
            mCore->postError(taskName, error); // Can't connect directly because newer connect syntax doesn't support default args
        });
        connect(mCurrentTask, &Task::blockingErrorOccurred, this,
                [this](QString taskName, int* response, Qx::Error error, QMessageBox::StandardButtons choices) {
            *response = mCore->postBlockingError(taskName, error, true, choices);
        });
        connect(mCurrentTask, &Task::longTaskStarted, this, &Driver::longTaskStarted);
        connect(mCurrentTask, &Task::longTaskTotalChanged, this, &Driver::longTaskTotalChanged);
        connect(mCurrentTask, &Task::longTaskProgressChanged, this, &Driver::longTaskProgressChanged);
        connect(mCurrentTask, &Task::longTaskFinished, this, &Driver::longTaskFinished);

        // QueuedConnection, allow event processing between tasks
        connect(mCurrentTask, &Task::complete, this, &Driver::completeTaskHandler, Qt::QueuedConnection);

        // Perform task
        mCurrentTask->perform();
    }
}

void Driver::cleanup()
{
    logEvent(LOG_EVENT_CLEANUP_START);

    // Close each remaining child process
    logEvent(LOG_EVENT_ENDING_CHILD_PROCESSES);
    TExec::deferredProcessManager()->closeProcesses();

    logEvent(LOG_EVENT_CLEANUP_FINISH);
}

void Driver::finish()
{
    logEvent(LOG_EVENT_FINISH);

    // Clear update cache
    if(CUpdate::isUpdateCacheClearable())
    {
        if(CUpdateError err = CUpdate::clearUpdateCache(); err.isValid())
            logError(err);
        else
            logEvent(LOG_EVENT_CLEARED_UPDATE_CACHE);
    }

    emit finished(logFinish(mErrorStatus.value()));
}

void Driver::quit()
{
    mQuitRequested = true;

    // Stop current task (assuming it can be)
    if(mCurrentTask)
        mCurrentTask->stop();
}

// Helper functions
std::unique_ptr<Fp::Install> Driver::findFlashpointInstall()
{
    QDir currentDir(CLIFP_DIR_PATH);
    std::unique_ptr<Fp::Install> fpInstall;

    do
    {
        logEvent(LOG_EVENT_FLASHPOINT_ROOT_CHECK.arg(QDir::toNativeSeparators(currentDir.absolutePath())));

        // Attempt to instantiate
        fpInstall = std::make_unique<Fp::Install>(currentDir.absolutePath());
        if(fpInstall->isValid())
        {
            if(fpInstall->outfittedDaemon() == Fp::Daemon::Unknown)
            {
                logError(Qx::GenericError(Qx::Warning, 12011, LOG_WARN_FP_UNRECOGNIZED_DAEMON));
                fpInstall.reset();
            }
            else
                break;
        }
        else
        {
            logError(fpInstall->error().setSeverity(Qx::Warning));
            fpInstall.reset();
        }
    }
    while(currentDir.cdUp());

    // Return instance, null or not
    return std::move(fpInstall);
}

// Notifications/Logging (core-forwarders)
void Driver::logCommand(QString commandName) { Q_ASSERT(mCore); mCore->logCommand(NAME, commandName); }
void Driver::logCommandOptions(QString commandOptions) { Q_ASSERT(mCore); mCore->logCommandOptions(NAME, commandOptions); }
void Driver::logError(Qx::Error error) { Q_ASSERT(mCore); mCore->logError(NAME, error); }
void Driver::logEvent(QString event) { Q_ASSERT(mCore); mCore->logEvent(NAME, event); }
void Driver::logTask(const Task* task) { Q_ASSERT(mCore); mCore->logTask(NAME, task); }
ErrorCode Driver::logFinish(Qx::Error errorState) { Q_ASSERT(mCore); return mCore->logFinish(NAME, errorState); }
void Driver::postError(Qx::Error error, bool log) { Q_ASSERT(mCore); mCore->postError(NAME, error, log); }
int Driver::postBlockingError(Qx::Error error, bool log, QMessageBox::StandardButtons bs, QMessageBox::StandardButton def) { Q_ASSERT(mCore); return mCore->postBlockingError(NAME, error, log); }

//-Slots--------------------------------------------------------------------------------
//Private:
void Driver::completeTaskHandler(Qx::Error e)
{
    // Handle errors
    if(e.isValid())
    {
        mErrorStatus = e;
        logEvent(LOG_EVENT_TASK_FINISH_ERR.arg(mCurrentTaskNumber)); // Record early end of task
    }

    // Cleanup handled task
    logEvent(LOG_EVENT_TASK_FINISH.arg(mCurrentTaskNumber));
    qxDelete(mCurrentTask);

    // Perform next task if any remain
    if(mCore->hasTasks())
        startNextTask();
    else
    {
        logEvent(LOG_EVENT_QUEUE_FINISH);
        cleanup();
        finish();
    }
}

//Public:
void Driver::drive()
{
    // Initialize
    init();

    //-Initialize Core--------------------------------------------------------------------------
    mErrorStatus = mCore->initialize(mArguments);
    if(mErrorStatus.isSet() || mArguments.empty()) // Terminate if error or no command
    {
        finish();
        return;
    }

    //-Prepare Command---------------------------------------------------------------------
    QString commandStr = mArguments.first().toLower();

    // Check for valid command
    if(CommandError ce = Command::isRegistered(commandStr); ce.isValid())
    {
        postError(ce);
        mErrorStatus = ce;
        finish();
        return;
    }

    // Create command instance
    std::unique_ptr<Command> commandProcessor = Command::acquire(commandStr, *mCore);

    //-Set Service Mode--------------------------------------------------------------------

    // Check state of standard launcher
    bool launcherRunning = Qx::processIsRunning(Fp::Install::LAUNCHER_NAME);
    mCore->setServicesMode(launcherRunning && commandProcessor->requiresServices() ? Core::Companion : Core::Standalone);

    //-Restrict app to only one instance---------------------------------------------------
    if(commandProcessor->autoBlockNewInstances() && !mCore->blockNewInstances())
    {
        DriverError err(DriverError::AlreadyOpen);
        postError(err);
        mErrorStatus = err;
        finish();
        return;
    }

    //-Get Flashpoint Install-------------------------------------------------------------
    if(commandProcessor->requiresFlashpoint())
    {
        // Find and link to Flashpoint Install
        std::unique_ptr<Fp::Install> flashpointInstall;
        logEvent(LOG_EVENT_FLASHPOINT_SEARCH);

        if(!(flashpointInstall = findFlashpointInstall()))
        {
            DriverError err(DriverError::InvalidInstall, ERR_INSTALL_INVALID_TIP);
            postError(err);
            mErrorStatus = err;
            finish();
            return;
        }
        logEvent(LOG_EVENT_FLASHPOINT_LINK.arg(QDir::toNativeSeparators(flashpointInstall->dir().absolutePath())));

        // Insert into core
        mCore->attachFlashpoint(std::move(flashpointInstall));
    }

    //-Catch early core errors-------------------------------------------------------------------
    QThread::msleep(100);
    QApplication::processEvents();
    if(mErrorStatus.isSet())
    {
        finish();
        return;
    }

    //-Process command-----------------------------------------------------------------------------
    mErrorStatus = commandProcessor->process(mArguments);
    if(mErrorStatus.isSet())
    {
        finish();
        return;
    }

    //-Handle Tasks-----------------------------------------------------------------------
    logEvent(LOG_EVENT_TASK_COUNT.arg(mCore->taskCount()));
    if(mCore->hasTasks())
    {
        // Process task queue
        logEvent(LOG_EVENT_QUEUE_START);
        startNextTask();
    }
    else
        finish();
}

void Driver::cancelActiveLongTask()
{
    if(mCurrentTask)
        mCurrentTask->stop();
}

void Driver::quitNow()
{
    // Handle quit state
    if(mQuitRequested)
    {
        logEvent(LOG_EVENT_QUIT_REQUEST_REDUNDANT);
        return;
    }

    logEvent(LOG_EVENT_QUIT_REQUEST);
    quit();
}
