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
Qx::Error CLink::createShortcut(const QString& name, const QDir& dir, QUuid id)
{
    QString filename = Qx::kosherizeFileName(name);

    // Create shortcut properties
    Qx::ShortcutProperties sp;
    sp.target = CLIFP_PATH;
    sp.targetArgs = CPlay::NAME + u" -"_s + TitleCommand::CL_OPTION_ID.names().constFirst() + ' ' + id.toString(QUuid::WithoutBraces);
    sp.comment = name;

    // Create shortcut
    QString fullShortcutPath = dir.absolutePath() + '/' + filename + u".lnk"_s;
    Qx::SystemError shortcutError = Qx::createShortcut(fullShortcutPath, sp);

    // Check for creation failure
    if(shortcutError.isValid())
    {
        postError(shortcutError);
        return shortcutError;
    }
    else
        logEvent(LOG_EVENT_CREATED_SHORTCUT.arg(id.toString(QUuid::WithoutBraces), QDir::toNativeSeparators(fullShortcutPath)));

    // Return success
    return CLinkError();
}
