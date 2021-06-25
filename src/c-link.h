#ifndef CSHORTCUT_H
#define CSHORTCUT_H

#include "command.h"

class CLink : public Command
{
//-Inner Classes--------------------------------------------------------------------------------------------------------
private:
    class ErrorCodes
    {
    //-Class Variables--------------------------------------------------------------------------------------------------
    public:
        static const ErrorCode INVALID_SHORTCUT_PARAM = 201;
    };

//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // General
    static inline const QString LNK_EXT = "lnk";
    static inline const QString DIAG_CAPTION = "Select a shortcut destination...";

    // Error Messages - Prep
    static inline const QString ERR_NO_TITLE = "No title to create a shortcut for was specified.";
    static inline const QString ERR_CREATE_FAILED = "Failed to create shortcut.";
    static inline const QString ERR_INVALID_PATH = "The provided shortcut path is not valid or a location where you do not have permissions to create a shortcut.";
    static inline const QString ERR_DIFFERENT_TITLE_SRC = "The shortcut title source was expected expected to be %1 but instead was %2";

    // Logging - Messages
    static inline const QString LOG_EVENT_FILE_PATH = "Shortcut path provided is for a file";
    static inline const QString LOG_EVENT_DIR_PATH = "Shortcut path provided is for a folder";
    static inline const QString LOG_EVENT_NO_PATH = "No shortcut path provided, user will be prompted";
    static inline const QString LOG_EVENT_SEL_PATH = "Shortcut path selected: %1";
    static inline const QString LOG_EVENT_DIAG_CANCEL = "Shortcut path selection canceled.";
    static inline const QString LOG_EVENT_CREATED_DIR_PATH = "Created directories for shortcut: %1";
    static inline const QString LOG_EVENT_CREATED_SHORTCUT = "Created shortcut to %1 at %2";

    // Logging - Errors

    // Command line option strings
    static inline const QString CL_OPT_ID_S_NAME = "i";
    static inline const QString CL_OPT_ID_L_NAME = "id";
    static inline const QString CL_OPT_ID_DESC = "UUID of title to make a shortcut for";

    static inline const QString CL_OPT_TITLE_S_NAME = "t";
    static inline const QString CL_OPT_TITLE_L_NAME = "title";
    static inline const QString CL_OPT_TITLE_DESC = "Title to make a shortcut for";

    static inline const QString CL_OPT_PATH_S_NAME = "p";
    static inline const QString CL_OPT_PATH_L_NAME = "path";
    static inline const QString CL_OPT_PATH_DESC = "Path to new shortcut. Path's ending with "".lnk"" will be interpreted as a named shortcut file. "
                                                   "Any other path will be interpreted as a directory and the title will automatically be used "
                                                   "as the filename";

    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{{CL_OPT_ID_S_NAME, CL_OPT_ID_L_NAME}, CL_OPT_ID_DESC, "id"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE{{CL_OPT_TITLE_S_NAME, CL_OPT_TITLE_L_NAME}, CL_OPT_TITLE_DESC, "title"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_PATH{{CL_OPT_PATH_S_NAME, CL_OPT_PATH_L_NAME}, CL_OPT_PATH_DESC, "path"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID, &CL_OPTION_TITLE, &CL_OPTION_PATH};

public:
    // Meta
    static inline const QString NAME = "link";
    static inline const QString DESCRIPTION = "Creates a shortcut to a Flashpoint title.";

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CLink(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    const QList<const QCommandLineOption*> options();
    const QString name();

public:
    ErrorCode process(const QStringList& commandLine);
};
REGISTER_COMMAND(CLink::NAME, CLink, CLink::DESCRIPTION);

#endif // CSHORTCUT_H
