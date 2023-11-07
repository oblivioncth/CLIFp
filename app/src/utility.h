#ifndef UTILITY_H
#define UTILITY_H

// Qt Includes
#include <QIcon>

// Magic Enum Includes
#include "magic_enum.hpp"

//-Macros----------------------------------------
#define ENUM_NAME(eenum) QString(magic_enum::enum_name(eenum).data())
#define CLIFP_DIR_PATH QCoreApplication::applicationDirPath()
#define CLIFP_PATH QCoreApplication::applicationFilePath()
#define CLIFP_CUR_APP_FILENAME QFileInfo(QCoreApplication::applicationFilePath()).fileName()
#define CLIFP_CUR_APP_BASENAME QFileInfo(QCoreApplication::applicationFilePath()).baseName()
#ifdef _WIN32
    #define CLIFP_CANONICAL_APP_FILNAME u"CLIFp.exe"_s
#else
    #define CLIFP_CANONICAL_APP_FILNAME u"clifp"_s
#endif

namespace Utility
{
//-Functions------------------------------------
const QIcon& appIconFromResources();
bool installAppIconForUser();
}
#endif // UTILITY_H
