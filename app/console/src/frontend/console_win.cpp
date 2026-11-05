// Unit Include
#include "frontend/console.h"

// Windows Includes
/* NOTE: This just mimics: #include <qx_windows.h> but I don't want to link to Qx::Windows just for that one header;
 * however if we ever need that lib for other reasons then replace this with said include
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOATOM
#define NOATOM
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOSOUND
#define NOSOUND
#endif
#ifndef NOHELP
#define NOHELP
#endif
#ifndef NOMCX
#define NOMCX
#endif

#include "windows.h"

//===============================================================================================================
// FrontendConsole
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Private:
bool FrontendConsole::checkInteractivity()
{
    DWORD mode;
    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) != 0;
};
