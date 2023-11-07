#ifndef CLINK_H
#define CLINK_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/title-command.h"

class QX_ERROR_TYPE(CLinkError, "CLinkError", 1213)
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
        {NoError, u""_s},
        {InvalidPath, u"The provided shortcut path is not valid or a location where you do not have permissions to create a shortcut."_s},
        {IconInstallFailed, u"Failed to install icons required for the shortcut."_s}
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
    static inline const QString STATUS_LINK = u"Linking"_s;

    // General
    static inline const QString DIAG_CAPTION = u"Select a shortcut destination..."_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_NO_PATH = u"No shortcut path provided, user will be prompted"_s;
    static inline const QString LOG_EVENT_SEL_PATH = u"Shortcut path selected: %1"_s;
    static inline const QString LOG_EVENT_DIAG_CANCEL = u"Shortcut path selection canceled."_s;
    static inline const QString LOG_EVENT_CREATED_DIR_PATH = u"Created directories for shortcut: %1"_s;
    static inline const QString LOG_EVENT_CREATED_SHORTCUT = u"Created shortcut to %1 at %2"_s;

    // Command line option strings
    static inline const QString CL_OPT_PATH_S_NAME = u"p"_s;
    static inline const QString CL_OPT_PATH_L_NAME = u"path"_s;
    static inline const QString CL_OPT_PATH_DESC = u"Path to a directory for the new shortcut"_s;

    static inline const QString CL_OPT_NAME_S_NAME = u"n"_s;
    static inline const QString CL_OPT_NAME_L_NAME = u"name"_s;
    static inline const QString CL_OPT_NAME_DESC = u"Name of the shortcut. Defaults to the name of the title"_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_PATH{{CL_OPT_PATH_S_NAME, CL_OPT_PATH_L_NAME}, CL_OPT_PATH_DESC, u"path"_s}; // Takes value
    static inline const QCommandLineOption CL_OPTION_NAME{{CL_OPT_NAME_S_NAME, CL_OPT_NAME_L_NAME}, CL_OPT_NAME_DESC, u"name"_s}; // Takes value

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_PATH, &CL_OPTION_NAME};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_REQUIRED{};

public:
    // Meta
    static inline const QString NAME = u"link"_s;
    static inline const QString DESCRIPTION = u"Creates a shortcut to a Flashpoint title."_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CLink(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    Qx::Error createShortcut(const QString& name, const QDir& dir, QUuid id);

protected:
    QList<const QCommandLineOption*> options() override;
    QSet<const QCommandLineOption*> requiredOptions() override;
    QString name() override;
    Qx::Error perform() override ;
};
REGISTER_COMMAND(CLink::NAME, CLink, CLink::DESCRIPTION);

#endif // CLINK_H
