// Unit Include
#include "driver.h"

// Qx Includes
#include <qx/core/qx-system.h>
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "command/command.h"
#include "task/t-exec.h"
#include "utility.h"

//===============================================================================================================
// DRIVER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
Driver::Driver(QStringList arguments) :
    mArguments(arguments),
    mErrorStatus(ErrorCode::NO_ERR),
    mCurrentTaskNumber(-1),
    mQuitRequested(false),
    mCore(nullptr)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
void Driver::init()
{
    // Create core
    mCore = new Core(this);

    //-Setup Core---------------------------
    connect(mCore, &Core::statusChanged, this, &Driver::statusChanged);
    connect(mCore, &Core::errorOccured, this, &Driver::errorOccured);
    connect(mCore, &Core::blockingErrorOccured, this, &Driver::blockingErrorOccured);
    connect(mCore, &Core::message, this, &Driver::message);
    connect(mCore, &Core::saveFileRequested, this, &Driver::saveFileRequested);
    connect(mCore, &Core::itemSelectionRequested, this, &Driver::itemSelectionRequested);

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
    connect(dpm, &DeferredProcessManager::eventOccurred, mCore, &Core::logEvent);
    connect(dpm, &DeferredProcessManager::errorOccurred, mCore, &Core::logError);
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
    mCore->logEvent(NAME, LOG_EVENT_TASK_START.arg(QString::number(mCurrentTaskNumber),
                                                   mCurrentTask->name(),
                                                   ENUM_NAME(mCurrentTask->stage())));

    // Only execute task after an error/quit if it is a Shutdown task
    bool isShutdown = mCurrentTask->stage() == Task::Stage::Shutdown;
    if(mErrorStatus.isSet() && !isShutdown)
    {
        mCore->logEvent(NAME, LOG_EVENT_TASK_SKIP_ERROR);

        // Queue up finished handler directly (executes on next event loop cycle) since task was skipped
        // Can't connect directly because newer connect syntax doesn't support default args
        QTimer::singleShot(0, this, [this](){ completeTaskHandler(); });
    }
    else if(mQuitRequested && !isShutdown)
    {
        mCore->logEvent(NAME, LOG_EVENT_TASK_SKIP_QUIT);

        // Queue up finished handler directly (executes on next event loop cycle) since task was skipped
        // Can't connect directly because newer connect syntax doesn't support default args
        QTimer::singleShot(0, this, [this](){ completeTaskHandler(); });
    }
    else
    {
        // Connect task notifiers
        connect(mCurrentTask, &Task::notificationReady, mCore, &Core::postMessage);
        connect(mCurrentTask, &Task::eventOccurred, mCore, &Core::logEvent);
        connect(mCurrentTask, &Task::errorOccurred, mCore, [this](QString taskName, Qx::GenericError error){
            mCore->postError(taskName, error); // Can't connect directly because newer connect syntax doesn't support default args
        });
        connect(mCurrentTask, &Task::blockingErrorOccured, this,
                [this](QString taskName, int* response, Qx::GenericError error, QMessageBox::StandardButtons choices) {
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
    mCore->logEvent(NAME, LOG_EVENT_CLEANUP_START);

    // Close each remaining child process
    mCore->logEvent(NAME, LOG_EVENT_ENDING_CHILD_PROCESSES);
    TExec::deferredProcessManager()->closeProcesses();

    mCore->logEvent(NAME, LOG_EVENT_CLEANUP_FINISH);
}

void Driver::finish() { emit finished(mCore->logFinish(NAME, mErrorStatus.value())); }

// Helper functions
std::unique_ptr<Fp::Install> Driver::findFlashpointInstall()
{
    QDir currentDir(CLIFP_DIR_PATH);
    std::unique_ptr<Fp::Install> fpInstall;

    do
    {
        mCore->logEvent(NAME, LOG_EVENT_FLASHPOINT_ROOT_CHECK.arg(QDir::toNativeSeparators(currentDir.absolutePath())));

        // Attempt to instantiate
        fpInstall = std::make_unique<Fp::Install>(currentDir.absolutePath());
        if(fpInstall->isValid())
            break;
        else
            fpInstall.reset();
    }
    while(currentDir.cdUp());

    // Return instance, null or not
    return std::move(fpInstall);
}



//-Slots--------------------------------------------------------------------------------
//Private:
void Driver::completeTaskHandler(ErrorCode ec)
{
    // Handle errors
    if(ec != ErrorCode::NO_ERR)
    {
        mErrorStatus = ec;
        mCore->logEvent(NAME, LOG_EVENT_TASK_FINISH_ERR.arg(mCurrentTaskNumber)); // Record early end of task
    }

    // Cleanup handled task
    mCore->logEvent(NAME, LOG_EVENT_TASK_FINISH.arg(mCurrentTaskNumber));
    qxDelete(mCurrentTask);

    // Perform next task if any remain
    if(mCore->hasTasks())
        startNextTask();
    else
    {
        mCore->logEvent(NAME, LOG_EVENT_QUEUE_FINISH);
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

    //-Restrict app to only one instance---------------------------------------------------
    if(!Qx::enforceSingleInstance(SINGLE_INSTANCE_ID))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ALREADY_OPEN));
        mErrorStatus = ErrorCode::ALREADY_OPEN;
        finish();
        return;
    }

    // Ensure Flashpoint Launcher isn't running
    if(Qx::processIsRunning(Fp::Install::LAUNCHER_NAME))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_LAUNCHER_RUNNING_P, ERR_LAUNCHER_RUNNING_S));
        mErrorStatus = ErrorCode::LAUNCHER_OPEN;
        finish();
        return;
    }

    //-Find and link to Flashpoint Install----------------------------------------------------------
    std::unique_ptr<Fp::Install> flashpointInstall;
    mCore->logEvent(NAME, LOG_EVENT_FLASHPOINT_SEARCH);

    if(!(flashpointInstall = findFlashpointInstall()))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_INSTALL_INVALID_P, ERR_INSTALL_INVALID_S));
        mErrorStatus = ErrorCode::INSTALL_INVALID;
        finish();
        return;
    }
    mCore->logEvent(NAME, LOG_EVENT_FLASHPOINT_LINK.arg(QDir::toNativeSeparators(flashpointInstall->fullPath())));

    // Insert into core
    mCore->attachFlashpoint(std::move(flashpointInstall));

    //-Handle Command and Command Options----------------------------------------------------------
    QString commandStr = mArguments.first().toLower();

    // Check for valid command
    if(!Command::isRegistered(commandStr))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_INVALID_COMMAND.arg(commandStr)));
        mErrorStatus = ErrorCode::INVALID_ARGS;
        finish();
        return;
    }

    // Create command instance
    std::unique_ptr<Command> commandProcessor = Command::acquire(commandStr, *mCore);

    // Process command
    mErrorStatus = commandProcessor->process(mArguments);
    if(mErrorStatus.isSet())
    {
        finish();
        return;
    }

    //-Handle Tasks-----------------------------------------------------------------------
    mCore->logEvent(NAME, LOG_EVENT_TASK_COUNT.arg(mCore->taskCount()));
    if(mCore->hasTasks())
    {
        // Process task queue
        mCore->logEvent(NAME, LOG_EVENT_QUEUE_START);
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
        mCore->logEvent(NAME, LOG_EVENT_QUIT_REQUEST_REDUNDANT);
        return;
    }

    mQuitRequested = true;
    mCore->logEvent(NAME, LOG_EVENT_QUIT_REQUEST);

    // Stop current task (assuming it can be)
    if(mCurrentTask)
        mCurrentTask->stop();
}