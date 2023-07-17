// Unit Include
#include "processbider.h"

// Qx Includes
#include <qx_windows.h>
#include <qx/core/qx-system.h>
#include <qx/core/qx-error.h>

// Windows Include
#include <shellapi.h>

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
ProcessBider::ProcessBider(QObject* parent, uint respawnGrace) :
    QThread(parent),
    mRespawnGrace(respawnGrace),
    mProcessHandle(nullptr)
{}

//-Class Functions-------------------------------------------------------------
//Private:
bool ProcessBider::closeAdminProcess(DWORD processId, bool force)
{
    /* Killing an elevated process from this process while it is unelevated requires (without COM non-sense) starting
     * a new process as admin to do the job. While a special purpose executable could be made, taskkill already
     * perfectly suitable here
     */

    // Setup taskkill args
    QString tkArgs;
    if(force)
        tkArgs += "/F ";
    tkArgs += "/PID ";
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

//-Instance Functions-------------------------------------------------------------
//Private:
ProcessBiderError ProcessBider::doWait()
{
    // Lock other threads from interaction while managing process handle
    QMutexLocker handleLocker(&mProcessHandleMutex);

    // Wait until process has stopped running for grace period
    DWORD spProcessId;
    do
    {
        // Yield for grace period
        emit statusChanged(LOG_EVENT_BIDE_GRACE.arg(mRespawnGrace).arg(mProcessName));
        if(mRespawnGrace > 0)
            QThread::sleep(mRespawnGrace);

        // Find process ID by name
        spProcessId = Qx::processId(mProcessName);

        // Check that process was found (is running)
        if(spProcessId)
        {
            emit statusChanged(LOG_EVENT_BIDE_RUNNING.arg(mProcessName));

            // Get process handle and see if it is valid
            DWORD rights = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
            if((mProcessHandle = OpenProcess(rights, FALSE, spProcessId)) == NULL)
            {
                Qx::SystemError nativeError = Qx::SystemError::fromHresult(HRESULT_FROM_WIN32(GetLastError()));
                ProcessBiderError err(ProcessBiderError::HandleAquisition, nativeError.cause());

                emit errorOccured(err);
                return err;
            }

            // Attempt to wait on process to terminate
            emit statusChanged(LOG_EVENT_BIDE_ON.arg(mProcessName));
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
                Qx::SystemError nativeError = Qx::SystemError::fromHresult(HRESULT_FROM_WIN32(GetLastError()));
                ProcessBiderError err(ProcessBiderError::ProcessHook, nativeError.cause());

                emit errorOccured(err);
                return err;
            }
            emit statusChanged(LOG_EVENT_BIDE_QUIT.arg(mProcessName));
        }
    }
    while(spProcessId);

    // Return success
    emit statusChanged(LOG_EVENT_BIDE_FINISHED.arg(mProcessName));
    return ProcessBiderError();
}

void ProcessBider::run()
{
    ProcessBiderError status = doWait();
    emit bideFinished(status);
}

//Public:
void ProcessBider::setRespawnGrace(uint respawnGrace) { mRespawnGrace = respawnGrace; }

bool ProcessBider::closeProcess()
{
    if(!mProcessHandle)
        return false;

    // Lock access to handle and auto-unlock when done
    QMutexLocker handleLocker(&mProcessHandleMutex);

    /* Get process ID for use in some of the following calls so that the specific permissions the mProcessHandle
     * was opened with don't have to be considered
     */
    DWORD processId = GetProcessId(mProcessHandle);

    // Check if admin rights are needed (CLIFp shouldn't be run as admin, but check anyway)
    bool selfElevated;
    if(Qx::processIsElevated(selfElevated).isValid())
        selfElevated = false; // If check fails, assume CLIFP is not elevated to be safe
    bool waitProcessElevated;
    if(Qx::processIsElevated(waitProcessElevated, processId).isValid())
        waitProcessElevated = true; // If check fails, assume process is elevated to be safe

    bool elevate = !selfElevated && waitProcessElevated;

    // Try clean close first
    if(!elevate)
        Qx::cleanKillProcess(processId);
    else
        closeAdminProcess(processId, false);

    // Wait for process to close (allow up to 2 seconds)
    DWORD waitRes = WaitForSingleObject(mProcessHandle, 2000);

    // See if process closed
    if(waitRes == WAIT_OBJECT_0)
        return true;

    // Force close
    if(!elevate)
        return !Qx::forceKillProcess(processId).isValid();
    else
        return closeAdminProcess(processId, true);
}

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Public Slots:
void ProcessBider::start(QString processName)
{
    // Start new thread for waiting
    if(!isRunning())
    {
        mProcessName = processName;
        QThread::start();
    }
}
