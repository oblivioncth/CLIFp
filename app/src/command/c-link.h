#ifndef CLINK_H
#define CLINK_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class CLink : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_LINK = QSL("Linking");

    // General
    static inline const QString DIAG_CAPTION = QSL("Select a shortcut destination...");

    // Error Messages - Prep
    static inline const QString ERR_NO_TITLE = QSL("The title to link was not specified.");
    static inline const QString ERR_CREATE_FAILED = QSL("Failed to create shortcut.");
    static inline const QString ERR_INVALID_PATH = QSL("The provided shortcut path is not valid or a location where you do not have permissions to create a shortcut.");
    static inline const QString ERR_DIFFERENT_TITLE_SRC = QSL("The shortcut title source was expected to be %1 but instead was %2");

    // Logging - Messages
    static inline const QString LOG_EVENT_FILE_PATH = QSL("Shortcut path provided is for a file");
    static inline const QString LOG_EVENT_DIR_PATH = QSL("Shortcut path provided is for a folder");
    static inline const QString LOG_EVENT_NO_PATH = QSL("No shortcut path provided, user will be prompted");
    static inline const QString LOG_EVENT_SEL_PATH = QSL("Shortcut path selected: %1");
    static inline const QString LOG_EVENT_DIAG_CANCEL = QSL("Shortcut path selection canceled.");
    static inline const QString LOG_EVENT_CREATED_DIR_PATH = QSL("Created directories for shortcut: %1");
    static inline const QString LOG_EVENT_CREATED_SHORTCUT = QSL("Created shortcut to %1 at %2");

    // Logging - Errors

    // Command line option strings
    static inline const QString CL_OPT_ID_S_NAME = QSL("i");
    static inline const QString CL_OPT_ID_L_NAME = QSL("id");
    static inline const QString CL_OPT_ID_DESC = QSL("UUID of title to make a shortcut for");

    static inline const QString CL_OPT_TITLE_S_NAME = QSL("t");
    static inline const QString CL_OPT_TITLE_L_NAME = QSL("title");
    static inline const QString CL_OPT_TITLE_DESC = QSL("Title to make a shortcut for");

    static inline const QString CL_OPT_TITLE_STRICT_S_NAME = QSL("T");
    static inline const QString CL_OPT_TITLE_STRICT_L_NAME = QSL("title-strict");
    static inline const QString CL_OPT_TITLE_STRICT_DESC = QSL("Same as -t, but exact matches only");

    static inline const QString CL_OPT_SUBTITLE_S_NAME = QSL("s");
    static inline const QString CL_OPT_SUBTITLE_L_NAME = QSL("subtitle");
    static inline const QString CL_OPT_SUBTITLE_DESC = QSL("Name of additional-app under the title to make a shortcut for. Must be used with -t / -T");

    static inline const QString CL_OPT_SUBTITLE_STRICT_S_NAME = QSL("S");
    static inline const QString CL_OPT_SUBTITLE_STRICT_L_NAME = QSL("subtitle-strict");
    static inline const QString CL_OPT_SUBTITLE_STRICT_DESC = QSL("Same as -s, but exact matches only");

    static inline const QString CL_OPT_PATH_S_NAME = QSL("p");
    static inline const QString CL_OPT_PATH_L_NAME = QSL("path");
    static inline const QString CL_OPT_PATH_DESC = QSL("Path to new shortcut. Path's ending with "".lnk""//"".desktop"" will be interpreted as a named shortcut file. "
                                                       "Any other path will be interpreted as a directory and the title will automatically be used "
                                                       "as the filename");

    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{{CL_OPT_ID_S_NAME, CL_OPT_ID_L_NAME}, CL_OPT_ID_DESC, "id"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE{{CL_OPT_TITLE_S_NAME, CL_OPT_TITLE_L_NAME}, CL_OPT_TITLE_DESC, "title"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE_STRICT{{CL_OPT_TITLE_STRICT_S_NAME, CL_OPT_TITLE_STRICT_L_NAME}, CL_OPT_TITLE_STRICT_DESC, "title-strict"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_SUBTITLE{{CL_OPT_SUBTITLE_S_NAME, CL_OPT_SUBTITLE_L_NAME}, CL_OPT_SUBTITLE_DESC, "subtitle"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_SUBTITLE_STRICT{{CL_OPT_SUBTITLE_STRICT_S_NAME, CL_OPT_SUBTITLE_STRICT_L_NAME}, CL_OPT_SUBTITLE_STRICT_DESC, "subtitle-strict"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_PATH{{CL_OPT_PATH_S_NAME, CL_OPT_PATH_L_NAME}, CL_OPT_PATH_DESC, "path"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID, &CL_OPTION_TITLE, &CL_OPTION_TITLE_STRICT, &CL_OPTION_SUBTITLE,
                                                                             &CL_OPTION_SUBTITLE_STRICT, &CL_OPTION_PATH};

public:
    // Meta
    static inline const QString NAME = QSL("link");
    static inline const QString DESCRIPTION = QSL("Creates a shortcut to a Flashpoint title.");

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CLink(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    ErrorCode createShortcut(const QString& name, const QDir& dir, QUuid id);
    QString shortcutExtension() const;

protected:
    const QList<const QCommandLineOption*> options();
    const QString name();

public:
    ErrorCode process(const QStringList& commandLine);
};
REGISTER_COMMAND(CLink::NAME, CLink, CLink::DESCRIPTION);

#endif // CLINK_H
