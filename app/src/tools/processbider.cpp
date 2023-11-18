// Unit Include
#include "processbider.h"
#include "processbider_p.h"

// Qx Includes
#include <qx/core/qx-system.h>

//===============================================================================================================
// ProcessBiderError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
ProcessBiderError::ProcessBiderError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool ProcessBiderError::isValid() const { return mType != NoError; }
QString ProcessBiderError::specific() const { return mSpecific; }
ProcessBiderError::Type ProcessBiderError::type() const { return mType; }

//Private:
Qx::Severity ProcessBiderError::deriveSeverity() const { return Qx::Warning; }
quint32 ProcessBiderError::deriveValue() const { return mType; }
QString ProcessBiderError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString ProcessBiderError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// ProcessBider
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
ProcessBider::ProcessBider(QObject* parent, const QString& name) :
    QThread(parent),
    mProcessName(name),
    mRespawnGrace(30000),
    mPollRate(500)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
ProcessBiderError ProcessBider::doWait()
{
    mWaiter = new ProcessWaiter(mProcessName);
    mWaiter->setPollRate(mPollRate);

    // Block outer access until waiting
    QWriteLocker writeLock(&mRWLock);

    // Wait until process has stopped running for grace period
    quint32 procId;
    do
    {
        // Yield for grace period
        emit statusChanged(LOG_EVENT_BIDE_GRACE.arg(mRespawnGrace).arg(mProcessName));
        if(mRespawnGrace > 0)
            QThread::sleep(mRespawnGrace);

        // Find process ID by name
        procId = Qx::processId(mProcessName);

        // Check that process was found (is running)
        if(procId)
        {
            emit statusChanged(LOG_EVENT_BIDE_RUNNING.arg(mProcessName));

            // Attempt to wait on process to terminate
            emit statusChanged(LOG_EVENT_BIDE_ON.arg(mProcessName));
            mWaiter->updateId(procId);
            writeLock.unlock(); // To allow close attempts
            if(!mWaiter->wait()) // Blocks until process ends
            {
                ProcessBiderError err(ProcessBiderError::Wait, mProcessName);
                emit errorOccurred(err);
                return err;
            }
            writeLock.relock();

            emit statusChanged(LOG_EVENT_BIDE_QUIT.arg(mProcessName));
        }
    }
    while(procId);

    // Return success
    emit statusChanged(LOG_EVENT_BIDE_FINISHED.arg(mProcessName));
    return ProcessBiderError();
}

void ProcessBider::run()
{
    ProcessBiderError status = doWait();
    qxDelete(mWaiter);
    emit bideFinished(status);
}

//Public:
void ProcessBider::setProcessName(const QString& name)
{
    mRWLock.lockForWrite();
    mProcessName = name;
    mRWLock.unlock();
}
void ProcessBider::setRespawnGrace(uint respawnGrace)
{
    mRWLock.lockForWrite();
    mRespawnGrace = respawnGrace;
    mRWLock.unlock();
}

void ProcessBider::setPollRate(uint pollRate)
{
    mRWLock.lockForWrite();
    mPollRate = pollRate;
    mRWLock.unlock();
}

/* TODO: Since this doesn't allow explicitly abandoning the wait, if for some reason the process opens itself over and over,
 * it's still possible to get stuck in a dead lock even if the close is successful
 */
ProcessBiderError ProcessBider::closeProcess()
{
    QReadLocker readLocker(&mRWLock);
    if(mWaiter && mWaiter->isWaiting() && !mWaiter->close())
    {
        ProcessBiderError err(ProcessBiderError::Close, mProcessName);
        emit errorOccurred(err);
        return err;
    }

    return ProcessBiderError();
}

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Public Slots:
void ProcessBider::start()
{
    // Start new thread for waiting
    if(!isRunning())
        QThread::start();
}
