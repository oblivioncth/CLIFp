// Unit Include
#include "c-link.h"

// Qx Includes

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


    // Return success
    return ErrorCode::NO_ERR;
}

QString CLink::shortcutExtension() const { return "desktop"; };
