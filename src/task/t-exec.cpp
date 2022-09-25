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

//Public:
void TExec::installDeferredProcessManager(DeferredProcessManager* manager) { smDeferredProcessManager = manager; }
DeferredProcessManager* TExec::deferredProcessManager() { return smDeferredProcessManager; }

//-Instance Functions-------------------------------------------------------------
//Private:
QString TExec::createEscapedShellArguments()
{
    // Determine arguments
    QString escapedArgs;
    if(std::holds_alternative<QString>(mParameters))
    {
        // Escape
        QString args = std::get<QString>(mParameters);
        escapedArgs = escapeForShell(args);
        if(args != escapedArgs)
            emit eventOccurred(NAME, LOG_EVENT_ARGS_ESCAPED.arg(args, escapedArgs));
    }
    else
    {
        // Collapse
        QStringList parameters = std::get<QStringList>(mParameters);
        QString collapsedParameters = collapseArguments(parameters);

        // Escape
        escapedArgs = escapeForShell(collapsedParameters);

        QStringList rebuild = QProcess::splitCommand(escapedArgs);
        if(rebuild != parameters)
        {
            emit eventOccurred(NAME, LOG_EVENT_ARGS_ESCAPED.arg("{\"" + parameters.join(R"(", ")") + "\"}",
                                                                "{\"" + rebuild.join(R"(", ")") + "\"}"));
        }
    }

    return escapedArgs;
}

bool TExec::cleanStartProcess(QProcess* process)
{
    // Note directories
    const QString currentDirPath = QDir::currentPath();
    const QString newDirPath = mDirectory.absolutePath();

    // Go to working directory
    QDir::setCurrent(newDirPath);
    emit eventOccurred(NAME, LOG_EVENT_CD.arg(QDir::toNativeSeparators(newDirPath)));

    // Start process
    process->start();
    emit eventOccurred(NAME, LOG_EVENT_INIT_PROCESS.arg(mIdentifier, mExecutable));

    // Return to previous working directory
    QDir::setCurrent(currentDirPath);
    emit eventOccurred(NAME, LOG_EVENT_CD.arg(QDir::toNativeSeparators(currentDirPath)));

    // Make sure process starts
    if(!process->waitForStarted())
    {
        emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical,
                                 ERR_EXE_NOT_STARTED.arg(mExecutable),
                                 ENUM_NAME(process->error())));
        delete process; // Clear finished process handle from heap
        return false;
    }

    // Return success
    return true;
}

void TExec::logProcessEnd(QProcess* process, ProcessType type)
{
    QString output = process->readAll();

    // Don't print extra linebreak
    if(!output.isEmpty() && output.back() == '\n')
        output.chop(1);

    emit eventOccurred(NAME, LOG_EVENT_END_PROCESS.arg(ENUM_NAME(type), mIdentifier, process->program(), output));
}

//Public:
QString TExec::name() const { return NAME; }
QStringList TExec::members() const
{
    QStringList ml = Task::members();
    ml.append(".executable() = \"" + mExecutable + "\"");
    ml.append(".directory() = \"" + mDirectory.absolutePath() + "\"");
    if(std::holds_alternative<QString>(mParameters))
        ml.append(".parameters() = \"" + std::get<QString>(mParameters) + "\"");
    else
        ml.append(".parameters() = {\"" + std::get<QStringList>(mParameters).join(R"(", ")") + "\"}");
    ml.append(".processType() = " + ENUM_NAME(mProcessType));
    ml.append(".identifier() = \"" + mIdentifier + "\"");
    return ml;
}

QString TExec::executable() const { return mExecutable; }
QDir TExec::directory() const { return mDirectory; }
const std::variant<QString, QStringList>& TExec::parameters() const { return mParameters; }
const QProcessEnvironment& TExec::environment() const { return mEnvironment; }
TExec::ProcessType TExec::processType() const { return mProcessType; }
QString TExec::identifier() const { return mIdentifier; }

void TExec::setExecutable(QString executable) { mExecutable = executable; }
void TExec::setDirectory(QDir directory) { mDirectory = directory; }
void TExec::setParameters(const std::variant<QString, QStringList>& parameters) { mParameters = parameters; }
void TExec::setEnvironment(const QProcessEnvironment& environment) { mEnvironment = environment; }
void TExec::setProcessType(ProcessType processType) { mProcessType = processType; }
void TExec::setIdentifier(QString identifier) { mIdentifier = identifier; }

void TExec::perform()
{
    // Get final executable path
    QString execPath = resolveExecutablePath();
    if(execPath.isEmpty())
    {
        emit errorOccurred(NAME, Qx::GenericError(mStage == Stage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                                  ERR_EXE_NOT_FOUND.arg(mExecutable)));
        emit complete(ErrorCode::EXECUTABLE_NOT_FOUND);
        return;
    }

    // Prepare process
    QProcess* taskProcess = prepareProcess(QFileInfo(execPath));

    // Set common process properties
    if(!mEnvironment.isEmpty()) // Don't override the QProcess default (use system env.) if no custom env. was set
        taskProcess->setProcessEnvironment(mEnvironment);
    taskProcess->setProcessChannelMode(QProcess::MergedChannels);

    // Cover each process type
    switch(mProcessType)
    {
        case ProcessType::Blocking:
            taskProcess->setParent(this);
            if(!cleanStartProcess(taskProcess))
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
            if(!cleanStartProcess(taskProcess))
            {
                emit complete(ErrorCode::PROCESS_START_FAIL);
                return;
            }
            logProcessStart(taskProcess, ProcessType::Deferred);

            if(smDeferredProcessManager)
                smDeferredProcessManager->manage(mIdentifier, taskProcess); // Add process to list for deferred termination
            else
                qWarning("Deferred process started without a deferred process manager installed!");
            break;

        case ProcessType::Detached:
            // TODO: Test if a parent can be set in this case instead of leaving taskProcess completely dangling.
            // It's that way for now because IIRC even when using startDetached(), the QProcess destructor still stops the process

            // startDetached causes parent shell to be inherited, which isn't desired here, so manually disconnect them
            taskProcess->setStandardOutputFile(QProcess::nullDevice());
            taskProcess->setStandardErrorFile(QProcess::nullDevice());
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
