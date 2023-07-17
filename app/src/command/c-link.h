#ifndef CLINK_H
#define CLINK_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/title-command.h"

class QX_ERROR_TYPE(CLinkError, "CLinkError", 1212)
{
    friend class CLink;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        InvalidPath = 1,
        IconInstallFailed = 2
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {InvalidPath, QSL("The provided shortcut path is not valid or a location where you do not have permissions to create a shortcut.")},
        {IconInstallFailed, QSL("Failed to install icons required for the shortcut.")}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    CLinkError(Type t = NoError, const QString& s = {});

    //-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class CLink : public TitleCommand
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_LINK = QSL("Linking");

    // General
    static inline const QString DIAG_CAPTION = QSL("Select a shortcut destination...");

    // Logging - Messages
    static inline const QString LOG_EVENT_FILE_PATH = QSL("Shortcut path provided is for a file");
    static inline const QString LOG_EVENT_DIR_PATH = QSL("Shortcut path provided is for a folder");
    static inline const QString LOG_EVENT_NO_PATH = QSL("No shortcut path provided, user will be prompted");
    static inline const QString LOG_EVENT_SEL_PATH = QSL("Shortcut path selected: %1");
    static inline const QString LOG_EVENT_DIAG_CANCEL = QSL("Shortcut path selection canceled.");
    static inline const QString LOG_EVENT_CREATED_DIR_PATH = QSL("Created directories for shortcut: %1");
    static inline const QString LOG_EVENT_CREATED_SHORTCUT = QSL("Created shortcut to %1 at %2");

    // Command line option strings
    static inline const QString CL_OPT_PATH_S_NAME = QSL("p");
    static inline const QString CL_OPT_PATH_L_NAME = QSL("path");
    static inline const QString CL_OPT_PATH_DESC = QSL("Path to new shortcut. Path's ending with "".lnk""//"".desktop"" will be interpreted as a named shortcut file. "
                                                       "Any other path will be interpreted as a directory and the title will automatically be used "
                                                       "as the filename");

    // Command line options
    static inline const QCommandLineOption CL_OPTION_PATH{{CL_OPT_PATH_S_NAME, CL_OPT_PATH_L_NAME}, CL_OPT_PATH_DESC, "path"}; // Takes value

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_PATH};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_REQUIRED{};

public:
    // Meta
    static inline const QString NAME = QSL("link");
    static inline const QString DESCRIPTION = QSL("Creates a shortcut to a Flashpoint title.");

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CLink(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    Qx::Error createShortcut(const QString& name, const QDir& dir, QUuid id);
    QString shortcutExtension() const;

protected:
    QList<const QCommandLineOption*> options() override;
    QSet<const QCommandLineOption*> requiredOptions() override;
    QString name() override;
    Qx::Error perform() override ;
};
REGISTER_COMMAND(CLink::NAME, CLink, CLink::DESCRIPTION);

#endif // CLINK_H
