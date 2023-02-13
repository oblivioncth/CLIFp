#ifndef CPREPARE_H
#define CPREPARE_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class CPrepare : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_PREPARE = QSL("Preparing");

    // Error Messages - Prep
    static inline const QString ERR_NO_TITLE = QSL("No title to prepare was specified.");

    // Logging - Messages

    // Logging - Errors
    static inline const QString LOG_WRN_PREP_NOT_DATA_PACK = QSL("The provided ID does not belong to a Data Pack based game (%1). No action will be taken.");

    // Command line option strings
    static inline const QString CL_OPT_ID_S_NAME = QSL("i");
    static inline const QString CL_OPT_ID_L_NAME = QSL("id");
    static inline const QString CL_OPT_ID_DESC = QSL("UUID of title to prepare");

    static inline const QString CL_OPT_TITLE_S_NAME = QSL("t");
    static inline const QString CL_OPT_TITLE_L_NAME = QSL("title");
    static inline const QString CL_OPT_TITLE_DESC = QSL("Title to prepare");

    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{{CL_OPT_ID_S_NAME, CL_OPT_ID_L_NAME}, CL_OPT_ID_DESC, "id"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE{{CL_OPT_TITLE_S_NAME, CL_OPT_TITLE_L_NAME}, CL_OPT_TITLE_DESC, "title"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID, &CL_OPTION_TITLE};

public:
    // Meta
    static inline const QString NAME = QSL("prepare");
    static inline const QString DESCRIPTION = QSL("Initializes Flashpoint for playing the provided Data Pack based title by UUID. If the title does not use a Data Pack this command has no effect.");

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CPrepare(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    const QList<const QCommandLineOption*> options();
    const QString name();

public:
    ErrorCode process(const QStringList& commandLine);
};
REGISTER_COMMAND(CPrepare::NAME, CPrepare, CPrepare::DESCRIPTION);

#endif // CPREPARE_H
