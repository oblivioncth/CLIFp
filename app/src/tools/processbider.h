#ifndef PROCESSWAITER_H
#define PROCESSWAITER_H

// Qt Includes
#include <QThread>
#include <QMutex>

// Qx Includes
#include <qx/windows/qx-common-windows.h>
#include <qx/utility/qx-macros.h>

/* This uses the approach of sub-classing QThread instead of the worker/object model. This means that by default there is no event
 * loop running in the new thread (not needed with current setup), and that only the contents of run() take place in the new thread,
 * with everything else happening in the thread that contains the instance of this class.
 *
 * This does mean that a Mutex must be used to protected against access to the same data between threads where necessary, but in
 * this case that is desirable as when the parent thread calls `endProcess()` we want it to get blocked if the run() thread is
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
 */

class QX_ERROR_TYPE(ProcessBiderError, "ProcessBiderError", 1233)
{
    friend class ProcessBider;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        HandleAquisition = 1,
        ProcessHook = 2
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {HandleAquisition, QSL("Could not get a wait handle to a restartable process, the title will likely not work correctly.")},
        {ProcessHook, QSL("Could not hook a restartable process for waiting, the title will likely not work correctly.")},
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

class ProcessBider : public QThread
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status Messages
    static inline const QString LOG_EVENT_BIDE_GRACE = QSL("Waiting %1 seconds for process %2 to be running");
    static inline const QString LOG_EVENT_BIDE_RUNNING = QSL("Wait-on process %1 is running");
    static inline const QString LOG_EVENT_BIDE_ON = QSL("Waiting for process %1 to finish");
    static inline const QString LOG_EVENT_BIDE_QUIT = QSL("Wait-on process %1 has finished");
    static inline const QString LOG_EVENT_BIDE_FINISHED = QSL("Wait-on process %1 was not running after the grace period");

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    // Process Info
    QString mProcessName;
    uint mRespawnGrace;

    // Process Handling
    HANDLE mProcessHandle;
    QMutex mProcessHandleMutex;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    ProcessBider(QObject* parent = nullptr, uint respawnGrace = 30000);

//-Class Functions---------------------------------------------------------------------------------------------------------
private:
    static bool closeAdminProcess(DWORD processId, bool force);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    ProcessBiderError doWait();
    void run() override;

public:
    void setRespawnGrace(uint respawnGrace);

    bool closeProcess();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
public slots:
    void start(QString processName);

signals:
    void statusChanged(QString statusMessage);
    void errorOccured(ProcessBiderError errorMessage);
    void bideFinished(ProcessBiderError errorStatus);
};

#endif // PROCESSWAITER_H
