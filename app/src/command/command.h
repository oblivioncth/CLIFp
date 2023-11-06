#ifndef COMMAND_H
#define COMMAND_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes 
#include "kernel/core.h"

class CommandFactory;

//-Macros-------------------------------------------------------------------------------------------------------------------
#define REGISTER_COMMAND(name, command, desc) \
    class command##Factory : public CommandFactory \
    { \
    public: \
        command##Factory() { Command::registerCommand(name, this, desc); } \
        virtual std::unique_ptr<Command> produce(Core& coreRef) { return std::make_unique<command>(coreRef); } \
    }; \
    static command##Factory _##command##Factory;

class QX_ERROR_TYPE(CommandError, "CommandError", 1210)
{
    friend class Command;

//-Class Enums--------------------------------------------------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        InvalidArguments = 1,
        InvalidCommand = 2,
        MissingRequiredOption = 3
    };

//-Class Variables------------------------------------------------------------------------------------------------
private:
    static inline const QString PRIMARY_STRING = u"Command parsing error."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    Type mType;
    QString mString;
    QString mDetails;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CommandError();

private:
    CommandError(Type type, const QString& errStr);

//-Instance Functions---------------------------------------------------------------------------------------------------
private:
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
    QString deriveDetails() const override;
    Qx::Severity deriveSeverity() const override;

    template<typename... Args>
    CommandError arged(Args... args) const
    {
        CommandError a = *this;
        a.mString = a.mString.arg(args...);
        return a;
    }

    CommandError& wDetails(const QString& det);

public:
    bool isValid() const;
    Type type() const;
    QString errorString() const;
};

class Command
{
//-Class Structs------------------------------------------------------------------------------------------------------
protected:
    struct Entry
    {
        CommandFactory* factory;
        QString description;
    };

//-Class Variables--------------------------------------------------------------------------------------------------------
private:
    // Error
    static inline const CommandError ERR_INVALID_ARGS =
        CommandError(CommandError::InvalidArguments, u"Invalid command arguments."_s);
    static inline const CommandError ERR_INVALID_COMMAND =
        CommandError(CommandError::InvalidCommand, u"'%1' is not a valid command"_s);
    static inline const CommandError ERR_MISSING_REQ_OPT =
        CommandError(CommandError::MissingRequiredOption, u"Missing required options for '%1'"_s);

    // Help template
    static inline const QString HELP_TEMPL = u"<u>Usage:</u><br>"
                                              "%1 &lt;options&gt;<br>"
                                              "<br>"
                                              "<u>Options:</u>%2<br>"
                                              "<br>"
                                              "*Required Option<br>"_s;
    static inline const QString HELP_OPT_TEMPL = u"<br><b>%1%2:</b> &nbsp;%3"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_C_HELP_SHOWN = u"Displayed help infomration for: %1"_s;

    // Standard command line option strings
    static inline const QString CL_OPT_HELP_S_NAME = u"h"_s;
    static inline const QString CL_OPT_HELP_E_NAME = u"?"_s;
    static inline const QString CL_OPT_HELP_L_NAME = u"help"_s;
    static inline const QString CL_OPT_HELP_DESC = u"Prints this help message."_s;

    // Standard command line options
    static inline const QCommandLineOption CL_OPTION_HELP{{CL_OPT_HELP_S_NAME, CL_OPT_HELP_E_NAME, CL_OPT_HELP_L_NAME}, CL_OPT_HELP_DESC}; // Boolean option
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_STANDARD{&CL_OPTION_HELP};

    // Meta
    static inline const QString NAME = u"command"_s;

//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    QString mHelpString;

protected:
    Core& mCore;
    QCommandLineParser mParser;

//-Constructor----------------------------------------------------------------------------------------------------------
protected:
    Command(Core& coreRef);

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    virtual ~Command() = default;

//-Class Functions----------------------------------------------------------------------------------------------------------
private:
    static QMap<QString, Entry>& registry();

public:
    static void registerCommand(const QString& name, CommandFactory* factory, const QString& desc);
    static CommandError isRegistered(const QString& name);
    static QList<QString> registered();
    static std::unique_ptr<Command> acquire(const QString& name, Core& coreRef);
    static bool hasDescription(const QString& name);
    static QString describe(const QString& name);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    CommandError parse(const QStringList& commandLine);
    bool checkStandardOptions();
    CommandError checkRequiredOptions();
    void showHelp();

protected:
    // Command specific
    virtual QList<const QCommandLineOption*> options() = 0;
    virtual QSet<const QCommandLineOption*> requiredOptions();
    virtual QString name() = 0;
    virtual Qx::Error perform() = 0;

public:
    virtual bool requiresFlashpoint() const;
    virtual bool autoBlockNewInstances() const;
    Qx::Error process(const QStringList& commandLine);
};

class CommandFactory
{
//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual std::unique_ptr<Command> produce(Core& coreRef) = 0;
};

#endif // COMMAND_H
