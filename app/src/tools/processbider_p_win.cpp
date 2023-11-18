// Unit Include
#include "processbider_p.h"

// Qx Includes
#include <qx_windows.h>
#include <qx/windows/qx-common-windows.h>
#include <qx/core/qx-system.h>

// Windows Include
#include <shellapi.h>

namespace
{

bool closeAdminProcess(DWORD processId, bool force)
{
    /* Killing an elevated process from this process while it is unelevated requires (without COM non-sense) starting
     * a new process as admin to do the job. While a special purpose executable could be made, taskkill already
     * perfectly suitable here
     */

    // Setup taskkill args
    QString tkArgs;
    if(force)
        tkArgs += u"/F "_s;
    tkArgs += u"/PID "_s;
    tkArgs += QString::number(processId);
    const std::wstring tkArgsStd = tkArgs.toStdWString();

    // Setup taskkill info
    SHELLEXECUTEINFOW tkExecInfo = {0};
    tkExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW); // Required
    tkExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS; // Causes hProcess member to be set to process handle
    tkExecInfo.hwnd = NULL;
    tkExecInfo.lpVerb = L"runas";
    tkExecInfo.lpFile = L"taskkill";
    tkExecInfo.lpParameters = tkArgsStd.data();
    tkExecInfo.lpDirectory = NULL;
    tkExecInfo.nShow = SW_HIDE;

    // Start taskkill
    if(!ShellExecuteEx(&tkExecInfo))
        return false;

    // Check for handle
    HANDLE tkHandle = tkExecInfo.hProcess;
    if(!tkHandle)
        return false;

    // Wait for taskkill to finish (should be fast)
    if(WaitForSingleObject(tkHandle, 5000) != WAIT_OBJECT_0)
        return false;

    DWORD exitCode;
    if(!GetExitCodeProcess(tkHandle, &exitCode))
        return false;

    // Cleanup taskkill handle
    CloseHandle(tkHandle);

    // Return taskkill result
    return exitCode == 0;
}

}

//===============================================================================================================
// ProcessWaiter
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Public:
bool ProcessWaiter::_wait()
{
    // Prevent changes while setting up the wait
    QMutexLocker mutLock(&mMutex);

    // Get process handle and see if it is valid
    DWORD rights = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
    if((mHandle = OpenProcess(rights, FALSE, mId)) == NULL) // Can use Qx::lastError() for error info here if desired
        return false;

    // Attempt to wait on process to terminate
    mutLock.unlock(); // Allow changes while waiting
    DWORD waitError = WaitForSingleObject(mHandle, INFINITE);
    mutLock.relock(); // Prevent changes during cleanup

    // Close handle to process
    CloseHandle(mHandle);
    mHandle = nullptr;

    /* Here the status can technically can be WAIT_ABANDONED, WAIT_OBJECT_0, WAIT_TIMEOUT, or WAIT_FAILED, but the first
     * and third should never occur here (the wait won't ever be abandoned, and the timeout is infinite), so this check is fine
     */
    if(waitError != WAIT_OBJECT_0) // Can use Qx::lastError() for error info here if desired
        return false;

    return true;
}

bool ProcessWaiter::_close()
{
    if(!mHandle)
        return true;

    // Prevent wait setup/cleanup during closure and auto-unlock when done
    QMutexLocker mutLock(&mMutex);

    // Check if admin rights are needed (CLIFp shouldn't be run as admin, but check anyway)
    bool selfElevated;
    if(Qx::processIsElevated(selfElevated).isValid())
        selfElevated = false; // If check fails, assume CLIFP is not elevated to be safe
    bool waitProcessElevated;
    if(Qx::processIsElevated(waitProcessElevated, mId).isValid())
        waitProcessElevated = true; // If check fails, assume process is elevated to be safe

    bool elevate = !selfElevated && waitProcessElevated;

    // Try clean close first
    if(!elevate)
        Qx::cleanKillProcess(mId);
    else
        closeAdminProcess(mId, false);

    // Wait for process to close (allow up to 2 seconds)
    DWORD waitRes = WaitForSingleObject(mHandle, 2000);

    // See if process closed
    if(waitRes == WAIT_OBJECT_0)
        return true;

    // Force close
    if(!elevate)
        return !Qx::forceKillProcess(mId).isValid();
    else
        return closeAdminProcess(mId, true);
}
