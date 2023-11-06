#ifndef DRIVER_H
#define DRIVER_H

// Qt Includes
#include <QThread>

// Qx Includes
#include <qx/network/qx-downloadmanager.h>
#include <qx/utility/qx-macros.h>

// Project Includes
#include "kernel/errorstatus.h"
#include "kernel/core.h"

class QX_ERROR_TYPE(DriverError, "DriverError", 1201)
{
    friend class Driver;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        AlreadyOpen,
        LauncherRunning,
        InvalidInstall,
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {AlreadyOpen, u"Only one instance of CLIFp can be used at a time!"_s},
        {LauncherRunning, u"The CLI cannot be used while the Flashpoint Launcher is running."_s},
        {InvalidInstall, u"CLIFp does not appear to be deployed in a valid Flashpoint install"_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    DriverError(Type t = NoError, const QString& s = {});

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class Driver : public QObject
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Error Messages
    static inline const QString ERR_LAUNCHER_RUNNING_TIP = u"Please close the Launcher first."_s;
    static inline const QString ERR_INSTALL_INVALID_TIP = u"You may need to update (i.e. the 'update' command)."_s;

    // Logging
    static inline const QString LOG_EVENT_FLASHPOINT_SEARCH = u"Searching for Flashpoint root..."_s;
    static inline const QString LOG_EVENT_FLASHPOINT_ROOT_CHECK = uR"(Checking if "%1" is flashpoint root)"_s;
    static inline const QString LOG_WARN_FP_UNRECOGNIZED_DAEMON = "Flashpoint install does not contain a recognized daemon!";
    static inline const QString LOG_EVENT_FLASHPOINT_LINK = uR"(Linked to Flashpoint install at: "%1")"_s;
    static inline const QString LOG_EVENT_TASK_COUNT = u"%1 task(s) to perform"_s;
    static inline const QString LOG_EVENT_QUEUE_START = u"Processing Task queue"_s;
    static inline const QString LOG_EVENT_TASK_START = u"Handling task %1 [%2] (%3)"_s;
    static inline const QString LOG_EVENT_TASK_FINISH = u"End of task %1"_s;
    static inline const QString LOG_EVENT_TASK_FINISH_ERR = u"Premature end of task %1"_s;
    static inline const QString LOG_EVENT_QUEUE_FINISH = u"Finished processing Task queue"_s;
    static inline const QString LOG_EVENT_ENDING_CHILD_PROCESSES = u"Closing deferred processes..."_s;
    static inline const QString LOG_EVENT_CLEANUP_START = u"Cleaning up..."_s;
    static inline const QString LOG_EVENT_CLEANUP_FINISH = u"Finished cleanup"_s;
    static inline const QString LOG_EVENT_TASK_SKIP_ERROR = u"Task skipped due to previous errors"_s;
    static inline const QString LOG_EVENT_TASK_SKIP_QUIT = u"Task skipped because the application is quitting"_s;
    static inline const QString LOG_EVENT_QUIT_REQUEST = u"Received quit request"_s;
    static inline const QString LOG_EVENT_QUIT_REQUEST_REDUNDANT = u"Received redundant quit request"_s;
    static inline const QString LOG_EVENT_CLEARED_UPDATE_CACHE = u"Cleared stale update cache."_s;

    // Meta
    static inline const QString NAME = u"driver"_s;

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

    ErrorStatus mErrorStatus;

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
    void completeTaskHandler(Qx::Error e = {});

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
    void errorOccurred(const Core::Error& error);
    void blockingErrorOccurred(QSharedPointer<int> response, const Core::BlockingError& blockingError);
    void message(const Message& message);
    void saveFileRequested(QSharedPointer<QString> file, const Core::SaveFileRequest& request);
    void itemSelectionRequested(QSharedPointer<QString> item, const Core::ItemSelectionRequest& request);
    void clipboardUpdateRequested(const QString& text);
    void questionAnswerRequested(QSharedPointer<bool> response, const QString& question);

    // Long task
    void longTaskProgressChanged(quint64 progress);
    void longTaskTotalChanged(quint64 total);
    void longTaskStarted(QString task);
    void longTaskFinished();
};

#endif // DRIVER_H
