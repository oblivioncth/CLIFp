#ifndef QXWINDOWS_H
#define QXWINDOWS_H
#include "Windows.h"
#include <QString>

namespace Qx
{

//-Functions-------------------------------------------------------------------------------------------------------------
DWORD getProcessIDByName(QString processName);
QString getProcessNameByID(DWORD processID);
bool processIsRunning(QString processName);
bool processIsRunning(DWORD processID);

}

#endif // QXWINDOWS_H
