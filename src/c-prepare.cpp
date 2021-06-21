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
Core::ErrorCode CPrepare::process(const QStringList& commandLine)
{
    Core::ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return Core::NO_ERR;

    // Enqueue prepare task
    if(mParser.isSet(CL_OPTION_ID))
    {
        Core::ErrorCode errorStatus;
        QSqlError sqlError;
        QUuid id;

        if((id = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_INVALID));
            return Core::ID_NOT_VALID;
        }

        // Open database
        if((errorStatus = mCore.openAndVerifyProperDatabase()))
            return errorStatus;

        bool titleUsesDataPack;
        if((sqlError = mCore.getFlashpointInstall().entryUsesDataPack(titleUsesDataPack, id)).isValid())
        {
            mCore.postError(Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, sqlError.text()));
            return Core::SQL_ERROR;
        }

        if(titleUsesDataPack)
        {
            if((errorStatus = mCore.enqueueDataPackTasks(id)))
                return errorStatus;
        }
        else
            mCore.logError(Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_PREP_NOT_DATA_PACK.arg(id.toString(QUuid::WithoutBraces))));
    }
    else
    {
        mCore.logError(Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_ID));
        return Core::INVALID_ARGS;
    }

    // Return success
    return Core::NO_ERR;
}
