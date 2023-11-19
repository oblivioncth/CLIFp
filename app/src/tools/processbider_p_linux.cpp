// Unit Include
#include "processbider_p.h"

// Qt Includes
#include <QThread>

// Qx Includes
#include <qx/core/qx-system.h>

//===============================================================================================================
// ProcessWaiter
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Public:
bool ProcessWaiter::_wait()
{
    // Poll for process existence
    QString currentName = mName;
    while(currentName == mName)
    {
        QThread::msleep(mPollRate);
        mMutex.lock(); // Don't allow close during check
        currentName = Qx::processName(mId);
        mMutex.unlock();
    }

    // Notify that the process closed
    mCloseNotifier.wakeAll();

    return true;
}

bool ProcessWaiter::_close()
{
    // NOTE: Does not handle killing processes that require greater permissions

    // Try clean close first
    Qx::cleanKillProcess(mId);

    // See if process closes (max 2 seconds, though al)
    mMutex.lock();
    if(mCloseNotifier.wait(&mMutex, std::max(uint(2000), mPollRate + 100)))
        return true;

    // Force close
    return !Qx::forceKillProcess(mId).isValid();
}
