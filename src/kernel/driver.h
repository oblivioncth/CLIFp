#ifndef DRIVER_H
#define DRIVER_H

// Qt Includes
#include <QThread>

// Qx Includes
#include <qx/core/qx-setonce.h>
#include <qx/network/qx-downloadmanager.h>

// Project Includes
#include "kernel/core.h"

class Driver : public QObject
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Single Instance ID
    static inline const QString SINGLE_INSTANCE_ID = "CLIFp_ONE_INSTANCE"; // Basically never change this

    // Error Messages
    static inline const QString ERR_ALREADY_OPEN = "Only one instance of CLIFp can be used at a time!";
    static inline const QString ERR_INVALID_COMMAND = R"("%1" is not a valid command)";
    static inline const QString ERR_LAUNCHER_RUNNING_P = "The CLI cannot be used while the Flashpoint Launcher is running.";
    static inline const QString ERR_LAUNCHER_RUNNING_S = "Please close the Launcher first.";
    static inline const QString ERR_INSTALL_INVALID_P = "CLIFp does not appear to be deployed in a valid Flashpoint install";
    static inline const QString ERR_INSTALL_INVALID_S = "Check its location and compatibility with your Flashpoint version.";

    // Logging
    static inline const QString LOG_EVENT_FLASHPOINT_SEARCH = "Searching for Flashpoint root...";
    static inline const QString LOG_EVENT_FLASHPOINT_ROOT_CHECK = R"(Checking if "%1" is flashpoint root)";
    static inline const QString LOG_EVENT_FLASHPOINT_LINK = R"(Linked to Flashpoint install at: "%1")";
    static inline const QString LOG_EVENT_TASK_COUNT = "%1 task(s) to perform";
    static inline const QString LOG_EVENT_QUEUE_START = "Processing Task queue";
    static inline const QString LOG_EVENT_TASK_START = "Handling task %1 [%2] (%3)";
    static inline const QString LOG_EVENT_TASK_FINISH = "End of task %1";
    static inline const QString LOG_EVENT_TASK_FINISH_ERR = "Premature end of task %1";
    static inline const QString LOG_EVENT_QUEUE_FINISH = "Finished processing Task queue";
    static inline const QString LOG_EVENT_ENDING_CHILD_PROCESSES = "Closing deferred processes...";
    static inline const QString LOG_EVENT_CLEANUP_START = "Cleaning up...";
    static inline const QString LOG_EVENT_CLEANUP_FINISH = "Finished cleanup";
    static inline const QString LOG_EVENT_TASK_SKIP_ERROR = "Task skipped due to previous errors";
    static inline const QString LOG_EVENT_TASK_SKIP_QUIT = "Task skipped because the application is quitting";
    static inline const QString LOG_EVENT_QUIT_REQUEST = "Received quit request";
    static inline const QString LOG_EVENT_QUIT_REQUEST_REDUNDANT = "Received redundant quit request";

    // Meta
    static inline const QString NAME = "driver";

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    QStringList mArguments;

    Core* mCore; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    /*
     * NOTE: The pointer members here could be on stack if they are assigned as children to this Driver instance in its initialization list, but at the moment using
     * pointers instead for simplicity. If set as a child of this instance, they will be moved with the instance automatically when the instance is moved to a separate
     * thread. Otherwise (and without using pointers and init during drive()), the connections they have will be invoked on the main thread since they are spawned when
     * Driver is constructed, which is done on the main thread before it is moved.
     *
     * However, if this were to be done then any member variables of Core that derive from QObject, and are constructed during its construction (e.g. stack variables),
     * would have to have their parents set as Core as well, with this being needed recursively all the way down the tree (i.e. those objects would have to have all
     * their QObject based members made children). Because ensuring this can be tricky and error prone if any members in this tree change, it's just easier to
     * construct core after Driver is already moved to the new thread.
    */

    Qx::SetOnce<ErrorCode> mErrorStatus;

    Task* mCurrentTask;
    int mCurrentTaskNumber;

    bool mQuitRequested;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Driver(QStringList arguments);

//-Instance Functions------------------------------------------------------------------------------------------------------------
private:
    // Setup
    void init();

    // Process
    void startNextTask();
    void cleanup();

    void finish();

    // Helper
    std::unique_ptr<Fp::Install> findFlashpointInstall();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void completeTaskHandler(ErrorCode ec = ErrorCode::NO_ERR);

public slots:
    // Worker main
    void drive();

    // Termination
    void cancelActiveLongTask();
    void quitNow();

signals:
    // Worker status
    void finished(ErrorCode errorCode);

    // Core forwarders
    void statusChanged(const QString& statusHeading, const QString& statusMessage);
    void errorOccured(const Core::Error& error);
    void blockingErrorOccured(QSharedPointer<int> response, const Core::BlockingError& blockingError);
    void message(const QString& message);
    void saveFileRequested(QSharedPointer<QString> file, const Core::SaveFileRequest& request);
    void itemSelectionRequested(QSharedPointer<QString> item, const Core::ItemSelectionRequest& request);

    // Long task
    void longTaskProgressChanged(quint64 progress);
    void longTaskTotalChanged(quint64 total);
    void longTaskStarted(QString task);
    void longTaskFinished();
};

#endif // DRIVER_H
