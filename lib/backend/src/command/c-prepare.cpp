// Unit Include
#include "c-prepare.h"

// Qx Includes
#include <qx/core/qx-genericerror.h>

// Project Includes
#include "kernel/core.h"

//===============================================================================================================
// CPREPARE
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CPrepare::CPrepare(Core& coreRef) : TitleCommand(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
QList<const QCommandLineOption*> CPrepare::options() const { return TitleCommand::options(); }
QString CPrepare::name() const { return NAME; }

Qx::Error CPrepare::perform()
{
    // Get ID to prepare
    QUuid id;
    if(Qx::Error ide = getTitleId(id); ide.isValid())
        return ide;

    // Enqueue prepare task
    Fp::GameData titleGameData;
    if(Fp::DbError gdErr = mCore.fpInstall().database()->getGameData(titleGameData, id); gdErr.isValid())
    {
        postDirective<DError>(gdErr);
        return gdErr;
    }

    if(!titleGameData.isNull())
    {
        if(Qx::Error ee = mCore.enqueueDataPackTasks(titleGameData); ee.isValid())
            return ee;

        postDirective<DStatusUpdate>(STATUS_PREPARE, id.toString(QUuid::WithoutBraces));
    }
    else
        logError(Qx::GenericError(Qx::Warning, 12141, LOG_WRN_PREP_NOT_DATA_PACK.arg(id.toString(QUuid::WithoutBraces))));

    // Return success
    return Qx::Error();
}

//Public:
bool CPrepare::requiresServices() const { return true; }
