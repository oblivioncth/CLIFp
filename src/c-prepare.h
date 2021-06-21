#ifndef CPREPARE_H
#define CPREPARE_H

#include "command.h"

class CPrepare : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Error Messages - Prep
    static inline const QString ERR_NO_ID= "No title to prepare was specified.";
    static inline const QString ERR_ID_INVALID = "The provided string was not a valid GUID/UUID.";

    // Logging - Messages

    // Logging - Errors
    static inline const QString LOG_WRN_PREP_NOT_DATA_PACK = "The provided ID does not belong to a Data Pack based game (%1). No action will be taken.";

    // Command line option strings
    static inline const QString CL_OPT_ID_S_NAME = "i";
    static inline const QString CL_OPT_ID_L_NAME = "id";
    static inline const QString CL_OPT_ID_DESC = "UUID of title to prepare";

    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{{CL_OPT_ID_S_NAME, CL_OPT_ID_L_NAME}, CL_OPT_ID_DESC, "id"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID};

public:
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
    Core::ErrorCode process(const QStringList& commandLine);
};

#endif // CPREPARE_H
