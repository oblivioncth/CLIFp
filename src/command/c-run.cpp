// Unit Include
#include "c-run.h"

// Project Includes
#include "task/t-exec.h"

//===============================================================================================================
// CRUN
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CRun::CRun(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
const QList<const QCommandLineOption*> CRun::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
const QString CRun::name() { return NAME; }

//Public:
ErrorCode CRun::process(const QStringList& commandLine)
{
    ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return ErrorCode::NO_ERR;

    // Make sure that at least an app was provided
    if(!mParser.isSet(CL_OPTION_PARAM))
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_APP));
        return ErrorCode::INVALID_ARGS;
    }

    // Enqueue startup tasks
    if((errorStatus = mCore.enqueueStartupTasks()))
        return errorStatus;

    QString inputPath = mCore.resolveTrueAppPath(mParser.value(CL_OPTION_APP), ""); // No way of knowing platform
    QFileInfo inputInfo = QFileInfo(mCore.fpInstall().fullPath() + '/' + inputPath);

    TExec* runTask = new TExec(&mCore);
    runTask->setIdentifier(NAME + " program");
    runTask->setStage(Task::Stage::Primary);
    runTask->setPath(inputInfo.absolutePath());
    runTask->setFilename(inputInfo.fileName());
    runTask->setParameters(mParser.value(CL_OPTION_PARAM));
    runTask->setEnvironment(mCore.childTitleProcessEnvironment());
    runTask->setProcessType(TExec::ProcessType::Blocking);

    mCore.enqueueSingleTask(runTask);
    mCore.setStatus(STATUS_RUN, runTask->filename());

#ifdef _WIN32
    // Add wait task if required
    if((errorStatus = mCore.conditionallyEnqueueBideTask(inputInfo)))
        return errorStatus;
#endif

    // Return success
    return ErrorCode::NO_ERR;
}
