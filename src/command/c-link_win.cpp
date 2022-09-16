// Unit Include
#include "c-link.h"

// Qx Includes
#include <qx/windows/qx-common-windows.h>

// Project Includes
#include "c-play.h"
#include "utility.h"

//===============================================================================================================
// CSHORTCUT
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Private:
ErrorCode CLink::createShortcut(const QString& name, const QDir& dir, QUuid id)
{
    // Create shortcut properties
    Qx::ShortcutProperties sp;
    sp.target = CLIFP_DIR_PATH + "/" + PROJECT_EXECUTABLE_BASE_NAME + ".exe";
    sp.targetArgs = CPlay::NAME + " -" + CPlay::CL_OPT_ID_S_NAME + " " + id.toString(QUuid::WithoutBraces);
    sp.comment = name;

    // Create shortcut
    QString fullShortcutPath = dir.absolutePath() + '/' + name + '.' + shortcutExtension();
    Qx::GenericError shortcutError = Qx::createShortcut(fullShortcutPath, sp);

    // Check for creation failure
    if(shortcutError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CREATE_FAILED, shortcutError.primaryInfo()));
        return ErrorCode::INVALID_SHORTCUT_PARAM;
    }
    else
        mCore.logEvent(NAME, LOG_EVENT_CREATED_SHORTCUT.arg(id.toString(QUuid::WithoutBraces), QDir::toNativeSeparators(fullShortcutPath)));

    // Return success
    return ErrorCode::NO_ERR;
}

QString CLink::shortcutExtension() const { return "lnk"; };
