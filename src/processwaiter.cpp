// Unit Include
#include "processwaiter.h"

//===============================================================================================================
// ProcessWaiter
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
ProcessWaiter::ProcessWaiter(QString processName, uint respawnGrace, QObject* parent) :
    QThread(parent),
    mProcessName(processName),
    mRespawnGrace(respawnGrace),
    mProcessHandle(nullptr)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
bool ProcessWaiter::processIsRunning()
{
    DWORD exitCode;
    if(!GetExitCodeProcess(mProcessHandle, &exitCode))
        return false;

    if(exitCode != STILL_ACTIVE)
        return false;
    else
    {
        /* Due to a design oversight, it's possible for a process to return the value
         * associated with STILL_ACTIVE as its exit code, which would make it look
         * like it's still running here when it isn't, so a different method must be
         * used to double check
         */

         // Zero timeout means check if "signaled" (i.e. dead) state immediately
         if(WaitForSingleObject(mProcessHandle, 0) == WAIT_TIMEOUT)
             return true;
         else
             return false;
    }
}

ErrorCode ProcessWaiter::doWait()
{
    // Lock other threads from interaction while managing process handle
    QMutexLocker handleLocker(&mProcessHandleMutex);

    // Wait until process has stopped running for grace period
    DWORD spProcessID;
    do
    {
        // Yield for grace period
        emit statusChanged(LOG_EVENT_WAIT_GRACE.arg(mRespawnGrace).arg(mProcessName));
        if(mRespawnGrace > 0)
            QThread::sleep(mRespawnGrace);

        // Find process ID by name
        spProcessID = Qx::processIdByName(mProcessName);

        // Check that process was found (is running)
        if(spProcessID)
        {
            emit statusChanged(LOG_EVENT_WAIT_RUNNING.arg(mProcessName));

            // Get process handle and see if it is valid
            if((mProcessHandle = OpenProcess(PROCESS_HANDLE_ACCESS_RIGHTS, FALSE, spProcessID)) == NULL)
            {
                Qx::GenericError nativeError = Qx::translateHresult(HRESULT_FROM_WIN32(GetLastError()));

                emit errorOccured(Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HANDLED_P.arg(mProcessName),
                                                   nativeError.primaryInfo()));
                return Core::ErrorCodes::WAIT_PROCESS_NOT_HANDLED;
            }

            // Attempt to wait on process to terminate
            emit statusChanged(LOG_EVENT_WAIT_ON.arg(mProcessName));
            handleLocker.unlock(); // Allow interaction while waiting
            DWORD waitError = WaitForSingleObject(mProcessHandle, INFINITE);
            handleLocker.relock(); // Explicitly lock again

            // Close handle to process
            CloseHandle(mProcessHandle);
            mProcessHandle = nullptr;

            /* Here the status can technically can be WAIT_ABANDONED, WAIT_OBJECT_0, WAIT_TIMEOUT, or WAIT_FAILED, but the first
             * and third should never occur here (the wait won't ever be abandoned, and the timeout is infinite), so this check is fine
             */
            if(waitError != WAIT_OBJECT_0)
            {
                Qx::GenericError nativeError = Qx::translateHresult(HRESULT_FROM_WIN32(GetLastError()));

                emit errorOccured(Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HOOKED_P.arg(mProcessName),
                                                   nativeError.primaryInfo()));
                return Core::ErrorCodes::WAIT_PROCESS_NOT_HOOKED;
            }
            emit statusChanged(LOG_EVENT_WAIT_QUIT.arg(mProcessName));
        }
    }
    while(spProcessID);

    // Return success
    emit statusChanged(LOG_EVENT_WAIT_FINISHED.arg(mProcessName));
    return Core::ErrorCodes::NO_ERR;
}

void ProcessWaiter::run()
{
    ErrorCode status = doWait();
    emit waitFinished(status);
}

//Public:
// Straight from the source: https://github.com/qt/qtbase/blob/05fc3aef53348fb58be6308076e000825b704e58/src/corelib/io/qprocess_win.cpp#L614
static BOOL QT_WIN_CALLBACK qt_terminateApp(HWND hwnd, LPARAM procId)
{
    DWORD currentProcId = 0;
    GetWindowThreadProcessId(hwnd, &currentProcId);
    if (currentProcId == (DWORD)procId)
        PostMessage(hwnd, WM_CLOSE, 0, 0);

    return TRUE;
}

bool ProcessWaiter::stopProcess()
{
    if(!mProcessHandle)
        return false;

    // Lock access to handle and auto-unlock when done
    QMutexLocker handleLocker(&mProcessHandleMutex);

    // Get additional process info
    DWORD processId = GetProcessId(mProcessHandle);
    DWORD threadId = 0;
    QList<DWORD> allProcessThreadIds = Qx::processThreadIds(processId);
    if(!allProcessThreadIds.empty())
        threadId = allProcessThreadIds.first(); // Assume first thread is main thread (not perfect, but good in most cases).

    // Try to notify process to close (allow 1 second)
    EnumWindows(qt_terminateApp, (LPARAM)processId); // Tells all top-level windows of the process to close
    if(threadId)
        PostThreadMessage(threadId, WM_CLOSE, 0, 0); // Tells the main thread of the process to close
    QThread::sleep(1); // This blocks the calling thread, not the one in run

    // See if process closed
    if(!processIsRunning())
        return true;

    // Force stop the process
    return TerminateProcess(mProcessHandle, 0xFFFF);
}

void ProcessWaiter::deleteLater() { QObject::deleteLater(); }

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Public Slots:
void ProcessWaiter::start()
{
    // Start new thread for waiting
    if(!isRunning())
        QThread::start();
}
