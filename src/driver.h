#ifndef DRIVER_H
#define DRIVER_H

#include "core.h"
#include "qx-net.h"

#include <QThread>

class Driver : public QObject
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Error Messages - Prep
    static inline const QString ERR_ALREADY_OPEN = "Only one instance of CLIFp can be used at a time!";
    static inline const QString ERR_INVALID_COMMAND = R"("%1" is not a valid command)";
    static inline const QString ERR_LAUNCHER_RUNNING_P = "The CLI cannot be used while the Flashpoint Launcher is running.";
    static inline const QString ERR_LAUNCHER_RUNNING_S = "Please close the Launcher first.";
    static inline const QString ERR_INSTALL_INVALID_P = "CLIFp does not appear to be deployed in a valid Flashpoint install";
    static inline const QString ERR_INSTALL_INVALID_S = "Check its location and compatability with your Flashpoint version.";

    // Error Messages - Execution
    static inline const QString ERR_EXTRA_NOT_FOUND = "The extra %1 does not exist!";
    static inline const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
    static inline const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
    static inline const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";
    static inline const QString ERR_PACK_SUM_MISMATCH = "The title's Data Pack checksum does not match its record!";
    static inline const QString WRN_WAIT_PROCESS_NOT_HANDLED_P  = "Could not get a wait handle to %1.";
    static inline const QString WRN_WAIT_PROCESS_NOT_HANDLED_S = "The title may not work correctly";
    static inline const QString WRN_WAIT_PROCESS_NOT_HOOKED_P  = "Could not hook %1 for waiting.";
    static inline const QString WRN_WAIT_PROCESS_NOT_HOOKED_S = "The title may not work correctly";

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
    static inline const QString LOG_EVENT_FLASHPOINT_ROOT_CHECK = "Checking if \"%1\" is flashpoint root";
    static inline const QString LOG_EVENT_FLASHPOINT_LINK = "Linked to Flashpoint install at: %1";
    static inline const QString LOG_EVENT_SHOW_MESSAGE = "Displayed message";
    static inline const QString LOG_EVENT_SHOW_EXTRA = "Opened folder of extra %1";
    static inline const QString LOG_EVENT_TASK_COUNT = "%1 task(s) to perform";
    static inline const QString LOG_EVENT_QUEUE_START = "Processing App Task queue";
    static inline const QString LOG_EVENT_TASK_START = "Handling task %1 (%2)";
    static inline const QString LOG_EVENT_TASK_FINISH = "End of task %1";
    static inline const QString LOG_EVENT_TASK_FINISH_ERR = "Premature end of task %1";
    static inline const QString LOG_EVENT_QUEUE_FINISH = "Finished processing App Task queue";
    static inline const QString LOG_EVENT_CLEANUP_FINISH = "Finished cleanup";
    static inline const QString LOG_EVENT_TASK_SKIP = "App Task skipped due to previous errors";
    static inline const QString LOG_EVENT_CD = "Changed current directory to: %1";
    static inline const QString LOG_EVENT_START_PROCESS = "Started %1 process: %2";
    static inline const QString LOG_EVENT_END_PROCESS = "%1 process %2 finished";
    static inline const QString LOG_EVENT_ARGS_ESCAPED = "CMD arguments escaped from [[%1]] to [[%2]]";
    static inline const QString LOG_EVENT_WAIT_GRACE = "Waiting " + QString::number(SECURE_PLAYER_GRACE) + " seconds for process %1 to be running";
    static inline const QString LOG_EVENT_WAIT_RUNNING = "Wait-on process %1 is running";
    static inline const QString LOG_EVENT_WAIT_ON = "Waiting for process %1 to finish";
    static inline const QString LOG_EVENT_WAIT_QUIT = "Wait-on process %1 has finished";
    static inline const QString LOG_EVENT_WAIT_FINISHED = "Wait-on process %1 was not running after the grace period";
    static inline const QString LOG_EVENT_DOWNLOADING_DATA_PACK = "Downloading Data Pack %1";
    static inline const QString LOG_EVENT_DOWNLOAD_AUTH = "Authentication required to download Data Pack, requesting credentials...";
    static inline const QString LOG_EVENT_DOWNLOAD_SUCC = "Data Pack downloaded successfully";

    // Meta
    static inline const QString NAME = "driver";

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    QStringList mArguments;
    QString mRawArguments;
    Core* mCore; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    QList<QProcess*> mActiveChildProcesses;
    Qx::SyncDownloadManager* mDownloadManager; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    /*
     TODO: The pointer members here could be on stack if they are assigned as children to this Driver instance in its initialization list, but at the moment using
     pointers instead for simplicity. If set as a child of this instance, they will be moved with the instance automatically when the instance is moved to a sperate
     thread. Otherwise (and without using pointers and init during drive()), the connections they have will be invoked on the main thread since they are spawned when
     Driver is constructed, which is done on the main thread before it is moved.
    */

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Driver(QStringList arguments, QString rawArguments);

//-Class Functions---------------------------------------------------------------------------------------------------------------
private:
    static QString getRawCommandLineParams(const QString& rawCommandLine);

//-Instance Functions------------------------------------------------------------------------------------------------------------
private:
    // Setup
    void init();

    // Process
    ErrorCode processTaskQueue();
    void handleExecutionError(int taskNum, ErrorCode& currentError, ErrorCode newError);
    bool cleanStartProcess(QProcess* process, QFileInfo exeInfo);
    ErrorCode waitOnProcess(QString processName, int graceSecs);
    void cleanup();

    // Helper
    std::unique_ptr<FP::Install> findFlashpointInstall();
    QString escapeNativeArgsForCMD(QString nativeArgs);
    void logProcessStart(const QProcess* process, Core::ProcessType type);
    void logProcessEnd(const QProcess* process, Core::ProcessType type);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
public slots:
    // Worker main
    void drive();

    // Net
    void cancelActiveDownloads();

signals:
    // Worker status
    void finished(ErrorCode errorCode);

    // Core forwarders
    void statusChanged(const QString& statusHeading, const QString& statusMessage);
    void errorOccured(const Core::Error& error);
    void blockingErrorOccured(QSharedPointer<int> response, const Core::BlockingError& blockingError);
    void message(const QString& message);

    // Network
    void authenticationRequired(QString prompt, QString* username, QString* password, bool* abort);
    void downloadProgressChanged(quint64 progress);
    void downloadTotalChanged(quint64 total);
    void downloadStarted(QString task);
    void downloadFinished(bool canceled);
};

#endif // DRIVER_H
