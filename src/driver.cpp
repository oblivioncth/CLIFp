// Unit Include
#include "driver.h"

// Qt Includes
#include <QDesktopServices>
#include <QProgressDialog>

// Qx Includes
#include <qx/windows/qx-common-windows.h>

// Project Includes
#include "command.h"

//===============================================================================================================
// DRIVER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
Driver::Driver(QStringList arguments, QString rawArguments) :
    mArguments(arguments),
    mRawArguments(rawArguments),
    mErrorStatus(Core::ErrorCodes::NO_ERR),
    mCurrentTaskNumber(-1),
    mQuitRequested(false),
    mCore(nullptr),
    mMainBlockingProcess(nullptr),
    mDownloadManager(nullptr)
{}

//-Class Functions----------------------------------------------------------------
//Private:
QString Driver::getRawCommandLineParams(const QString& rawCommandLine)
{
    // Remove application path, based on WINE
    // https://github.com/wine-mirror/wine/blob/a10267172046fc16aaa1cd1237701f6867b92fc0/dlls/shcore/main.c#L259
    QString::const_iterator rawIt = rawCommandLine.constBegin();
    if (*rawIt == '"')
    {
        ++rawIt;
        while (rawIt != rawCommandLine.constEnd())
            if (*rawIt++ == '"')
                break;
    }
    else
        while (rawIt != rawCommandLine.constEnd() && *rawIt != ' ' && *rawIt != '\t')
            ++rawIt;

    // Remove spaces before first arg
    while (*rawIt == ' ' || *rawIt == '\t')
        ++rawIt;

    // Return cropped string
    return QString(rawIt);
}

//-Instance Functions-------------------------------------------------------------
//Private:
void Driver::init()
{
    // Create core & download manager
    mCore = new Core(this, getRawCommandLineParams(mRawArguments));
    mDownloadManager = new Qx::AsyncDownloadManager(this);

    //-Setup Self Connections---------------
    connect(this, &Driver::__currentTaskFinished, this, &Driver::finishedTaskHandler, Qt::QueuedConnection); // This way external events are processed between tasks

    //-Setup Core---------------------------
    connect(mCore, &Core::statusChanged, this, &Driver::statusChanged);
    connect(mCore, &Core::errorOccured, this, &Driver::errorOccured);
    connect(mCore, &Core::blockingErrorOccured, this, &Driver::blockingErrorOccured);
    connect(mCore, &Core::message, this, &Driver::message);


    //-Setup Download Manager---------------------------
    mDownloadManager->setOverwrite(true);

    // Download event handlers
    connect(mDownloadManager, &Qx::AsyncDownloadManager::sslErrors, this, [this](Qx::GenericError errorMsg, bool* ignore) {
        int choice = mCore->postBlockingError(NAME, errorMsg, true, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        *ignore = choice == QMessageBox::Yes;
    });

    connect(mDownloadManager, &Qx::AsyncDownloadManager::authenticationRequired, this, [this](QString prompt, QAuthenticator* authenticator) {
        mCore->logEvent(NAME, LOG_EVENT_DOWNLOAD_AUTH);
        emit authenticationRequired(prompt, authenticator);
    });

    connect(mDownloadManager, &Qx::AsyncDownloadManager::proxyAuthenticationRequired, this, [this](QString prompt, QAuthenticator* authenticator) {
        mCore->logEvent(NAME, LOG_EVENT_DOWNLOAD_AUTH);
        emit authenticationRequired(prompt, authenticator);
    });

    connect(mDownloadManager, &Qx::AsyncDownloadManager::downloadTotalChanged, this, &Driver::downloadTotalChanged);
    connect(mDownloadManager, &Qx::AsyncDownloadManager::downloadProgress, this, &Driver::downloadProgressChanged);
    connect(mDownloadManager, &Qx::AsyncDownloadManager::finished, this, &Driver::finishedDownloadHandler);
}

void Driver::processExecTask(const std::shared_ptr<Core::ExecTask> task)
{
    // Emit complete signal on return unless dismissed
    QScopeGuard autoFinishEmitter([this](){ emit __currentTaskFinished(); });

    // Ensure executable exists
    QFileInfo executableInfo(task->path + "/" + task->filename);
    if(!executableInfo.exists() || !executableInfo.isFile())
    {
        mCore->postError(NAME, Qx::GenericError(task->stage == Core::TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                        ERR_EXE_NOT_FOUND.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
        handleTaskError(Core::ErrorCodes::EXECUTABLE_NOT_FOUND);
        return;
    }

    // Ensure executable is valid
    if(!executableInfo.isExecutable())
    {
        mCore->postError(NAME, Qx::GenericError(task->stage == Core::TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                        ERR_EXE_NOT_VALID.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
        handleTaskError(Core::ErrorCodes::EXECUTABLE_NOT_VALID);
        return;
    }

    // Move to executable directory
    QDir::setCurrent(task->path);
    mCore->logEvent(NAME, LOG_EVENT_CD.arg(QDir::toNativeSeparators(task->path)));

    // Check if task is a batch file
    bool batchTask = QFileInfo(task->filename).suffix() == BAT_SUFX;

    // Create process handle
    QProcess* taskProcess = new QProcess();
    taskProcess->setProgram(batchTask ? CMD_EXE : task->filename);
    taskProcess->setArguments(task->param);
    taskProcess->setNativeArguments(batchTask ? CMD_ARG_TEMPLATE.arg(task->filename, escapeNativeArgsForCMD(task->nativeParam))
                                              : task->nativeParam);
    taskProcess->setStandardOutputFile(QProcess::nullDevice()); // Don't inhert console window
    taskProcess->setStandardErrorFile(QProcess::nullDevice()); // Don't inhert console window

    // Cover each process type
    switch(task->processType)
    {
        case Core::ProcessType::Blocking:
            taskProcess->setParent(this);
            if(!cleanStartProcess(taskProcess, executableInfo))
            {
                handleTaskError(Core::ErrorCodes::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, Core::ProcessType::Blocking);

            // Now wait on the process asynchronously
            mMainBlockingProcess = taskProcess;
            connect(mMainBlockingProcess, &QProcess::finished, this, &Driver::finishedBlockingExecutionHandler);
            autoFinishEmitter.dismiss();
            break;

        case Core::ProcessType::Deferred:
            taskProcess->setParent(this);
            if(!cleanStartProcess(taskProcess, executableInfo))
            {
                handleTaskError(Core::ErrorCodes::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, Core::ProcessType::Deferred);

            mActiveChildProcesses.append(taskProcess); // Add process to list for deferred termination
            break;

        case Core::ProcessType::Detached:
            if(!taskProcess->startDetached())
            {
                handleTaskError(Core::ErrorCodes::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, Core::ProcessType::Detached);
            break;
    }
}

void Driver::processMessageTask(const std::shared_ptr<Core::MessageTask> task)
{
    mCore->postMessage(task->message);
    mCore->logEvent(NAME, LOG_EVENT_SHOW_MESSAGE);
    emit __currentTaskFinished();
}

void Driver::processExtraTask(const std::shared_ptr<Core::ExtraTask> task)
{
    // Ensure extra exists
    if(task->dir.exists())
    {
        // Open extra
        QDesktopServices::openUrl(QUrl::fromLocalFile(task->dir.absolutePath()));
        mCore->logEvent(NAME, LOG_EVENT_SHOW_EXTRA.arg(QDir::toNativeSeparators(task->dir.path())));
    }
    else
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(QDir::toNativeSeparators(task->dir.path()))));
        handleTaskError(Core::ErrorCodes::EXTRA_NOT_FOUND);
    }

    emit __currentTaskFinished();
}

void Driver::processWaitTask(const std::shared_ptr<Core::WaitTask> task)
{
    // Create waiter
    mProcessWaiter = new ProcessWaiter(task->processName, SECURE_PLAYER_GRACE, this);

    // Connect notifiers
    connect(mProcessWaiter, &ProcessWaiter::statusChanged, this,  [this](QString statusMessage){ mCore->logEvent(NAME, statusMessage); });
    connect(mProcessWaiter, &ProcessWaiter::errorOccured, this, [this](Qx::GenericError errorMessage){ mCore->postError(NAME, errorMessage); });
    connect(mProcessWaiter, &ProcessWaiter::waitFinished, this, &Driver::finishedWaitHandler);

    // Start wait
    mProcessWaiter->start();
}

void Driver::processDownloadTask(const std::shared_ptr<Core::DownloadTask> task)
{
    // Setup download
    QFile packFile(task->destPath + "/" + task->destFileName);
    QFileInfo packFileInfo(packFile);
    mDownloadManager->appendTask(Qx::DownloadTask{task->targetFile, packFileInfo.absoluteFilePath()});

    // Log/label string
    QString label = LOG_EVENT_DOWNLOADING_DATA_PACK.arg(packFileInfo.fileName());
    mCore->logEvent(NAME, label);

    // Start download
    emit downloadStarted(label);
    mDownloadManager->processQueue();
}

void Driver::startNextTask()
{
    // Ensure tasks exist
    if(!mCore->hasTasks())
        throw std::runtime_error(Q_FUNC_INFO " called with no tasks remaining.");

    // Get task at front of queue
    std::shared_ptr<Core::Task> currentTask = mCore->frontTask();
    mCurrentTaskNumber++;
    mCore->logEvent(NAME, LOG_EVENT_TASK_START.arg(mCurrentTaskNumber).arg(ENUM_NAME(currentTask->stage)));

    // Only execute task after an error/quit if it is a Shutdown task
    bool isShutdown = currentTask->stage == Core::TaskStage::Shutdown;
    if(mErrorStatus.isSet() && !isShutdown)
    {
        mCore->logEvent(NAME, LOG_EVENT_TASK_SKIP_ERROR);
        emit __currentTaskFinished(); // Since task was skipped
    }
    else if(mQuitRequested && !isShutdown)
    {
        mCore->logEvent(NAME, LOG_EVENT_TASK_SKIP_QUIT);
        emit __currentTaskFinished(); // Since task was skipped
    }
    if(!mErrorStatus.isSet() || currentTask->stage == Core::TaskStage::Shutdown)
    {
        // Cover each task type
        if(auto messageTask = std::dynamic_pointer_cast<Core::MessageTask>(currentTask)) // Message
            processMessageTask(messageTask);
        else if(auto extraTask = std::dynamic_pointer_cast<Core::ExtraTask>(currentTask)) // Extra
            processExtraTask(extraTask);
        else if(auto waitTask = std::dynamic_pointer_cast<Core::WaitTask>(currentTask)) // Wait task
            processWaitTask(waitTask);
        else if(auto downloadTask = std::dynamic_pointer_cast<Core::DownloadTask>(currentTask)) // Download task
            processDownloadTask(downloadTask);
        else if(auto execTask = std::dynamic_pointer_cast<Core::ExecTask>(currentTask)) // Execution task
            processExecTask(execTask);
        else
            throw std::runtime_error("Unhandled Task child was present in task queue");
    }
}

void Driver::handleTaskError(ErrorCode error)
{
    mErrorStatus = error;
    mCore->logEvent(NAME, LOG_EVENT_TASK_FINISH_ERR.arg(mCurrentTaskNumber)); // Record early end of task
}

bool Driver::cleanStartProcess(QProcess* process, QFileInfo exeInfo)
{
    // Start process and confirm
    process->start();
    if(!process->waitForStarted())
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical,
                                   ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(exeInfo.absoluteFilePath()))));
        delete process; // Clear finished process handle from heap
        return false;
    }

    // Return success
    return true;
}

void Driver::cleanup()
{
    // Close each remaining child process
    for(QProcess* childProcess : qAsConst(mActiveChildProcesses))
    {
        childProcess->close();
        logProcessEnd(childProcess, Core::ProcessType::Deferred);
        delete childProcess;
    }

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

QString Driver::escapeNativeArgsForCMD(QString nativeArgs)
{
    static const QSet<QChar> escapeChars{'^','&','<','>','|'};
    QString escapedNativeArgs;
    bool inQuotes = false;

    for(int i = 0; i < nativeArgs.size(); i++)
    {
        const QChar& chr = nativeArgs.at(i);
        if(chr== '"' && (inQuotes || i != nativeArgs.lastIndexOf('"')))
            inQuotes = !inQuotes;

        escapedNativeArgs.append((!inQuotes && escapeChars.contains(chr)) ? '^' + chr : chr);
    }

    if(nativeArgs != escapedNativeArgs)
        mCore->logEvent(NAME, LOG_EVENT_ARGS_ESCAPED.arg(nativeArgs, escapedNativeArgs));

    return escapedNativeArgs;
}


void Driver::logProcessStart(const QProcess* process, Core::ProcessType type)
{
    QString eventStr = process->program();
    if(!process->arguments().isEmpty())
        eventStr += " " + process->arguments().join(" ");
    if(!process->nativeArguments().isEmpty())
        eventStr += " " + process->nativeArguments();

    mCore->logEvent(NAME, LOG_EVENT_START_PROCESS.arg(ENUM_NAME(type), eventStr));
}

void Driver::logProcessEnd(const QProcess* process, Core::ProcessType type)
{
    mCore->logEvent(NAME, LOG_EVENT_END_PROCESS.arg(ENUM_NAME(type), process->program()));
}

//-Slots--------------------------------------------------------------------------------
//Private:
void Driver::finishedBlockingExecutionHandler()
{
    // Ensure all is well
    if(!mMainBlockingProcess)
        throw std::runtime_error(Q_FUNC_INFO " called with when the main blocking process pointer was null.");

    // Handle process cleanup
    logProcessEnd(mMainBlockingProcess, Core::ProcessType::Blocking);
    mMainBlockingProcess->deleteLater(); // Clear finished process handle from heap when possible
    mMainBlockingProcess = nullptr;

    emit __currentTaskFinished();
}
void Driver::finishedDownloadHandler(Qx::DownloadManagerReport downloadReport)
{
    // Handle result
    emit downloadFinished(downloadReport.outcome() == Qx::DownloadManagerReport::Outcome::Abort);
    if(downloadReport.wasSuccessful())
    {
        // Get task completed download task
        std::shared_ptr<Core::DownloadTask> downloadTask;
        if(!(downloadTask = std::dynamic_pointer_cast<Core::DownloadTask>(mCore->frontTask())))
            throw std::runtime_error(Q_FUNC_INFO "called when top task of queue is not a DownloadTask.");

        // Confirm checksum is correct
        QFile packFile(downloadTask->destPath + "/" + downloadTask->destFileName);
        bool checksumMatch;
        Qx::IoOpReport checksumResult = Qx::fileMatchesChecksum(checksumMatch, packFile, downloadTask->sha256, QCryptographicHash::Sha256);
        if(checksumResult.isFailure() || !checksumMatch)
        {
            mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_PACK_SUM_MISMATCH));
            handleTaskError(Core::ErrorCodes::DATA_PACK_INVALID);
        }
        else
            mCore->logEvent(NAME, LOG_EVENT_DOWNLOAD_SUCC);
    }
    else
    {
        downloadReport.errorInfo().setErrorLevel(Qx::GenericError::Critical);
        mCore->postError(NAME, downloadReport.errorInfo());
        handleTaskError(Core::ErrorCodes::CANT_OBTAIN_DATA_PACK);
    }

    emit __currentTaskFinished();
}

void Driver::finishedWaitHandler(ErrorCode errorStatus)
{
    // Handle potential error
    if(errorStatus)
        handleTaskError(errorStatus);

    // Delete waiter when possible
    mProcessWaiter->deleteLater();
    mProcessWaiter = nullptr;

    emit __currentTaskFinished();
}

void Driver::finishedTaskHandler()
{
    // Remove task from queue
    mCore->removeFrontTask();

    // Note handled task
    mCore->logEvent(NAME, LOG_EVENT_TASK_FINISH.arg(mCurrentTaskNumber));

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
        mErrorStatus = Core::ErrorCodes::ALREADY_OPEN;
        finish();
        return;
    }

    // Ensure Flashpoint Launcher isn't running
    if(Qx::processIsRunning(Fp::Install::LAUNCHER_INFO.fileName()))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_LAUNCHER_RUNNING_P, ERR_LAUNCHER_RUNNING_S));
        mErrorStatus = Core::ErrorCodes::LAUNCHER_OPEN;
        finish();
        return;
    }

    //-Find and link to Flashpoint Install----------------------------------------------------------
    std::unique_ptr<Fp::Install> flashpointInstall;
    mCore->logEvent(NAME, LOG_EVENT_FLASHPOINT_SEARCH);

    if(!(flashpointInstall = findFlashpointInstall()))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_INSTALL_INVALID_P, ERR_INSTALL_INVALID_S));
        mErrorStatus = Core::ErrorCodes::INSTALL_INVALID;
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
        mErrorStatus = Core::ErrorCodes::INVALID_ARGS;
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
        // Process app task queue
        mCore->logEvent(NAME, LOG_EVENT_QUEUE_START);
        startNextTask();
    }
    else
        finish();
}

void Driver::cancelActiveDownloads() { mDownloadManager->abort(); }

void Driver::closeNow()
{
    mQuitRequested = true;
    mCore->logEvent(NAME, LOG_EVENT_CLOSE_REQUEST);

    //-Handle all potential cases in which CLIFp is waiting for something---------------

    // Downloading
    if(mDownloadManager->isProcessing())
        mDownloadManager->abort();

    // Main process running
    if(mMainBlockingProcess) // May want to add && isOpen() here, but not sure if that is implemented correctly
    {
        // Try soft kill
        mMainBlockingProcess->terminate();
        if(mMainBlockingProcess->state() != QProcess::NotRunning)
            mMainBlockingProcess->waitForFinished(2000); // Allow up to 2 seconds to close

        /* See if process stopped. Can't use result of QProcess::waitForFinished directly since there is a small
         * race condition in which the process ends between the previous check of its state and when
         * waitForFinished is invoked. If this happened, waitForFinished would return false and give the impression
         * that the process was still running
         */
        if(mMainBlockingProcess->state() != QProcess::NotRunning)
            mMainBlockingProcess->terminate(); // Hard kill
    }

    // Waiting on restarted main process
    if(mProcessWaiter)
    {
        if(!mProcessWaiter->closeProcess())
            mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Error, ERR_CANT_CLOSE_WAIT_ON));
    }

}
