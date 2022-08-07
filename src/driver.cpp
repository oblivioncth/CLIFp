// Unit Include
#include "driver.h"

// Qt Includes
#include <QDesktopServices>
#include <QProgressDialog>

// Qx Includes
#include <qx/windows/qx-common-windows.h>
#include <qx_windows.h>

// Project Includes
#include "command.h"

//===============================================================================================================
// DRIVER
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
Driver::Driver(QStringList arguments, QString rawArguments) :
    mArguments(arguments),
    mRawArguments(rawArguments),
    mErrorStatus(Core::ErrorCodes::NO_ERR)
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
    mDownloadManager = new Qx::SyncDownloadManager(this);

    //-Setup Core---------------------------
    connect(mCore, &Core::statusChanged, this, &Driver::statusChanged);
    connect(mCore, &Core::errorOccured, this, &Driver::errorOccured);
    connect(mCore, &Core::blockingErrorOccured, this, &Driver::blockingErrorOccured);
    connect(mCore, &Core::message, this, &Driver::message);


    //-Setup Download Manager---------------------------
    mDownloadManager->setOverwrite(true);

    // Download event handlers
    connect(mDownloadManager, &Qx::SyncDownloadManager::sslErrors, this, [this](Qx::GenericError errorMsg, bool* ignore) {
        int choice = mCore->postBlockingError(NAME, errorMsg, true, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        *ignore = choice == QMessageBox::Yes;
    });

    connect(mDownloadManager, &Qx::SyncDownloadManager::authenticationRequired, this, [this](QString prompt, QAuthenticator* authenticator) {
        mCore->logEvent(NAME, LOG_EVENT_DOWNLOAD_AUTH);
        emit authenticationRequired(prompt, authenticator);
    });

    connect(mDownloadManager, &Qx::SyncDownloadManager::proxyAuthenticationRequired, this, [this](QString prompt, QAuthenticator* authenticator) {
        mCore->logEvent(NAME, LOG_EVENT_DOWNLOAD_AUTH);
        emit authenticationRequired(prompt, authenticator);
    });

    connect(mDownloadManager, &Qx::SyncDownloadManager::downloadTotalChanged, this, &Driver::downloadTotalChanged);
    connect(mDownloadManager, &Qx::SyncDownloadManager::downloadProgress, this, &Driver::downloadProgressChanged);
}

void Driver::processTaskQueue()
{
    mCore->logEvent(NAME, LOG_EVENT_QUEUE_START);

    // Exhaust queue
    for(int taskNum = 0; mCore->hasTasks(); taskNum++)
    {
        // Handle task at front of queue
        std::shared_ptr<Core::Task> currentTask = mCore->takeFrontTask();
        mCore->logEvent(NAME, LOG_EVENT_TASK_START.arg(taskNum).arg(ENUM_NAME(currentTask->stage)));

        // Only execute task after an error if it is a Shutdown task
        if(!mErrorStatus.isSet() || currentTask->stage == Core::TaskStage::Shutdown)
        {
            // Cover each task type
            if(std::dynamic_pointer_cast<Core::MessageTask>(currentTask))
            {
                std::shared_ptr<Core::MessageTask> messageTask = std::static_pointer_cast<Core::MessageTask>(currentTask);

                mCore->postMessage(messageTask->message);
                mCore->logEvent(NAME, LOG_EVENT_SHOW_MESSAGE);
            }
            else if(std::dynamic_pointer_cast<Core::ExtraTask>(currentTask)) // Extra
            {
                std::shared_ptr<Core::ExtraTask> extraTask = std::static_pointer_cast<Core::ExtraTask>(currentTask);

                // Ensure extra exists
                if(!extraTask->dir.exists())
                {
                    mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(QDir::toNativeSeparators(extraTask->dir.path()))));
                    handleExecutionError(taskNum, Core::ErrorCodes::EXTRA_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Open extra
                QDesktopServices::openUrl(QUrl::fromLocalFile(extraTask->dir.absolutePath()));
                mCore->logEvent(NAME, LOG_EVENT_SHOW_EXTRA.arg(QDir::toNativeSeparators(extraTask->dir.path())));
            }
            else if(std::dynamic_pointer_cast<Core::WaitTask>(currentTask)) // Wait task
            {
                std::shared_ptr<Core::WaitTask> waitTask = std::static_pointer_cast<Core::WaitTask>(currentTask);

                ErrorCode waitError = waitOnProcess(waitTask->processName, SECURE_PLAYER_GRACE);
                if(waitError)
                {
                    handleExecutionError(taskNum, waitError);
                    continue; // Continue to next task
                }
            }
            else if(std::dynamic_pointer_cast<Core::DownloadTask>(currentTask)) // Download task
            {
                std::shared_ptr<Core::DownloadTask> downloadTask = std::static_pointer_cast<Core::DownloadTask>(currentTask);

                // Setup download
                QFile packFile(downloadTask->destPath + "/" + downloadTask->destFileName);
                QFileInfo packFileInfo(packFile);
                mDownloadManager->appendTask(Qx::DownloadTask{downloadTask->targetFile, packFileInfo.absoluteFilePath()});

                // Log/label string
                QString label = LOG_EVENT_DOWNLOADING_DATA_PACK.arg(packFileInfo.fileName());
                mCore->logEvent(NAME, label);

                // Start download
                emit downloadStarted(label);
                Qx::DownloadManagerReport downloadReport = mDownloadManager->processQueue(); // This spins an event loop

                // Handle result
                emit downloadFinished(downloadReport.outcome() == Qx::DownloadManagerReport::Outcome::Abort);
                if(!downloadReport.wasSuccessful())
                {
                    downloadReport.errorInfo().setErrorLevel(Qx::GenericError::Critical);
                    mCore->postError(NAME, downloadReport.errorInfo());
                    handleExecutionError(taskNum, Core::ErrorCodes::CANT_OBTAIN_DATA_PACK);
                    continue; // Continue to next task
                }

                // Confirm checksum
                bool checksumMatch;
                Qx::IoOpReport checksumResult = Qx::fileMatchesChecksum(checksumMatch, packFile, downloadTask->sha256, QCryptographicHash::Sha256);
                if(checksumResult.isFailure() || !checksumMatch)
                {
                    mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_PACK_SUM_MISMATCH));
                    handleExecutionError(taskNum, Core::ErrorCodes::DATA_PACK_INVALID);
                    continue; // Continue to next task
                }

                mCore->logEvent(NAME, LOG_EVENT_DOWNLOAD_SUCC);
            }
            else if(std::dynamic_pointer_cast<Core::ExecTask>(currentTask)) // Execution task
            {
                std::shared_ptr<Core::ExecTask> execTask = std::static_pointer_cast<Core::ExecTask>(currentTask);

                // Ensure executable exists
                QFileInfo executableInfo(execTask->path + "/" + execTask->filename);
                if(!executableInfo.exists() || !executableInfo.isFile())
                {
                    mCore->postError(NAME, Qx::GenericError(execTask->stage == Core::TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                                    ERR_EXE_NOT_FOUND.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(taskNum, Core::ErrorCodes::EXECUTABLE_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Ensure executable is valid
                if(!executableInfo.isExecutable())
                {
                    mCore->postError(NAME, Qx::GenericError(execTask->stage == Core::TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                                    ERR_EXE_NOT_VALID.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(taskNum, Core::ErrorCodes::EXECUTABLE_NOT_VALID);
                    continue; // Continue to next task
                }

                // Move to executable directory
                QDir::setCurrent(execTask->path);
                mCore->logEvent(NAME, LOG_EVENT_CD.arg(QDir::toNativeSeparators(execTask->path)));

                // Check if task is a batch file
                bool batchTask = QFileInfo(execTask->filename).suffix() == BAT_SUFX;

                // Create process handle
                QProcess* taskProcess = new QProcess();
                taskProcess->setProgram(batchTask ? CMD_EXE : execTask->filename);
                taskProcess->setArguments(execTask->param);
                taskProcess->setNativeArguments(batchTask ? CMD_ARG_TEMPLATE.arg(execTask->filename, escapeNativeArgsForCMD(execTask->nativeParam))
                                                          : execTask->nativeParam);
                taskProcess->setStandardOutputFile(QProcess::nullDevice()); // Don't inhert console window
                taskProcess->setStandardErrorFile(QProcess::nullDevice()); // Don't inhert console window

                // Cover each process type
                switch(execTask->processType)
                {
                    case Core::ProcessType::Blocking:
                        taskProcess->setParent(this);
                        if(!cleanStartProcess(taskProcess, executableInfo))
                        {
                            handleExecutionError(taskNum, Core::ErrorCodes::PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, Core::ProcessType::Blocking);

                        taskProcess->waitForFinished(-1); // Wait for process to end, don't time out
                        logProcessEnd(taskProcess, Core::ProcessType::Blocking);
                        delete taskProcess; // Clear finished process handle from heap
                        break;

                    case Core::ProcessType::Deferred:
                        taskProcess->setParent(this);
                        if(!cleanStartProcess(taskProcess, executableInfo))
                        {
                            handleExecutionError(taskNum, Core::ErrorCodes::PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, Core::ProcessType::Deferred);

                        mActiveChildProcesses.append(taskProcess); // Add process to list for deferred termination
                        break;

                    case Core::ProcessType::Detached:
                        if(!taskProcess->startDetached())
                        {
                            handleExecutionError(taskNum, Core::ErrorCodes::PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, Core::ProcessType::Detached);
                        break;
                }
            }
            else
                throw new std::runtime_error("Unhandled Task child was present in task queue");
        }
        else
            mCore->logEvent(NAME, LOG_EVENT_TASK_SKIP);

        // Remove handled task
        mCore->logEvent(NAME, LOG_EVENT_TASK_FINISH.arg(taskNum));
    }
}

void Driver::handleExecutionError(int taskNum, ErrorCode error)
{
    mErrorStatus = error;
    mCore->logEvent(NAME, LOG_EVENT_TASK_FINISH_ERR.arg(taskNum)); // Record early end of task
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

ErrorCode Driver::waitOnProcess(QString processName, int graceSecs)
{
    // Wait until secure player has stopped running for grace period
    DWORD spProcessID;
    do
    {
        // Yield for grace period
        mCore->logEvent(NAME, LOG_EVENT_WAIT_GRACE.arg(processName));
        if(graceSecs > 0)
            QThread::sleep(graceSecs);

        // Find process ID by name
        spProcessID = Qx::processIdByName(processName);

        // Check that process was found (is running)
        if(spProcessID)
        {
            mCore->logEvent(NAME, LOG_EVENT_WAIT_RUNNING.arg(processName));

            // Get process handle and see if it is valid
            HANDLE batchProcessHandle;
            if((batchProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, spProcessID)) == NULL)
            {
                mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HANDLED_P.arg(processName),
                                           WRN_WAIT_PROCESS_NOT_HANDLED_S));
                return Core::ErrorCodes::WAIT_PROCESS_NOT_HANDLED;
            }

            // Attempt to wait on process to terminate
            mCore->logEvent(NAME, LOG_EVENT_WAIT_ON.arg(processName));
            DWORD waitError = WaitForSingleObject(batchProcessHandle, INFINITE);

            // Close handle to process
            CloseHandle(batchProcessHandle);

            if(waitError != WAIT_OBJECT_0)
            {
                mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HOOKED_P.arg(processName),
                                           WRN_WAIT_PROCESS_NOT_HOOKED_S));
                return Core::ErrorCodes::WAIT_PROCESS_NOT_HOOKED;
            }
            mCore->logEvent(NAME, LOG_EVENT_WAIT_QUIT.arg(processName));
        }
    }
    while(spProcessID);

    // Return success
    mCore->logEvent(NAME, LOG_EVENT_WAIT_FINISHED.arg(processName));
    return Core::ErrorCodes::NO_ERR;
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
}

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
//Public:
void Driver::drive()
{
    // Initialize
    init();

    //-Initialize Core--------------------------------------------------------------------------
    mErrorStatus = mCore->initialize(mArguments);
    if(mErrorStatus.isSet() || mArguments.empty()) // Terminate if error or no command
    {
        emit finished(mCore->logFinish(NAME, mErrorStatus.value()));
        return;
    }

    //-Restrict app to only one instance---------------------------------------------------
    if(!Qx::enforceSingleInstance())
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ALREADY_OPEN));
        emit finished(mCore->logFinish(NAME, Core::ErrorCodes::ALREADY_OPEN));
        return;
    }

    // Ensure Flashpoint Launcher isn't running
    if(Qx::processIsRunning(Fp::Install::LAUNCHER_INFO.fileName()))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_LAUNCHER_RUNNING_P, ERR_LAUNCHER_RUNNING_S));
        emit finished(mCore->logFinish(NAME, Core::ErrorCodes::LAUNCHER_OPEN));
        return;
    }

    //-Find and link to Flashpoint Install----------------------------------------------------------
    std::unique_ptr<Fp::Install> flashpointInstall;
    mCore->logEvent(NAME, LOG_EVENT_FLASHPOINT_SEARCH);

    if(!(flashpointInstall = findFlashpointInstall()))
    {
        mCore->postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_INSTALL_INVALID_P, ERR_INSTALL_INVALID_S));
        emit finished(mCore->logFinish(NAME, Core::ErrorCodes::INSTALL_INVALID));
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
        emit finished(mCore->logFinish(NAME, Core::ErrorCodes::INVALID_ARGS));
        return;
    }

    // Create command instance
    std::unique_ptr<Command> commandProcessor = Command::acquire(commandStr, *mCore);

    // Process command
    mErrorStatus = commandProcessor->process(mArguments);
    if(mErrorStatus.isSet())
    {
        emit finished(mCore->logFinish(NAME, mErrorStatus.value()));
        return;
    }

    //-Handle Tasks-----------------------------------------------------------------------
    mCore->logEvent(NAME, LOG_EVENT_TASK_COUNT.arg(mCore->taskCount()));
    if(mCore->hasTasks())
    {
        // Process app task queue
        processTaskQueue();
        mCore->logEvent(NAME, LOG_EVENT_QUEUE_FINISH);

        // Cleanup
        cleanup();
        mCore->logEvent(NAME, LOG_EVENT_CLEANUP_FINISH);

        // Return error status
        emit finished(mCore->logFinish(NAME, mErrorStatus.value()));
        return;
    }
    else
    {
        emit finished(mCore->logFinish(NAME, Core::ErrorCodes::NO_ERR));
        return;
    }
}

void Driver::cancelActiveDownloads() { mDownloadManager->abort(); }
