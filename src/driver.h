#ifndef DRIVER_H
#define DRIVER_H

// Qt Includes
#include <QThread>

// Qx Includes
#include <qx/core/qx-setonce.h>
#include <qx/network/qx-downloadmanager.h>

// Project Includes
#include "core.h"
#include "processwaiter.h"
#include "mounter.h"

class Driver : public QObject
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Single Instance ID
    static inline const QString SINGLE_INSTANCE_ID = "CLIFp_ONE_INSTANCE"; // Basically never change this

    // Error Messages - Prep
    static inline const QString ERR_ALREADY_OPEN = "Only one instance of CLIFp can be used at a time!";
    static inline const QString ERR_INVALID_COMMAND = R"("%1" is not a valid command)";
    static inline const QString ERR_LAUNCHER_RUNNING_P = "The CLI cannot be used while the Flashpoint Launcher is running.";
    static inline const QString ERR_LAUNCHER_RUNNING_S = "Please close the Launcher first.";
    static inline const QString ERR_INSTALL_INVALID_P = "CLIFp does not appear to be deployed in a valid Flashpoint install";
    static inline const QString ERR_INSTALL_INVALID_S = "Check its location and compatibility with your Flashpoint version.";

    // Error Messages - Execution
    static inline const QString ERR_EXTRA_NOT_FOUND = "The extra %1 does not exist!";
    static inline const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
    static inline const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
    static inline const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";
    static inline const QString ERR_PACK_SUM_MISMATCH = "The title's Data Pack checksum does not match its record!";
    static inline const QString ERR_PACK_EXTRACT = "Could not extract data pack %1.";
    static inline const QString ERR_PACK_EXTRACT_OPEN = "Failed to open the archive.";
    static inline const QString ERR_PACK_EXTRACT_MAKE_PATH = "Failed to create file path.";
    static inline const QString ERR_PACK_EXTRACT_OPEN_ARCH_FILE = "Failed to open archive file (code 0x%1).";
    static inline const QString ERR_PACK_EXTRACT_OPEN_DISK_FILE = "Failed to open disk file \"%1\".";
    static inline const QString ERR_PACK_EXTRACT_WRITE_DISK_FILE = "Failed to write disk file \"%1\".";
    static inline const QString ERR_PACK_EXTRACT_GENERAL_ZIP = "General zip error (code 0x%1).";
    static inline const QString ERR_CANT_CLOSE_WAIT_ON = "Could not automatically end the running title! It will have to be closed manually.";

    // Suffixes
    static inline const QString BAT_SUFX = "bat";
    static inline const QString EXE_SUFX = "exe";

    // Processing constants
    static inline const QString CMD_EXE = "cmd.exe";
    static inline const QString CMD_ARG_TEMPLATE = R"(/d /s /c ""%1" %2")";

    // Wait timing
    static const int SECURE_PLAYER_GRACE = 2; // Seconds to allow the secure player to restart in cases it does

    // Logging - Messages
    static inline const QString LOG_EVENT_FLASHPOINT_SEARCH = "Searching for Flashpoint root...";
    static inline const QString LOG_EVENT_FLASHPOINT_ROOT_CHECK = R"(Checking if "%1" is flashpoint root)";
    static inline const QString LOG_EVENT_FLASHPOINT_LINK = R"(Linked to Flashpoint install at: "%1")";
    static inline const QString LOG_EVENT_SHOW_MESSAGE = "Displayed message";
    static inline const QString LOG_EVENT_SHOW_EXTRA = "Opened folder of extra %1";
    static inline const QString LOG_EVENT_TASK_COUNT = "%1 task(s) to perform";
    static inline const QString LOG_EVENT_QUEUE_START = "Processing App Task queue";
    static inline const QString LOG_EVENT_TASK_START = "Handling task %1 [%2] (%3)";
    static inline const QString LOG_EVENT_TASK_FINISH = "End of task %1";
    static inline const QString LOG_EVENT_TASK_FINISH_ERR = "Premature end of task %1";
    static inline const QString LOG_EVENT_QUEUE_FINISH = "Finished processing App Task queue";
    static inline const QString LOG_EVENT_CLEANUP_FINISH = "Finished cleanup";
    static inline const QString LOG_EVENT_TASK_SKIP_ERROR = "App Task skipped due to previous errors";
    static inline const QString LOG_EVENT_TASK_SKIP_QUIT = "App Task skipped because the application is quitting";
    static inline const QString LOG_EVENT_CD = "Changed current directory to: %1";
    static inline const QString LOG_EVENT_START_PROCESS = "Started %1 process: %2";
    static inline const QString LOG_EVENT_END_PROCESS = "%1 process %2 finished";
    static inline const QString LOG_EVENT_ARGS_ESCAPED = "CMD arguments escaped from [[%1]] to [[%2]]";
    static inline const QString LOG_EVENT_DOWNLOADING_DATA_PACK = "Downloading Data Pack %1";
    static inline const QString LOG_EVENT_DOWNLOAD_AUTH = "Authentication required to download Data Pack, requesting credentials...";
    static inline const QString LOG_EVENT_DOWNLOAD_SUCC = "Data Pack downloaded successfully";
    static inline const QString LOG_EVENT_EXTRACTING_DATA_PACK = "Extracting Data Pack %1";
    static inline const QString LOG_EVENT_MOUNTING_DATA_PACK = "Mounting Data Pack %1";
    static inline const QString LOG_EVENT_QUIT_REQUEST = "Received quit request";
    static inline const QString LOG_EVENT_QUIT_REQUEST_REDUNDANT = "Received redundant quit request";
    static inline const QString LOG_EVENT_STOPPING_MAIN_PROCESS = "Stopping primary execution process...";
    static inline const QString LOG_EVENT_STOPPING_WAIT_PROCESS = "Stopping current wait on execution process...";
    static inline const QString LOG_EVENT_STOPPING_DOWNLOADS = "Stopping current download(s)...";
    static inline const QString LOG_EVENT_STOPPING_MOUNT = "Stopping current mount(s)...";


    // Meta
    static inline const QString NAME = "driver";

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    QStringList mArguments;
    QString mRawArguments;
    Qx::SetOnce<ErrorCode> mErrorStatus;
    int mCurrentTaskNumber;
    bool mQuitRequested;

    Core* mCore; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    QProcess* mMainBlockingProcess;
    QList<QProcess*> mActiveChildProcesses;
    ProcessWaiter* mProcessWaiter; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    Mounter* mMounter; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    Qx::AsyncDownloadManager* mDownloadManager; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    /*
     * TODO: The pointer members here could be on stack if they are assigned as children to this Driver instance in its initialization list, but at the moment using
     * pointers instead for simplicity. If set as a child of this instance, they will be moved with the instance automatically when the instance is moved to a separate
     * thread. Otherwise (and without using pointers and init during drive()), the connections they have will be invoked on the main thread since they are spawned when
     * Driver is constructed, which is done on the main thread before it is moved.
    */

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Driver(QStringList arguments, QString rawArguments);

//-Class Functions---------------------------------------------------------------------------------------------------------------
private:
    static QString getRawCommandLineParams(const QString& rawCommandLine);
    static Qx::GenericError extractZipSubFolderContentToDir(QString zipFilePath, QString subFolder, QDir dir);

//-Instance Functions------------------------------------------------------------------------------------------------------------
private:
    // Setup
    void init();

    // Process
    void processExecTask(const std::shared_ptr<Core::ExecTask> task);
    void processMessageTask(const std::shared_ptr<Core::MessageTask> task);
    void processExtraTask(const std::shared_ptr<Core::ExtraTask> task);
    void processWaitTask(const std::shared_ptr<Core::WaitTask> task);
    void processDownloadTask(const std::shared_ptr<Core::DownloadTask> task);
    void processMountTask(const std::shared_ptr<Core::MountTask> task);
    void processExtractTask(const std::shared_ptr<Core::ExtractTask> task);

    void startNextTask();
    void handleTaskError(ErrorCode error);
    bool cleanStartProcess(QProcess* process, QFileInfo exeInfo);
    ErrorCode waitOnProcess(QString processName, int graceSecs);
    void cleanup();

    void finish();

    // Helper
    std::unique_ptr<Fp::Install> findFlashpointInstall();
    QString escapeNativeArgsForCMD(QString nativeArgs);
    void logProcessStart(const QProcess* process, Core::ProcessType type);
    void logProcessEnd(const QProcess* process, Core::ProcessType type);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void finishedBlockingExecutionHandler();
    void finishedDownloadHandler(Qx::DownloadManagerReport downloadReport);
    void finishedWaitHandler(ErrorCode errorStatus);
    void finishedMountHandler(ErrorCode errorStatus);
    void finishedTaskHandler();

public slots:
    // Worker main
    void drive();

    // Net
    void cancelActiveLongTask();

    // General
    void quitNow();

signals:
    // Private Signals
    void __currentTaskFinished();

    // Worker status
    void finished(ErrorCode errorCode);

    // Core forwarders
    void statusChanged(const QString& statusHeading, const QString& statusMessage);
    void errorOccured(const Core::Error& error);
    void blockingErrorOccured(QSharedPointer<int> response, const Core::BlockingError& blockingError);
    void message(const QString& message);

    // Network
    void authenticationRequired(QString prompt, QAuthenticator* authenticator);

    // Long task
    void longTaskProgressChanged(quint64 progress);
    void longTaskTotalChanged(quint64 total);
    void longTaskStarted(QString task);
    void longTaskFinished();
};

#endif // DRIVER_H
