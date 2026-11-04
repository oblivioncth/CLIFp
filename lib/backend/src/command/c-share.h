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
        UnregistrationFailed = 2,
        AlreadyRegistered = 3,
        NotRegistered = 4,

    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {RegistrationFailed, u"Failed to register CLIFp as the 'flashpoint' scheme handler."_s},
        {UnregistrationFailed, u"Failed to remove CLIFp as the 'flashpoint' scheme handler."_s},
        {AlreadyRegistered, u"CLIFp is already set as the `flashpoint` scheme handler"_s},
        {NotRegistered, u"CLIFp is not set as the `flashpoint` scheme handler"_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;
    Qx::Severity mSeverity;

//-Constructor-------------------------------------------------------------
private:
    CShareError(Type t = NoError, const QString& s = {}, Qx::Severity sv = Qx::Critical);

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
    static inline const QString MSG_ALREADY_REGISTERED = u"This copy of CLIFp is already registered as the protocol handler."_s;
    static inline const QString MSG_NOT_REGISTERED = u"This copy of CLIFp is not registered as the protocol handler."_s;
    static inline const QString MSG_GENERATED_URL = u"Share URL placed in clipboard:\n\n%1"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_REGISTRATION = u"Registering CLIFp to handle flashpoint protocol links..."_s;
    static inline const QString LOG_EVENT_UNREGISTRATION = u"Removing CLIFp as the flashpoint protocol link handler..."_s;
    static inline const QString LOG_EVENT_URL = u"Share URL generated: %1"_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_CONFIGURE{
        {u"c"_s, u"configure"_s},
        u"Registers CLIFp at its current location to handle 'flashpoint://' links. Warns if it is already."_s
    }; // Boolean

    static inline const QCommandLineOption CL_OPTION_CONFIGURED{
        {u"C"_s, u"configured"_s},
        u"Ensures CLIFp at its current location is registered to handle 'flashpoint://' links."_s
    }; // Boolean

    static inline const QCommandLineOption CL_OPTION_UNCONFIGURE{
        // TODO: If we every make configuring the share handler its own command (e.g. handle, share-handle, etc), make sure this option has a short name
        {u"unconfigure"_s}, // No short name... sucks, but I can't think of a great letter for it given this competes with title-command options
        u"Unregisters CLIFp as the 'flashpoint://' link handler if registered. Warns if it isn't."_s
    }; // Boolean

    static inline const QCommandLineOption CL_OPTION_UNCONFIGURED{
        // TODO: If we every make configuring the share handler its own command (e.g. handle, share-handle, etc), make sure this option has a short name
        {u"unconfigured"_s}, // No short name... sucks, but I can't think of a great letter for it given this competes with title-command options
        u"Ensures CLIFp is not set as the 'flashpoint://' link handler"_s
    }; // Boolean

    static inline const QCommandLineOption CL_OPTION_UNIVERSAL{
        {u"u"_s, u"universal"_s},
        u"Creates a share URL that utilizes an https redirect for increased portability."_s
    }; // Boolean

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{
        &CL_OPTION_CONFIGURE, &CL_OPTION_CONFIGURED, &CL_OPTION_UNCONFIGURE, &CL_OPTION_UNCONFIGURED, &CL_OPTION_UNIVERSAL
    };

public:
    // Meta
    static inline const QString NAME = u"share"_s;
    static inline const QString DESCRIPTION = u"Generates a URL for starting a Flashpoint title that can be shared to other users."_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CShare(Core& coreRef, const QStringList& commandLine);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    QList<const QCommandLineOption*> options() const override;
    QString name() const override;

public:
    Qx::Error perform() override;
};

#endif // CSHARE_H
