#ifndef UTILITY_H
#define UTILITY_H

// Qt Includes
#include <QIcon>

// Magic Enum Includes
#include "magic_enum.hpp"

//-Macros----------------------------------------
#define ENUM_NAME(eenum) QString(magic_enum::enum_name(eenum).data())
#define CLIFP_DIR_PATH QCoreApplication::applicationDirPath()
#define CLIFP_CUR_APP_FILENAME QFileInfo(QCoreApplication::applicationFilePath()).fileName()

namespace Utility
{
//-Functions------------------------------------
const QIcon& appIconFromResources();
bool installAppIconForUser();
}
#endif // UTILITY_H
