// Unit Include
#include "t-exec.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>

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
QString TExec::collapseArguments(const QStringList& args)
{
    QString reduction;
    for(int i = 0; i < args.size(); i++)
    {
        const QString& param = args[i];
        if(!param.isEmpty())
        {
            if(param.contains(Qx::RegularExpression::WHITESPACE) && !(param.front() == '\"' && param.back() == '\"'))
                reduction += '\"' + param + '\"';
            else
                reduction += param;

            if(i != args.size() - 1)
                reduction += ' ';
        }

    }

    return reduction;
}

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
bool TExec::cleanStartProcess(QProcess* process, QFileInfo exeInfo)
{
    // Start process and confirm
    const QString currentDirPath = QDir::currentPath();
    QDir::setCurrent(mPath);
    emit eventOccurred(NAME, LOG_EVENT_CD.arg(QDir::toNativeSeparators(mPath)));
    process->start();
    QDir::setCurrent(currentDirPath);
    emit eventOccurred(NAME, LOG_EVENT_CD.arg(QDir::toNativeSeparators(currentDirPath)));
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
    if(std::holds_alternative<QString>(mParameters))
        ml.append(".parameters() = \"" + std::get<QString>(mParameters) + "\"");
    else
        ml.append(".parameters() = {\"" + std::get<QStringList>(mParameters).join(R"(", ")") + "\"}");
    ml.append(".processType() = " + ENUM_NAME(mProcessType));
    return ml;
}

QString TExec::path() const { return mPath; }
QString TExec::filename() const { return mFilename; }
const std::variant<QString, QStringList>& TExec::parameters() const { return mParameters; }
TExec::ProcessType TExec::processType() const { return mProcessType; }

void TExec::setPath(QString path) { mPath = path; }
void TExec::setFilename(QString filename) { mFilename = filename; }
void TExec::setParameters(const std::variant<QString, QStringList>& parameters) { mParameters = parameters; }
void TExec::setProcessType(ProcessType processType) { mProcessType = processType; }

void TExec::perform()
{
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

    // Check if task requires a shell
    bool requiresShell = executableInfo.suffix() == SHELL_EXT_WIN || executableInfo.suffix() == SHELL_EXT_LINUX;

    // Create process handle
    QProcess* taskProcess;
    if(requiresShell)
        taskProcess = prepareShellProcess();
    else
        taskProcess = prepareDirectProcess();

    // Set common process properties
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
             // Can't use 'this' as parent since process will outlive this instance, so use parent of task
            taskProcess->setParent(this->parent());
            if(!cleanStartProcess(taskProcess, executableInfo))
            {
                emit complete(ErrorCode::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, ProcessType::Deferred);

            smActiveChildProcesses.append(taskProcess); // Add process to list for deferred termination
            break;

        case ProcessType::Detached:
            // TODO: Test if a parent can be set in this case instead of leaving taskProcess completely dangling.
            // It's that way for now because IIRC even when using startDetached(), the QProcess destructor still stops the process
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
        throw std::runtime_error(std::string(Q_FUNC_INFO) + " called with when the main blocking process pointer was null.");

    // Handle process cleanup
    logProcessEnd(mBlockingProcess, ProcessType::Blocking);
    mBlockingProcess->deleteLater(); // Clear finished process handle from heap when possible
    mBlockingProcess = nullptr;

    // Return success
    emit complete(ErrorCode::NO_ERR);
}
