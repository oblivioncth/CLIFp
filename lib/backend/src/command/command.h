#ifndef COMMAND_H
#define COMMAND_H

// Qt Includes
#include <QCommandLineParser>

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "kernel/directorate.h"

class CommandFactory;
class Core;

class QX_ERROR_TYPE(CommandError, "CommandError", 1210)
{
    friend class Command;

//-Class Enums--------------------------------------------------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        InvalidArguments,
        InvalidCommand,
        MissingRequiredOption
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
    CommandError& wDetails(const QString& det);

    template<typename... Args>
    CommandError arged(Args... args) const;

public:
    bool isValid() const;
    Type type() const;
    QString errorString() const;
};

class Command : public Directorate
{   
//-Class Structs------------------------------------------------------------------------------------------------------
private:
    struct Entry
    {
        using Producer = std::unique_ptr<Command> (*)(Core& core);
        using Description = const QString*;

        Producer producer;
        Description desc;
    };

//-Class Aliases------------------------------------------------------------------------------------------------------
private:
    using Registry = QHash<QString, Entry>;

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

    // Logging - Primary Labels
    static inline const QString COMMAND_LABEL = u"Command: %1"_s;
    static inline const QString COMMAND_OPT_LABEL = u"Command Options: %1"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_C_HELP_SHOWN = u"Displayed help information for: %1"_s;

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

    // Registry
    static inline Registry smRegistry;

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
    template<class C>
        requires std::derived_from<C, Command>
    static void registerCommand();

    template <typename... Cs>
    static void registerCommands();

public:
    static void registerAllCommands();
    static CommandError isRegistered(const QString& name);
    static QList<QString> registered();
    static std::unique_ptr<Command> acquire(const QString& name, Core& core);
    static bool hasDescription(const QString& name);
    static QString describe(const QString& name);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    void logCommand(const QString& commandName);
    void logCommandOptions(const QString& commandOptions);
    CommandError parse(const QStringList& commandLine);
    bool checkStandardOptions();
    CommandError checkRequiredOptions();
    void showHelp();

protected:
    // Command specific
    virtual QList<const QCommandLineOption*> options() const = 0;
    virtual QSet<const QCommandLineOption*> requiredOptions() const;
    virtual QString name() const = 0;
    virtual Qx::Error perform() = 0;

public:
    virtual bool requiresFlashpoint() const;
    virtual bool requiresServices() const;
    virtual bool autoBlockNewInstances() const;
    Qx::Error process(const QStringList& commandLine);
};

#endif // COMMAND_H
