#ifndef DRIVER_P_H
#define DRIVER_P_H

// Qt Includes
#include <QThread>

// Qx Includes
#include <qx/network/qx-downloadmanager.h>
#include <qx/utility/qx-macros.h>

// Project Includes
#include "kernel/errorstatus.h"
#include "kernel/directorate.h"

class Driver;
class Core;
namespace Fp { class Install; }

class QX_ERROR_TYPE(DriverError, "DriverError", 1202)
{
    friend class DriverPrivate;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        AlreadyOpen,
        InvalidInstall,
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {AlreadyOpen, u"Only one instance of CLIFp can be used at a time!"_s},
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

class DriverPrivate : public Directorate
{
    Q_DECLARE_PUBLIC(Driver);
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Error Messages
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
    static inline const QString LOG_EVENT_CORE_ABORT = u"Core abort signaled, quitting now."_s;
    static inline const QString LOG_EVENT_FINISH = u"Finishing run..."_s;

    // Meta
    static inline const QString NAME = u"driver"_s;

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    Driver* const q_ptr;
    QStringList mArguments;
    std::unique_ptr<Core> mCore; // Must not be spawned during construction but after object is moved to thread and operated (since it uses signals/slots)
    /*
     * This note might be outdated, now that we're using PIMPL, as parenting the members to the public class might cause race conditions for signal disconnection
     * purposes.
     *
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
    DriverPrivate(Driver* q, QStringList arguments);

//-Instance Functions------------------------------------------------------------------------------------------------------------
private:
    QString name() const override;
    ErrorCode logFinish(const Qx::Error& errorState) const;

    // Setup
    void init();

    // Process
    void startNextTask();
    void completeTaskHandler(const Qx::Error& e = {});
    void cleanup();

    void finish();
    void quit();

    // Helper
    std::unique_ptr<Fp::Install> findFlashpointInstall();

public:
    void drive();
    void cancelActiveLongTask();
    void quitNow();
};

#endif // DRIVER_P_H
