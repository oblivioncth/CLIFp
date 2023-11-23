#ifndef CSHARE_H
#define CSHARE_H

// Project Includes
#include "command/title-command.h"

class QX_ERROR_TYPE(CShareError, "CShareError", 1216)
{
    friend class CShare;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        RegistrationFailed = 1,
        UnregistrationFailed = 2
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {RegistrationFailed, u"Failed to register CLIFp as the 'flashpoint' scheme handler."_s},
        {UnregistrationFailed, u"Failed to remove CLIFp as the 'flashpoint' scheme handler."_s}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    CShareError(Type t = NoError, const QString& s = {});

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

class CShare : public TitleCommand
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_SHARE = u"Sharing"_s;

    // General
    static inline const QString SCHEME_TEMPLATE_STD = u"flashpoint://%1"_s;
    static inline const QString SCHEME_TEMPLATE_UNI = u"https://oblivioncth.github.io/CLIFp/redirect.html?uuid=%1"_s;
    static inline const QString SCHEME = u"flashpoint"_s;
    static inline const QString SCHEME_NAME = u"Flashpoint (via CLIFp)"_s;

    // Messages
    static inline const QString MSG_REGISTRATION_COMPLETE = u"Successfully registered CLIFp to respond to 'flashpoint://' requests."_s;
    static inline const QString MSG_UNREGISTRATION_COMPLETE = u"Successfully removed CLIFp as the 'flashpoint://' request handler."_s;
    static inline const QString MSG_GENERATED_URL = u"Share URL placed in clipboard:\n\n%1"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_REGISTRATION = u"Registering CLIFp to handle flashpoint protocol links..."_s;
    static inline const QString LOG_EVENT_UNREGISTRATION = u"Removing CLIFp as the flashpoint protocol link handler..."_s;
    static inline const QString LOG_EVENT_URL = u"Share URL generated: %1"_s;

    // Command line option strings
    static inline const QString CL_OPT_CONFIGURE_S_NAME = u"c"_s;
    static inline const QString CL_OPT_CONFIGURE_L_NAME = u"configure"_s;
    static inline const QString CL_OPT_CONFIGURE_DESC = u"Registers CLIFp at its current location to handle 'flashpoint://' links."_s;

    static inline const QString CL_OPT_UNCONFIGURE_S_NAME = u"C"_s;
    static inline const QString CL_OPT_UNCONFIGURE_L_NAME = u"unconfigure"_s;
    static inline const QString CL_OPT_UNCONFIGURE_DESC = u"Unregisters CLIFp as the 'flashpoint://' link handler if registered."_s;

    static inline const QString CL_OPT_UNIVERSAL_S_NAME = u"u"_s;
    static inline const QString CL_OPT_UNIVERSAL_L_NAME = u"universal"_s;
    static inline const QString CL_OPT_UNIVERSAL_DESC = u"Creates a share URL that utilizes an https redirect for increased portability."_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_CONFIGURE{{CL_OPT_CONFIGURE_S_NAME, CL_OPT_CONFIGURE_L_NAME}, CL_OPT_CONFIGURE_DESC}; // Boolean
    static inline const QCommandLineOption CL_OPTION_UNCONFIGURE{{CL_OPT_UNCONFIGURE_S_NAME, CL_OPT_UNCONFIGURE_L_NAME}, CL_OPT_UNCONFIGURE_DESC}; // Boolean
    static inline const QCommandLineOption CL_OPTION_UNIVERSAL{{CL_OPT_UNIVERSAL_S_NAME, CL_OPT_UNIVERSAL_L_NAME}, CL_OPT_UNIVERSAL_DESC}; // Boolean

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_CONFIGURE, &CL_OPTION_UNCONFIGURE, &CL_OPTION_UNIVERSAL};

public:
    // Meta
    static inline const QString NAME = u"share"_s;
    static inline const QString DESCRIPTION = u"Generates a URL for starting a Flashpoint title that can be shared to other users."_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CShare(Core& coreRef);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    QList<const QCommandLineOption*> options() const override;
    QString name() const override;
    Qx::Error perform() override;
};
REGISTER_COMMAND(CShare::NAME, CShare, CShare::DESCRIPTION);

#endif // CSHARE_H
