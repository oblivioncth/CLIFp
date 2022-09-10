// Unit Include
#include "c-prepare.h"

//===============================================================================================================
// CPREPARE
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CPrepare::CPrepare(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
const QList<const QCommandLineOption*> CPrepare::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
const QString CPrepare::name() { return NAME; }

//Public:
ErrorCode CPrepare::process(const QStringList& commandLine)
{
    ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return ErrorCode::NO_ERR;

    // Get ID to prepare
    QUuid id;

    if(mParser.isSet(CL_OPTION_ID))
    {
        if((id = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_INVALID));
            return ErrorCode::ID_NOT_VALID;
        }    
    }
    else if(mParser.isSet(CL_OPTION_TITLE))
    {
        if((errorStatus = mCore.getGameIDFromTitle(id, mParser.value(CL_OPTION_TITLE))))
            return errorStatus;
    }
    else
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_TITLE));
        return ErrorCode::INVALID_ARGS;
    }

    // Enqueue prepare task
    QSqlError sqlError;

    bool titleUsesDataPack;
    if((sqlError = mCore.getFlashpointInstall().database()->entryUsesDataPack(titleUsesDataPack, id)).isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, sqlError.text()));
        return ErrorCode::SQL_ERROR;
    }

    if(titleUsesDataPack)
    {
        if((errorStatus = mCore.enqueueDataPackTasks(id)))
            return errorStatus;

        mCore.setStatus(STATUS_PREPARE, id.toString(QUuid::WithoutBraces));
    }
    else
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_PREP_NOT_DATA_PACK.arg(id.toString(QUuid::WithoutBraces))));

    // Return success
    return ErrorCode::NO_ERR;
}
