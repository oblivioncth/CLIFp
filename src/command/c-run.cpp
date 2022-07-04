// Unit Include
#include "c-run.h"

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
        return Core::ErrorCodes::NO_ERR;

    // Make sure that at least an app was provided
    if(!mParser.isSet(CL_OPTION_PARAM))
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_APP));
        return Core::ErrorCodes::INVALID_ARGS;
    }

    // Enqueue startup tasks
    if((errorStatus = mCore.enqueueStartupTasks()))
        return errorStatus;

    QFileInfo inputInfo = QFileInfo(mCore.getFlashpointInstall().fullPath() + '/' + mParser.value(CL_OPTION_APP));

    std::shared_ptr<Core::ExecTask> runTask = std::make_shared<Core::ExecTask>();
    runTask->stage = Core::TaskStage::Primary;
    runTask->path = inputInfo.absolutePath();
    runTask->filename = inputInfo.fileName();
    runTask->param = QStringList();
    runTask->nativeParam = mParser.value(CL_OPTION_PARAM);
    runTask->processType = Core::ProcessType::Blocking;

    mCore.enqueueSingleTask(runTask);
    mCore.setStatus(STATUS_RUN, runTask->filename);

    // Add wait task if required
    if((errorStatus = mCore.enqueueConditionalWaitTask(inputInfo)))
        return errorStatus;

    // Return success
    return Core::ErrorCodes::NO_ERR;
}
