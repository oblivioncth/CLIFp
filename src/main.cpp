#include <QApplication>
#include <QProcess>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QProgressDialog>
#include <queue>

#include "qx-windows.h"
#include "qx-io.h"
#include "qx-net.h"
#include "qx-widgets.h"

#include "version.h"
#include "core.h"
#include "command.h"
#include "flashpoint-install.h"
#include "logger.h"


// Error Messages - Prep
const QString ERR_ALREADY_OPEN = "Only one instance of CLIFp can be used at a time!";
const QString ERR_LAUNCHER_RUNNING_P = "The CLI cannot be used while the Flashpoint Launcher is running.";
const QString ERR_LAUNCHER_RUNNING_S = "Please close the Launcher first.";
const QString ERR_INSTALL_INVALID_P = "CLIFp does not appear to be deployed in a valid Flashpoint install";
const QString ERR_INSTALL_INVALID_S = "Check its location and compatability with your Flashpoint version.";

// Error Messages - Execution
const QString ERR_EXTRA_NOT_FOUND = "The extra %1 does not exist!";
const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";
const QString ERR_PACK_SUM_MISMATCH = "The title's Data Pack checksum does not match its record!";
const QString WRN_WAIT_PROCESS_NOT_HANDLED_P  = "Could not get a wait handle to %1.";
const QString WRN_WAIT_PROCESS_NOT_HANDLED_S = "The title may not work correctly";
const QString WRN_WAIT_PROCESS_NOT_HOOKED_P  = "Could not hook %1 for waiting.";
const QString WRN_WAIT_PROCESS_NOT_HOOKED_S = "The title may not work correctly";


// Suffixes
const QString BAT_SUFX = "bat";
const QString EXE_SUFX = "exe";

// Processing constants
const QString CMD_EXE = "cmd.exe";
const QString CMD_ARG_TEMPLATE = R"(/d /s /c ""%1" %2")";

// Wait timing
const int SECURE_PLAYER_GRACE = 2; // Seconds to allow the secure player to restart in cases it does

// Logging - Messages
const QString LOG_EVENT_FLASHPOINT_SEARCH = "Searching for Flashpoint root...";
const QString LOG_EVENT_FLASHPOINT_ROOT_CHECK = "Checking if \"%1\" is flashpoint root";
const QString LOG_EVENT_FLASHPOINT_LINK = "Linked to Flashpoint install at: %1";
const QString LOG_EVENT_SHOW_MESSAGE = "Displayed message";
const QString LOG_EVENT_SHOW_EXTRA = "Opened folder of extra %1";
const QString LOG_EVENT_QUEUE_START = "Processing App Task queue";
const QString LOG_EVENT_TASK_START = "Handling task %1 (%2)";
const QString LOG_EVENT_TASK_FINISH = "End of task %1";
const QString LOG_EVENT_TASK_FINISH_ERR = "Premature end of task %1";
const QString LOG_EVENT_QUEUE_FINISH = "Finished processing App Task queue";
const QString LOG_EVENT_CLEANUP_FINISH = "Finished cleanup";
const QString LOG_EVENT_TASK_SKIP = "App Task skipped due to previous errors";
const QString LOG_EVENT_CD = "Changed current directory to: %1";
const QString LOG_EVENT_START_PROCESS = "Started %1 process: %2";
const QString LOG_EVENT_END_PROCESS = "%1 process %2 finished";
const QString LOG_EVENT_ARGS_ESCAPED = "CMD arguments escaped from [[%1]] to [[%2]]";
const QString LOG_EVENT_WAIT_GRACE = "Waiting " + QString::number(SECURE_PLAYER_GRACE) + " seconds for process %1 to be running";
const QString LOG_EVENT_WAIT_RUNNING = "Wait-on process %1 is running";
const QString LOG_EVENT_WAIT_ON = "Waiting for process %1 to finish";
const QString LOG_EVENT_WAIT_QUIT = "Wait-on process %1 has finished";
const QString LOG_EVENT_WAIT_FINISHED = "Wait-on process %1 was not running after the grace period";
const QString LOG_EVENT_DOWNLOADING_DATA_PACK = "Downloading Data Pack %1";
const QString LOG_EVENT_DOWNLOAD_AUTH = "Authentication required to download Data Pack, requesting credentials...";
const QString LOG_EVENT_DOWNLOAD_SUCC = "Data Pack downloaded successfully";

// Error Messages - Prep
const QString ERR_INVALID_COMMAND = R"("%1" is not a valid command)";

// Meta
const QString NAME = "main";

// Prototypes - Process
Core::ErrorCode processTaskQueue(Core& core, QList<QProcess*>& childProcesses);
void handleExecutionError(Core& core, int taskNum, Core::ErrorCode& currentError, Core::ErrorCode newError);
bool cleanStartProcess(Core& core, QProcess* process, QFileInfo exeInfo);
Core::ErrorCode waitOnProcess(Core& core, QString processName, int graceSecs);
void cleanup(Core& core, QList<QProcess*>& childProcesses);

// Prototypes - Helper
QString findFlashpointRoot(Core& core);
QString escapeNativeArgsForCMD(Core& core, QString nativeArgs);
void logProcessStart(Core& core, const QProcess* process, Core::ProcessType type);
void logProcessEnd(Core& core, const QProcess* process, Core::ProcessType type);

int main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // QApplication Object
    QApplication app(argc, argv);

    // Set application name
    QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
    QCoreApplication::setApplicationVersion(VER_FILEVERSION_STR);

    // Error status tracker
    Core::ErrorCode errorStatus = Core::NO_ERR;

    // Create Core instance
    Core coreCLI;

    //-Setup Core--------------------------------------------------------------------------
    QStringList clArgs = app.arguments();
    if((errorStatus = coreCLI.initialize(clArgs)) || clArgs.empty()) // Terminate if error or no command
        return coreCLI.logFinish(NAME, errorStatus);

    //-Restrict app to only one instance---------------------------------------------------
    if(!Qx::enforceSingleInstance())
    {
        coreCLI.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_ALREADY_OPEN));
        return coreCLI.logFinish(NAME, Core::ALREADY_OPEN);
    }

    // Ensure Flashpoint Launcher isn't running
    if(Qx::processIsRunning(QFileInfo(FP::Install::LAUNCHER_PATH).fileName()))
    {
        coreCLI.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_LAUNCHER_RUNNING_P, ERR_LAUNCHER_RUNNING_S));
        return coreCLI.logFinish(NAME, Core::LAUNCHER_OPEN);
    }

    //-Find and link to Flashpoint Install----------------------------------------------------------
    QString fpRoot;
    coreCLI.logEvent(NAME, LOG_EVENT_FLASHPOINT_SEARCH);

    if((fpRoot = findFlashpointRoot(coreCLI)).isNull())
    {
        coreCLI.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_INSTALL_INVALID_P, ERR_INSTALL_INVALID_S));
        return coreCLI.logFinish(NAME, Core::INSTALL_INVALID);
    }

    std::unique_ptr<FP::Install> flashpointInstall = std::make_unique<FP::Install>(fpRoot, CLIFP_DIR_PATH.remove(fpRoot).remove(1, 1));
    coreCLI.logEvent(NAME, LOG_EVENT_FLASHPOINT_LINK.arg(fpRoot));

    // Insert into core
    if((errorStatus = coreCLI.attachFlashpoint(std::move(flashpointInstall))))
        return coreCLI.logFinish(NAME, errorStatus);

    // Open database connection

    //-Handle Command and Command Options----------------------------------------------------------
    QString commandStr = clArgs.first().toLower();

    // Check for valid command
    if(!Command::isRegistered(commandStr))
    {
        coreCLI.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_INVALID_COMMAND.arg(commandStr)));
        return coreCLI.logFinish(NAME, Core::INVALID_ARGS);
    }

    // Create command instance
    std::unique_ptr<Command> commandProcessor = Command::acquire(commandStr, coreCLI);

    // Process command
    if((errorStatus = commandProcessor->process(clArgs)))
        return coreCLI.logFinish(NAME, errorStatus);

    //-Handle Tasks-----------------------------------------------------------------------
    if(coreCLI.hasTasks())
    {
        // Process tracking
        QList<QProcess*> activeChildProcesses;

        // Process app task queue
        Core::ErrorCode executionError = processTaskQueue(coreCLI, activeChildProcesses);
        coreCLI.logEvent(NAME, LOG_EVENT_QUEUE_FINISH);

        // Cleanup
        cleanup(coreCLI, activeChildProcesses);
        coreCLI.logEvent(NAME, LOG_EVENT_CLEANUP_FINISH);

        // Return error status
        return coreCLI.logFinish(NAME, executionError);
    }
    else
        return coreCLI.logFinish(NAME, Core::NO_ERR);
}

Core::ErrorCode processTaskQueue(Core& core, QList<QProcess*>& childProcesses)
{
    core.logEvent(NAME, LOG_EVENT_QUEUE_START);
    // Error tracker
    Core::ErrorCode executionError = Core::NO_ERR;

    // Exhaust queue
    int taskNum = -1;
    while(core.hasTasks())
    {
        // Handle task at front of queue
        ++taskNum;
        std::shared_ptr<Core::Task> currentTask = core.takeFrontTask();
        core.logEvent(NAME, LOG_EVENT_TASK_START.arg(taskNum).arg(ENUM_NAME(currentTask->stage)));

        // Only execute task after an error if it is a Shutdown task
        if(!executionError || currentTask->stage == Core::TaskStage::Shutdown)
        {
            // Cover each task type
            if(std::dynamic_pointer_cast<Core::MessageTask>(currentTask)) // Message (currently ignores process type)
            {
                std::shared_ptr<Core::MessageTask> messageTask = std::static_pointer_cast<Core::MessageTask>(currentTask);

                QMessageBox::information(nullptr, QCoreApplication::applicationName(), messageTask->message);
                core.logEvent(NAME, LOG_EVENT_SHOW_MESSAGE);
            }
            else if(std::dynamic_pointer_cast<Core::ExtraTask>(currentTask)) // Extra
            {
                std::shared_ptr<Core::ExtraTask> extraTask = std::static_pointer_cast<Core::ExtraTask>(currentTask);

                // Ensure extra exists
                if(!extraTask->dir.exists())
                {
                    core.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(extraTask->dir.path())));
                    handleExecutionError(core, taskNum, executionError, Core::EXTRA_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Open extra
                QDesktopServices::openUrl(QUrl::fromLocalFile(extraTask->dir.absolutePath()));
                core.logEvent(NAME, LOG_EVENT_SHOW_EXTRA.arg(extraTask->dir.path()));
            }
            else if(std::dynamic_pointer_cast<Core::WaitTask>(currentTask)) // Wait task
            {
                std::shared_ptr<Core::WaitTask> waitTask = std::static_pointer_cast<Core::WaitTask>(currentTask);

                Core::ErrorCode waitError = waitOnProcess(core, waitTask->processName, SECURE_PLAYER_GRACE);
                if(waitError)
                {
                    handleExecutionError(core, taskNum, executionError, waitError);
                    continue; // Continue to next task
                }
            }
            else if(std::dynamic_pointer_cast<Core::DownloadTask>(currentTask))
            {
                std::shared_ptr<Core::DownloadTask> downloadTask = std::static_pointer_cast<Core::DownloadTask>(currentTask);

                // Setup download
                QFile packFile(downloadTask->destPath + "/" + downloadTask->destFileName);
                Qx::SyncDownloadManager dm;
                dm.setOverwrite(true);
                dm.appendTask(Qx::DownloadTask{downloadTask->targetFile, &packFile});

                // Download event handlers
                QObject::connect(&dm, &Qx::SyncDownloadManager::sslErrors, [&core](Qx::GenericError errorMsg, bool* abort) {
                    int choice = core.postError(NAME, errorMsg, true, QMessageBox::Yes | QMessageBox::Abort, QMessageBox::Abort);
                    *abort = choice == QMessageBox::Abort;
                });

                QObject::connect(&dm, &Qx::SyncDownloadManager::authenticationRequired, [&core](QString prompt, QString* username, QString* password, bool* abort) {
                    core.logEvent(NAME, LOG_EVENT_DOWNLOAD_AUTH);
                    Qx::LoginDialog ld;
                    ld.setPrompt(prompt);

                    int choice = ld.exec();

                    if(choice == QDialog::Accepted)
                    {
                        *username = ld.getUsername();
                        *password = ld.getPassword();
                    }
                    else
                        *abort = true;
                });

                // Log/label string
                QString label = LOG_EVENT_DOWNLOADING_DATA_PACK.arg(QFileInfo(packFile).fileName());
                core.logEvent(NAME, label);

                // Prepare progress bar
                QProgressDialog pd(label, QString("Cancel"), 0, 0);
                pd.setWindowModality(Qt::WindowModal);
                pd.setMinimumDuration(0);
                QObject::connect(&dm, &Qx::SyncDownloadManager::downloadTotalChanged, &pd, &QProgressDialog::setMaximum);
                QObject::connect(&dm, &Qx::SyncDownloadManager::downloadProgress, &pd, &QProgressDialog::setValue);
                QObject::connect(&pd, &QProgressDialog::canceled, &dm, &Qx::SyncDownloadManager::abort);

                // Start download
                Qx::GenericError downloadError= dm.processQueue();

                // Handle result
                pd.close();
                if(downloadError.isValid())
                {
                    downloadError.setErrorLevel(Qx::GenericError::Critical);
                    core.postError(NAME, downloadError);
                    handleExecutionError(core, taskNum, executionError, Core::CANT_OBTAIN_DATA_PACK);
                }

                // Confirm checksum
                bool checksumMatch;
                Qx::IOOpReport checksumResult = Qx::fileMatchesChecksum(checksumMatch, packFile, downloadTask->sha256, QCryptographicHash::Sha256);
                if(!checksumResult.wasSuccessful() || !checksumMatch)
                {
                    core.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_PACK_SUM_MISMATCH));
                    handleExecutionError(core, taskNum, executionError, Core::DATA_PACK_INVALID);
                }

                core.logEvent(NAME, LOG_EVENT_DOWNLOAD_SUCC);
            }
            else if(std::dynamic_pointer_cast<Core::ExecTask>(currentTask)) // Execution task
            {
                std::shared_ptr<Core::ExecTask> execTask = std::static_pointer_cast<Core::ExecTask>(currentTask);

                // Ensure executable exists
                QFileInfo executableInfo(execTask->path + "/" + execTask->filename);
                if(!executableInfo.exists() || !executableInfo.isFile())
                {
                    core.postError(NAME, Qx::GenericError(execTask->stage == Core::TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                                    ERR_EXE_NOT_FOUND.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(core, taskNum, executionError, Core::EXECUTABLE_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Ensure executable is valid
                if(!executableInfo.isExecutable())
                {
                    core.postError(NAME, Qx::GenericError(execTask->stage == Core::TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                                    ERR_EXE_NOT_VALID.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(core, taskNum, executionError, Core::EXECUTABLE_NOT_VALID);
                    continue; // Continue to next task
                }

                // Move to executable directory
                QDir::setCurrent(execTask->path);
                core.logEvent(NAME, LOG_EVENT_CD.arg(execTask->path));

                // Check if task is a batch file
                bool batchTask = QFileInfo(execTask->filename).suffix() == BAT_SUFX;

                // Create process handle
                QProcess* taskProcess = new QProcess();
                taskProcess->setProgram(batchTask ? CMD_EXE : execTask->filename);
                taskProcess->setArguments(execTask->param);
                taskProcess->setNativeArguments(batchTask ? CMD_ARG_TEMPLATE.arg(execTask->filename, escapeNativeArgsForCMD(core, execTask->nativeParam))
                                                          : execTask->nativeParam);
                taskProcess->setStandardOutputFile(QProcess::nullDevice()); // Don't inhert console window
                taskProcess->setStandardErrorFile(QProcess::nullDevice()); // Don't inhert console window

                // Cover each process type
                switch(execTask->processType)
                {
                    case Core::ProcessType::Blocking:
                        taskProcess->setParent(QApplication::instance());
                        if(!cleanStartProcess(core,taskProcess, executableInfo))
                        {
                            handleExecutionError(core, taskNum, executionError, Core::PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(core, taskProcess, Core::ProcessType::Blocking);

                        taskProcess->waitForFinished(-1); // Wait for process to end, don't time out
                        logProcessEnd(core, taskProcess, Core::ProcessType::Blocking);
                        delete taskProcess; // Clear finished process handle from heap
                        break;

                    case Core::ProcessType::Deferred:
                        taskProcess->setParent(QApplication::instance());
                        if(!cleanStartProcess(core, taskProcess, executableInfo))
                        {
                            handleExecutionError(core, taskNum, executionError, Core::PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(core, taskProcess, Core::ProcessType::Deferred);

                        childProcesses.append(taskProcess); // Add process to list for deferred termination
                        break;

                    case Core::ProcessType::Detached:
                        if(!taskProcess->startDetached())
                        {
                            handleExecutionError(core, taskNum, executionError, Core::PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(core, taskProcess, Core::ProcessType::Detached);
                        break;
                }
            }
            else
                throw new std::runtime_error("Unhandled Task child was present in task queue");
        }
        else
            core.logEvent(NAME, LOG_EVENT_TASK_SKIP);

        // Remove handled task
        core.logEvent(NAME, LOG_EVENT_TASK_FINISH.arg(taskNum));
    }

    // Return error status
    return executionError;
}

void handleExecutionError(Core& core, int taskNum, Core::ErrorCode& currentError, Core::ErrorCode newError)
{
    if(!currentError) // Only record first error
        currentError = newError;

    core.logEvent(NAME, LOG_EVENT_TASK_FINISH_ERR.arg(taskNum)); // Record early end of task
}

bool cleanStartProcess(Core& core, QProcess* process, QFileInfo exeInfo)
{
    // Start process and confirm
    process->start();
    if(!process->waitForStarted())
    {
        core.postError(NAME, Qx::GenericError(Qx::GenericError::Critical,
                                   ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(exeInfo.absoluteFilePath()))));
        delete process; // Clear finished process handle from heap
        return false;
    }

    // Return success
    return true;
}

Core::ErrorCode waitOnProcess(Core& core, QString processName, int graceSecs)
{
    // Wait until secure player has stopped running for grace period
    DWORD spProcessID;
    do
    {
        // Yield for grace period
        core.logEvent(NAME, LOG_EVENT_WAIT_GRACE.arg(processName));
        if(graceSecs > 0)
            QThread::sleep(graceSecs);

        // Find process ID by name
        spProcessID = Qx::getProcessIDByName(processName);

        // Check that process was found (is running)
        if(spProcessID)
        {
            core.logEvent(NAME, LOG_EVENT_WAIT_RUNNING.arg(processName));

            // Get process handle and see if it is valid
            HANDLE batchProcessHandle;
            if((batchProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, spProcessID)) == NULL)
            {
                core.postError(NAME, Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HANDLED_P.arg(processName),
                                           WRN_WAIT_PROCESS_NOT_HANDLED_S));
                return Core::WAIT_PROCESS_NOT_HANDLED;
            }

            // Attempt to wait on process to terminate
            core.logEvent(NAME, LOG_EVENT_WAIT_ON.arg(processName));
            DWORD waitError = WaitForSingleObject(batchProcessHandle, INFINITE);

            // Close handle to process
            CloseHandle(batchProcessHandle);

            if(waitError != WAIT_OBJECT_0)
            {
                core.postError(NAME, Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HOOKED_P.arg(processName),
                                           WRN_WAIT_PROCESS_NOT_HOOKED_S));
                return Core::WAIT_PROCESS_NOT_HOOKED;
            }
            core.logEvent(NAME, LOG_EVENT_WAIT_QUIT.arg(processName));
        }
    }
    while(spProcessID);

    // Return success
    core.logEvent(NAME, LOG_EVENT_WAIT_FINISHED.arg(processName));
    return Core::NO_ERR;
}


void cleanup(Core& core, QList<QProcess*>& childProcesses)
{
    // Close each remaining child process
    for(QProcess* childProcess : childProcesses)
    {
        childProcess->close();
        logProcessEnd(core, childProcess, Core::ProcessType::Deferred);
        delete childProcess;
    }
}

//-Helper Functions-----------------------------------------------------------------
QString findFlashpointRoot(Core& core)
{
    QDir currentDir(CLIFP_DIR_PATH);

    do
    {
        core.logEvent(NAME, LOG_EVENT_FLASHPOINT_ROOT_CHECK.arg(currentDir.absolutePath()));
        if(FP::Install::checkInstallValidity(currentDir.absolutePath(), FP::Install::CompatLevel::Execution).installValid)
            return currentDir.absolutePath();
    }
    while(currentDir.cdUp());

    // Return null string on failure to find root
    return QString();
}

QString escapeNativeArgsForCMD(Core& core, QString nativeArgs)
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
        core.logEvent(NAME, LOG_EVENT_ARGS_ESCAPED.arg(nativeArgs, escapedNativeArgs));

    return escapedNativeArgs;
}


void logProcessStart(Core& core, const QProcess* process, Core::ProcessType type)
{
    QString eventStr = process->program();
    if(!process->arguments().isEmpty())
        eventStr += " " + process->arguments().join(" ");
    if(!process->nativeArguments().isEmpty())
        eventStr += " " + process->nativeArguments();

    core.logEvent(NAME, LOG_EVENT_START_PROCESS.arg(ENUM_NAME(type), eventStr));
}

void logProcessEnd(Core& core, const QProcess* process, Core::ProcessType type)
{
    core.logEvent(NAME, LOG_EVENT_END_PROCESS.arg(ENUM_NAME(type), process->program()));
}



