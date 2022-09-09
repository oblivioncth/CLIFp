// Unit Include
#include "t-exec.h"

// Project Includes
#include "utility.h"

//===============================================================================================================
// TExec
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
TExec::TExec(QObject* parent) :
    Task(parent),
    mBlockingProcess(nullptr)
{}

//-Class Functions----------------------------------------------------------------
//Private:
QString TExec::createCloseProcessString(const QProcess* process, ProcessType type)
{
    return LOG_EVENT_END_PROCESS.arg(ENUM_NAME(type), process->program());
}

//Public:
QStringList TExec::closeChildProcesses()
{
    // Ended processes
    QStringList logStrings;

    // Close each remaining child process
    for(QProcess* childProcess : qAsConst(smActiveChildProcesses))
    {
        childProcess->close();
        logStrings << createCloseProcessString(childProcess, ProcessType::Deferred);
        delete childProcess;
    }

    return logStrings;
}

//-Instance Functions-------------------------------------------------------------
//Private:
QString TExec::escapeNativeArgsForCMD(QString nativeArgs)
{
    static const QSet<QChar> escapeChars{'^','&','<','>','|'};
    QString escapedNativeArgs;
    bool inQuotes = false;

    for(int i = 0; i < nativeArgs.size(); i++)
    {
        const QChar& chr = nativeArgs.at(i);
        if(chr== '"' && (inQuotes || i != nativeArgs.lastIndexOf('"')))
            inQuotes = !inQuotes;

        escapedNativeArgs.append((!inQuotes && escapeChars.contains(chr)) ? '^' + chr : chr);
    }

    if(nativeArgs != escapedNativeArgs)
        emit eventOccurred(NAME, LOG_EVENT_ARGS_ESCAPED.arg(nativeArgs, escapedNativeArgs));

    return escapedNativeArgs;
}

bool TExec::cleanStartProcess(QProcess* process, QFileInfo exeInfo)
{
    // Start process and confirm
    process->start();
    if(!process->waitForStarted())
    {
        emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical,
                                 ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(exeInfo.absoluteFilePath()))));
        delete process; // Clear finished process handle from heap
        return false;
    }

    // Return success
    return true;
}

void TExec::logProcessStart(const QProcess* process, ProcessType type)
{
    QString eventStr = process->program();
    if(!process->arguments().isEmpty())
        eventStr += " " + process->arguments().join(" ");
    if(!process->nativeArguments().isEmpty())
        eventStr += " " + process->nativeArguments();

    emit eventOccurred(NAME, LOG_EVENT_START_PROCESS.arg(ENUM_NAME(type), eventStr));
}

void TExec::logProcessEnd(const QProcess* process, ProcessType type)
{
    emit eventOccurred(NAME, createCloseProcessString(process, type));
}

//Public:
QString TExec::name() const { return NAME; }
QStringList TExec::members() const
{
    QStringList ml = Task::members();
    ml.append(".path() = \"" + QDir::toNativeSeparators(mPath) + "\"");
    ml.append(".filename() = \"" + mFilename + "\"");
    ml.append(".parameters() = {\"" + mParameters.join(R"(", ")") + "\"}");
    ml.append(".nativeParameters() = \"" + mNativeParameters + "\"");
    ml.append(".processType() = " + ENUM_NAME(mProcessType));
    return ml;
}

QString TExec::path() const { return mPath; }
QString TExec::filename() const { return mFilename; }
const QStringList& TExec::parameters() const { return mParameters; }
QString TExec::nativeParameters() const { return mNativeParameters; }
TExec::ProcessType TExec::processType() const { return mProcessType; }

void TExec::setPath(QString path) { mPath = path; }
void TExec::setFilename(QString filename) { mFilename = filename; }
void TExec::setParameters(const QStringList& parameters) { mParameters = parameters; }
void TExec::setNativeParameters(QString nativeParameters) { mNativeParameters = nativeParameters; }
void TExec::setProcessType(ProcessType processType) { mProcessType = processType; }

void TExec::perform()
{
    // TODO: Implement proper app swaps via preferences.json
    // Check for Basilisk exception
    if(mFilename == "Basilisk-Portable.exe")
    {
        emit eventOccurred(NAME, LOG_EVENT_BASILISK_EXCEPTION);
        mFilename = "FPNavigator.exe";
        mPath.replace("Basilisk-Portable", "fpnavigator-portable");
    }

    // Ensure executable exists
    QFileInfo executableInfo(mPath + "/" + mFilename);
    if(!executableInfo.exists() || !executableInfo.isFile())
    {
        emit errorOccurred(NAME, Qx::GenericError(mStage == Stage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                                  ERR_EXE_NOT_FOUND.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
        emit complete(ErrorCode::EXECUTABLE_NOT_FOUND);
        return;
    }

    // Ensure executable is valid
    if(!executableInfo.isExecutable())
    {
        emit errorOccurred(NAME, Qx::GenericError(mStage == Stage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                                 ERR_EXE_NOT_VALID.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
        emit complete(ErrorCode::EXECUTABLE_NOT_VALID);
        return;
    }

    // Move to executable directory
    QDir::setCurrent(mPath);
    emit eventOccurred(NAME, LOG_EVENT_CD.arg(QDir::toNativeSeparators(mPath)));

    // Check if task is a batch file
    bool batchTask = QFileInfo(mFilename).suffix() == BAT_SUFX;

    // Create process handle
    QProcess* taskProcess = new QProcess();
    taskProcess->setProgram(batchTask ? CMD_EXE : mFilename);
    taskProcess->setArguments(mParameters);
    taskProcess->setNativeArguments(batchTask ? CMD_ARG_TEMPLATE.arg(mFilename, escapeNativeArgsForCMD(mNativeParameters))
                                              : mNativeParameters);
    taskProcess->setStandardOutputFile(QProcess::nullDevice()); // Don't inherit console window
    taskProcess->setStandardErrorFile(QProcess::nullDevice()); // Don't inherit console window

    // Cover each process type
    switch(mProcessType)
    {
        case ProcessType::Blocking:
            taskProcess->setParent(this);
            if(!cleanStartProcess(taskProcess, executableInfo))
            {
                emit complete(ErrorCode::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, ProcessType::Blocking);

            // Now wait on the process asynchronously
            mBlockingProcess = taskProcess;
            connect(mBlockingProcess, &QProcess::finished, this, &TExec::postBlockingProcess);
            return;

        case ProcessType::Deferred:
            taskProcess->setParent(this);
            if(!cleanStartProcess(taskProcess, executableInfo))
            {
                emit complete(ErrorCode::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, ProcessType::Deferred);

            smActiveChildProcesses.append(taskProcess); // Add process to list for deferred termination
            break;

        case ProcessType::Detached:
            if(!taskProcess->startDetached())
            {
                emit complete(ErrorCode::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, ProcessType::Detached);
            break;
    }

    // Return success
    emit complete(ErrorCode::NO_ERR);
}

void TExec::stop()
{
    if(mBlockingProcess)
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_MAIN_PROCESS);
        // NOTE: Careful in this function, once the process is dead and the finishedBlockingExecutionHandler
        // slot has been invoked, mMainBlockingProcess gets reset to nullptr

        // Try soft kill
        mBlockingProcess->terminate();
        bool closed = mBlockingProcess->waitForFinished(2000); // Allow up to 2 seconds to close
        /* NOTE: Initially there was concern that the above has a race condition in that the app could
         * finish closing after terminate was invoked, but before the portion of waitForFinished
         * that checks if the process is still running (since it returns false if it isn't) is reached.
         * This would make the return value misleading (it would appear as if the process is still running
         * (I think this behavior of waitForFinished is silly but that's an issue for another time); however,
         * debug testing showed that after sitting at a break point before waitForFinished, but after terminate
         * for a significant amount of time and then executing waitForFinished it still initially treated the
         * process as if it was running. Likely QProcess isn't made aware that the app stopped until some kind
         * of event is processed after it ends that is processed within waitForFinished, meaning there is no
         * race condition
         */

        // Hard kill if necessary
        if(!closed)
            mBlockingProcess->kill(); // Hard kill
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TExec::postBlockingProcess()
{
    // Ensure all is well
    if(!mBlockingProcess)
        throw std::runtime_error(Q_FUNC_INFO " called with when the main blocking process pointer was null.");

    // Handle process cleanup
    logProcessEnd(mBlockingProcess, ProcessType::Blocking);
    mBlockingProcess->deleteLater(); // Clear finished process handle from heap when possible
    mBlockingProcess = nullptr;

    // Return success
    emit complete(ErrorCode::NO_ERR);
}
