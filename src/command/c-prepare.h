#ifndef CPREPARE_H
#define CPREPARE_H

#include "command.h"

class CPrepare : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_PREPARE = "Preparing";

    // Error Messages - Prep
    static inline const QString ERR_NO_TITLE = "No title to prepare was specified.";

    // Logging - Messages

    // Logging - Errors
    static inline const QString LOG_WRN_PREP_NOT_DATA_PACK = "The provided ID does not belong to a Data Pack based game (%1). No action will be taken.";

    // Command line option strings
    static inline const QString CL_OPT_ID_S_NAME = "i";
    static inline const QString CL_OPT_ID_L_NAME = "id";
    static inline const QString CL_OPT_ID_DESC = "UUID of title to prepare";

    static inline const QString CL_OPT_TITLE_S_NAME = "t";
    static inline const QString CL_OPT_TITLE_L_NAME = "title";
    static inline const QString CL_OPT_TITLE_DESC = "Title to prepare";

    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{{CL_OPT_ID_S_NAME, CL_OPT_ID_L_NAME}, CL_OPT_ID_DESC, "id"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE{{CL_OPT_TITLE_S_NAME, CL_OPT_TITLE_L_NAME}, CL_OPT_TITLE_DESC, "title"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID, &CL_OPTION_TITLE};

public:
    // Meta
    static inline const QString NAME = "prepare";
    static inline const QString DESCRIPTION = "Initializes Flashpoint for playing the provided Data Pack based title by UUID. If the title does not use a Data Pack this command has no effect.";

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
