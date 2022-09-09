// Unit Include
#include "c-show.h"

// Project Includes
#include "task/t-message.h"
#include "task/t-extra.h"

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
ErrorCode CShow::process(const QStringList& commandLine)
{
    ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return ErrorCode::NO_ERR;

    // Enqueue show task
    if(mParser.isSet(CL_OPTION_MSG))
    {
        std::shared_ptr<TMessage> messageTask = std::make_shared<TMessage>();
        messageTask->setStage(Task::Stage::Primary);
        messageTask->setMessage(mParser.value(CL_OPTION_MSG));
        messageTask->setModal(true);

        mCore.enqueueSingleTask(messageTask);
        mCore.setStatus(STATUS_SHOW_MSG, messageTask->message());
    }
    else if(mParser.isSet(CL_OPTION_EXTRA))
    {
        std::shared_ptr<TExtra> extraTask = std::make_shared<TExtra>();
        extraTask->setStage(Task::Stage::Primary);
        extraTask->setDirectory(QDir(mCore.getFlashpointInstall().extrasDirectory().absolutePath() + "/" + mParser.value(CL_OPTION_EXTRA)));

        mCore.enqueueSingleTask(extraTask);
        mCore.setStatus(STATUS_SHOW_EXTRA, extraTask->directory().dirName());
    }
    else
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_SHOW));
        return ErrorCode::INVALID_ARGS;
    }

    // Return success
    return ErrorCode::NO_ERR;
}
