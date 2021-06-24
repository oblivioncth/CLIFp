#ifndef CSHOW_H
#define CSHOW_H

#include "command.h"

class CShow : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Error Messages - Prep
    static inline const QString ERR_NO_SHOW = "No message or extra to show was provided.";

    // Logging - Messages

    // Logging - Errors

    // Command line option strings
    static inline const QString CL_OPT_MSG_S_NAME = "m";
    static inline const QString CL_OPT_MSG_L_NAME = "message";
    static inline const QString CL_OPT_MSG_DESC = "Displays an pop-up dialog with the supplied message. Used primarily for some additional apps.";

    static inline const QString CL_OPT_EXTRA_S_NAME = "e";
    static inline const QString CL_OPT_EXTRA_L_NAME = "extra";
    static inline const QString CL_OPT_EXTRA_DESC = "Opens an explorer window to the specified extra. Used primarily for some additional apps.";

    // Command line options
    static inline const QCommandLineOption CL_OPTION_MSG{{CL_OPT_MSG_S_NAME, CL_OPT_MSG_L_NAME}, CL_OPT_MSG_DESC, "message"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_EXTRA{{CL_OPT_EXTRA_S_NAME, CL_OPT_EXTRA_L_NAME}, CL_OPT_EXTRA_DESC, "extra"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_MSG, &CL_OPTION_EXTRA};

public:
    // Meta
    static inline const QString NAME = "show";
    static inline const QString DESCRIPTION = "Display a message or extra folder";

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CShow(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    const QList<const QCommandLineOption*> options();
    const QString name();

public:
    ErrorCode process(const QStringList& commandLine);
};
REGISTER_COMMAND(CShow::NAME, CShow, CShow::DESCRIPTION);

#endif // CSHOW_H
