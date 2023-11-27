// Unit Include
#include "t-exec.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// TExecError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TExecError::TExecError(Type t, const QString& s, Qx::Severity sv) :
    mType(t),
    mSpecific(s),
    mSeverity(sv)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TExecError::isValid() const { return mType != NoError; }
QString TExecError::specific() const { return mSpecific; }
TExecError::Type TExecError::type() const { return mType; }

//Private:
Qx::Severity TExecError::deriveSeverity() const { return mSeverity; }
quint32 TExecError::deriveValue() const { return mType; }
QString TExecError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TExecError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// TExec
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
TExec::TExec(QObject* parent) :
    Task(parent),
    mEnvironment(smDefaultEnv),
    mBlockingProcessManager(nullptr)
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
void TExec::setDefaultProcessEnvironment(const QProcessEnvironment pe) { smDefaultEnv = pe; }
QProcessEnvironment TExec::defaultProcessEnvironment() { return smDefaultEnv; }

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
            emitEventOccurred(LOG_EVENT_ARGS_ESCAPED.arg(args, escapedArgs));
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
            emitEventOccurred(LOG_EVENT_ARGS_ESCAPED.arg(u"{\""_s + parameters.join(uR"(", ")"_s) + u"\"}"_s,
                                                                u"{\""_s + rebuild.join(uR"(", ")"_s) + u"\"}"_s));
        }
    }

    return escapedArgs;
}

void TExec::removeRedundantFullQuotes(QProcess& process)
{
    /* Sometimes service arguments have been observed to be "pre-prepped" for shell use by being fully quoted even
     * when not needed. This is an issue since QProcess will quote non-native arguments automatically when not using
     * the shell, which we don't for most things other than scripts, so we have to remove such quotes here. Note this
     * affects all execution tasks though, not just services.
     */
    QStringList args = process.arguments();
    for(QString& a : args)
    {
        // Determine if arg is simply fully quoted
        if(a.size() < 3 || (a.front() != '"' && a.back() != '"')) // min 3 maintains " and "" which theoretically could be significant
            continue;

        QStringView inner(a.cbegin() + 1, a.cend() - 1);
        bool redundant = true;
        bool escaped = false;
        for(const QChar& c : inner)
        {
            if(c == '\\')
                escaped = true;
            else if(c == '"' && !escaped)
            {
                redundant = false;
                break;
            }
            else
                escaped = false;
        }

        if(redundant)
        {
            emitEventOccurred(LOG_EVENT_REMOVED_REDUNDANT_QUOTES.arg(a));
            a = inner.toString();
        }
    }

    process.setArguments(args);
}

TExecError TExec::cleanStartProcess(QProcess* process)
{
    // Note directories
    const QString currentDirPath = QDir::currentPath();
    const QString newDirPath = mDirectory.absolutePath();

    // Go to working directory
    QDir::setCurrent(newDirPath);
    emitEventOccurred(LOG_EVENT_CD.arg(QDir::toNativeSeparators(newDirPath)));

    // Start process
    process->start();
    emitEventOccurred(LOG_EVENT_STARTING.arg(mIdentifier, process->program()));

    // Return to previous working directory
    QDir::setCurrent(currentDirPath);
    emitEventOccurred(LOG_EVENT_CD.arg(QDir::toNativeSeparators(currentDirPath)));

    // Make sure process starts
    if(!process->waitForStarted())
    {
        TExecError err(TExecError::CouldNotStart, ERR_DETAILS_TEMPLATE.arg(process->program(), ENUM_NAME(process->error())));
        emitErrorOccurred(err);
        delete process; // Clear finished process handle from heap
        return err;
    }

    // Return success
    emitEventOccurred(LOG_EVENT_STARTED_PROCESS.arg(mIdentifier));
    return TExecError();
}

//Public:
QString TExec::name() const { return NAME; }
QStringList TExec::members() const
{
    QStringList ml = Task::members();
    ml.append(u".executable() = \""_s + mExecutable + u"\""_s);
    ml.append(u".directory() = \""_s + mDirectory.absolutePath() + u"\""_s);
    if(std::holds_alternative<QString>(mParameters))
        ml.append(u".parameters() = \""_s + std::get<QString>(mParameters) + u"\""_s);
    else
        ml.append(u".parameters() = {\""_s + std::get<QStringList>(mParameters).join(uR"(", ")"_s) + u"\"}"_s);
    ml.append(u".processType() = "_s + ENUM_NAME(mProcessType));
    ml.append(u".identifier() = \""_s + mIdentifier + u"\""_s);
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
    emitEventOccurred(LOG_EVENT_PREPARING_PROCESS.arg(ENUM_NAME(mProcessType), mIdentifier, mExecutable));

    // Get final executable path
    QString execPath = resolveExecutablePath();
    if(execPath.isEmpty())
    {
        TExecError err(TExecError::CouldNotFind, mExecutable, mStage == Stage::Shutdown ? Qx::Err : Qx::Critical);
        emitErrorOccurred(err);
        emit complete(err);
        return;
    }

    // Prepare process object
    QProcess* taskProcess = prepareProcess(QFileInfo(execPath));
    removeRedundantFullQuotes(*taskProcess);
    logPreparedProcess(taskProcess);

    // Set common process properties
    taskProcess->setProcessEnvironment(mEnvironment);

    // Cover each process type
    switch(mProcessType)
    {
        case ProcessType::Blocking:
            // Setup blocking process manager (it adopts the process)
            mBlockingProcessManager = new BlockingProcessManager(taskProcess, mIdentifier, this);
            connect(mBlockingProcessManager, &BlockingProcessManager::eventOccurred, this, &TExec::eventOccurred);
            connect(mBlockingProcessManager, &BlockingProcessManager::finished, this, &TExec::postBlockingProcess);

            // Setup and start process
            if(TExecError se = cleanStartProcess(taskProcess); se.isValid())
            {
                emit complete(se);
                return;
            }

            // Now wait on the process asynchronously...
            return;

        case ProcessType::Deferred:
             // Can't use 'this' as parent since process will outlive this instance, so use parent of task
            taskProcess->setParent(this->parent());
            if(TExecError se = cleanStartProcess(taskProcess); se.isValid())
            {
                emit complete(se);
                return;
            }

            if(smDeferredProcessManager)
                smDeferredProcessManager->manage(mIdentifier, taskProcess); // Add process to list for deferred termination
            else
                qWarning("Deferred process started without a deferred process manager installed!");
            break;

        case ProcessType::Detached:
            // The parent here doesn't really matter since the program is detached, so just make it the same as deferred
            taskProcess->setParent(this->parent());

            // startDetached causes parent shell to be inherited, which isn't desired here, so manually disconnect them
            taskProcess->setStandardOutputFile(QProcess::nullDevice());
            taskProcess->setStandardErrorFile(QProcess::nullDevice());
            if(!taskProcess->startDetached())
            {
                TExecError err(TExecError::CouldNotStart, ERR_DETAILS_TEMPLATE.arg(taskProcess->program(), ENUM_NAME(taskProcess->error())));
                emitErrorOccurred(err);
                emit complete(err);
                return;
            }
            break;
    }

    // Return success
    emit complete(TExecError());
}

void TExec::stop()
{
    if(mBlockingProcessManager)
    {
        emitEventOccurred(LOG_EVENT_STOPPING_BLOCKING_PROCESS.arg(mIdentifier));
        mBlockingProcessManager->closeProcess();
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TExec::postBlockingProcess()
{
    // The BPM will get deleted automatically when this destructs since it's a child,
    // but drop the pointer to it now anyway just to avoid shenanigans with "stop()".
    mBlockingProcessManager = nullptr;

    // Return success
    emit complete(TExecError());
}
