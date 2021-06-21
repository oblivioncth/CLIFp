#include "c-show.h"

//===============================================================================================================
// CSHOW
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CShow::CShow(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
const QList<const QCommandLineOption*> CShow::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
const QString CShow::name() { return NAME; }

//Public:
Core::ErrorCode CShow::process(const QStringList& commandLine)
{
    Core::ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return Core::NO_ERR;

    // Enqueue show task
    if(mParser.isSet(CL_OPTION_MSG))
    {
        std::shared_ptr<Core::MessageTask> messageTask = std::make_shared<Core::MessageTask>();
        messageTask->stage = Core::TaskStage::Primary;
        messageTask->message = mParser.value(CL_OPTION_MSG);
        messageTask->modal = true;

        mCore.enqueueSingleTask(messageTask);
    }
    else if(mParser.isSet(CL_OPTION_EXTRA))
    {
        std::shared_ptr<Core::ExtraTask> extraTask = std::make_shared<Core::ExtraTask>();
        extraTask->stage = Core::TaskStage::Primary;
        extraTask->dir = QDir(mCore.getFlashpointInstall().getExtrasDirectory().absolutePath() + "/" + mParser.value(CL_OPTION_EXTRA));

        mCore.enqueueSingleTask(extraTask);
    }
    else
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_SHOW));
        return Core::INVALID_ARGS;
    }

    // Return success
    return Core::NO_ERR;
}
