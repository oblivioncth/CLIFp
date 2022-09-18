#ifndef UTILITY_H
#define UTILITY_H

// Magic Enum Includes
#include "magic_enum.hpp"

//-Macros----------------------------------------
#define ENUM_NAME(eenum) QString(magic_enum::enum_name(eenum).data())
#define CLIFP_DIR_PATH QCoreApplication::applicationDirPath()
#define CLIFP_CUR_APP_FILENAME QFileInfo(QCoreApplication::applicationFilePath()).fileName()

#endif // UTILITY_H
