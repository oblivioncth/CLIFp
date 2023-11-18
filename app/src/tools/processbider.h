#ifndef PROCESSWAITER_H
#define PROCESSWAITER_H

// Qt Includes
#include <QThread>
#include <QReadWriteLock>

// Qx Includes
#include <qx/core/qx-abstracterror.h>
#include <qx/utility/qx-helpers.h>

/* This uses the approach of sub-classing QThread instead of the worker/object model. This means that by default there is no event
 * loop running in the new thread (not needed with current setup), and that only the contents of run() take place in the new thread,
 * with everything else happening in the thread that contains the instance of this class.
 *
 * This does mean that a Mutex must be used to protected against access to the same data between threads where necessary, but in
 * this case that is desirable as when the parent thread calls `closeProcess()` we want it to get blocked if the run() thread is
 * busy doing work until it goes back to waiting (or the wait process stops on its own), since we want to make sure that the wait
 * process has ended before Driver takes its next steps.
 *
 * If for whatever reason the worker/object approach is desired in the future, an alternative to achieve the same thing would be to
 * use a QueuedBlocking connection between the signal emitted by the worker instance method `endProcess()` (which acts as the
 * interface to the object) and the slot in the object instance; however, in order for the object to be able to process the quit
 * signal it cannot be sitting there blocked by WaitOnSingleObject (since it's now that threads responsibility to perform the quit
 * instead of the thread that emitted the signal), so instead RegisterWaitForSingleObject would have to be used, which may have
 * caveats since a thread spawned by the OS is used to trigger the specified callback function. It would potentially be safe if that
 * callback function simply emits an internal signal with a queued connection that then triggers the thread managing the object to
 * handle the quit upon its next event loop cycle.
 *
 * NOTE: Technically the thread synchronization here is imperfect as a blocked closeProcess() is unlocked one step before the wait
 * actually starts so there could be a race between that and the waiter being marked as "in wait". Not a great way to avoid this though
 * since the lock can't be unlocked any later since the waiting thread deadlocks once the wait is started.
 */

class QX_ERROR_TYPE(ProcessBiderError, "ProcessBiderError", 1235)
{
    friend class ProcessBider;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        Wait = 1,
        Close = 2
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {Wait, u"Could not setup a wait on the process."_s},
        {Close, u"Could not close the wait on process."_s},
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    ProcessBiderError(Type t = NoError, const QString& s = {});

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

class ProcessWaiter;

class ProcessBider : public QThread
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status Messages
    static inline const QString LOG_EVENT_BIDE_GRACE = u"Waiting %1 seconds for process %2 to be running"_s;
    static inline const QString LOG_EVENT_BIDE_RUNNING = u"Wait-on process %1 is running"_s;
    static inline const QString LOG_EVENT_BIDE_ON = u"Waiting for process %1 to finish"_s;
    static inline const QString LOG_EVENT_BIDE_QUIT = u"Wait-on process %1 has finished"_s;
    static inline const QString LOG_EVENT_BIDE_FINISHED = u"Wait-on process %1 was not running after the grace period"_s;

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    // Work
    ProcessWaiter* mWaiter;
    QReadWriteLock mRWLock;

    // Process Info
    QString mProcessName;
    uint mRespawnGrace;
    uint mPollRate;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    ProcessBider(QObject* parent = nullptr, const QString& name = {});

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    // Run in wait thread
    ProcessBiderError doWait();
    void run() override;

public:
    // Run in external thread
    void setProcessName(const QString& name);
    void setRespawnGrace(uint respawnGrace);
    void setPollRate(uint pollRate); // Ignored on Windows
    ProcessBiderError closeProcess();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
public slots:
    void start();

signals:
    void statusChanged(QString statusMessage);
    void errorOccurred(ProcessBiderError errorMessage);
    void bideFinished(ProcessBiderError errorStatus);
};

#endif // PROCESSWAITER_H
