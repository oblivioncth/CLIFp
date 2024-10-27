// Unit Include
#include "kernel/driver.h"
#include "driver_p.h"

// Qt Includes
#include <QCoreApplication>

// Qx Includes
#include <qx/core/qx-system.h>
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "kernel/core.h"
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
// DriverPrivate
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
DriverPrivate::DriverPrivate(Driver* q, QStringList arguments) :
    q_ptr(q),
    mArguments(arguments),
    mCore(nullptr),
    mErrorStatus(),
    mCurrentTask(nullptr),
    mCurrentTaskNumber(-1),
    mQuitRequested(false)
{
    // Required in order to ensure the commands aren't discarded when this is a static lib
    Command::registerAllCommands();
}

//-Instance Functions-------------------------------------------------------------
//Private:
QString DriverPrivate::name() const { return NAME; }

ErrorCode DriverPrivate::logFinish(const Qx::Error& errorState) const
{
    Director* dtor = director();
    Q_ASSERT(dtor);
    return dtor->logFinish(name(), errorState);
}

void DriverPrivate::init()
{
    Q_Q(Driver);
    // Create core, attach director to self
    mCore = std::make_unique<Core>();
    Director* dtor = mCore->director();
    setDirector(mCore->director());

    //-Setup Core & Director---------------------------
    QObject::connect(mCore.get(), &Core::abort, q, [this](CoreError err){
        logEvent(LOG_EVENT_CORE_ABORT);
        mErrorStatus = err;
        quit();
    });
    QObject::connect(dtor, &Director::announceAsyncDirective, q, &Driver::asyncDirectiveAccounced);
    QObject::connect(dtor, &Director::announceSyncDirective, q, &Driver::syncDirectiveAccounced);

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
    DeferredProcessManager* dpm = new DeferredProcessManager(*mCore);
    TExec::installDeferredProcessManager(dpm);
}

void DriverPrivate::startNextTask()
{
    Q_Q(Driver);

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
    bool erroring = mErrorStatus.isSet();
    bool qutting = mQuitRequested;
    bool skip = (erroring || qutting) && !isShutdown;

    if(skip)
    {
        logEvent(erroring ? LOG_EVENT_TASK_SKIP_ERROR : LOG_EVENT_TASK_SKIP_QUIT);

        // Queue up finished handler directly (executes on next event loop cycle) since task was skipped
        // Can't connect directly because newer connect syntax doesn't support default args
        QTimer::singleShot(0, q, [this](){ completeTaskHandler(); });
    }
    else
    {
        // QueuedConnection, allow event processing between tasks
        QObject::connect(mCurrentTask, &Task::complete, q, [this](const Qx::Error& e){
            completeTaskHandler(e);
        }, Qt::QueuedConnection);

        // Perform task
        mCurrentTask->perform();
    }
}

void DriverPrivate::cleanup()
{
    logEvent(LOG_EVENT_CLEANUP_START);

    // Close each remaining child process
    logEvent(LOG_EVENT_ENDING_CHILD_PROCESSES);
    TExec::deferredProcessManager()->closeProcesses();

    logEvent(LOG_EVENT_CLEANUP_FINISH);
}

void DriverPrivate::finish()
{
    Q_Q(Driver);

    logEvent(LOG_EVENT_FINISH);

    // Clear update cache
    if(CUpdate::isUpdateCacheClearable())
    {
        if(CUpdateError err = CUpdate::clearUpdateCache(); err.isValid())
            logError(err);
        else
            logEvent(LOG_EVENT_CLEARED_UPDATE_CACHE);
    }

    emit q->finished(logFinish(mErrorStatus.value()));
}

void DriverPrivate::quit()
{
    mQuitRequested = true;

    // Stop current task (assuming it can be)
    if(mCurrentTask)
        mCurrentTask->stop();
}

// Helper functions
std::unique_ptr<Fp::Install> DriverPrivate::findFlashpointInstall()
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

//-Slots--------------------------------------------------------------------------------
//Private:
void DriverPrivate::completeTaskHandler(const Qx::Error& e)
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
void DriverPrivate::drive()
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
        postDirective<DError>(ce);
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
        postDirective<DError>(err);
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
            postDirective<DError>(err);
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
    QCoreApplication::processEvents();
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

void DriverPrivate::cancelActiveLongTask()
{
    if(mCurrentTask)
        mCurrentTask->stop();
}

void DriverPrivate::quitNow()
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

//===============================================================================================================
// Driver
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
Driver::Driver(QStringList arguments) : d_ptr(std::make_unique<DriverPrivate>(this, arguments)) {}

//-Destructor--------------------------------------------------------------------
//Public:
Driver::~Driver() = default;

//-Slots--------------------------------------------------------------------------------
//Public:
void Driver::drive() { Q_D(Driver); d->drive(); }
void Driver::cancelActiveLongTask() { Q_D(Driver); d->cancelActiveLongTask(); }
void Driver::quitNow() { Q_D(Driver); d->quitNow(); }
