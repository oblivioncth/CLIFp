#ifndef COMMAND_H
#define COMMAND_H

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
protected:
    // Help template
    static inline const QString HELP_TEMPL = "<u>Usage:</u><br>"
                                             "%1 &lt;options&gt;<br>"
                                             "<br>"
                                             "<u>Options:</u>%2";
    static inline const QString HELP_OPT_TEMPL = "<br><b>%1:</b> &nbsp;%2";

    // Logging - Messages
    static inline const QString LOG_EVENT_C_HELP_SHOWN = "Displayed help infomration for: %1";

    // Standard command line option strings
    static inline const QString CL_OPT_HELP_S_NAME = "h";
    static inline const QString CL_OPT_HELP_L_NAME = "help";
    static inline const QString CL_OPT_HELP_E_NAME = "?";
    static inline const QString CL_OPT_HELP_DESC = "Prints this help message.";

    // Standard command line options
    static inline const QCommandLineOption CL_OPTION_HELP{{CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC}; // Boolean option
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_STANDARD{&CL_OPTION_HELP};

    // Meta
    static inline const QString NAME = "command";

//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    QString mHelpString;

protected:
    Core& mCore;
    QCommandLineParser mParser;

//-Constructor----------------------------------------------------------------------------------------------------------
protected:
    Command(Core& coreRef);

//-Class Functions----------------------------------------------------------------------------------------------------------
private:
    static QMap<QString, Entry>& registry();

public:
    static void registerCommand(const QString& name, CommandFactory* factory, const QString& desc);
    static bool isRegistered(const QString& name);
    static QList<QString> registered();
    static std::unique_ptr<Command> acquire(const QString& name, Core& coreRef);
    static QString describe(const QString& name);

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    virtual const QList<const QCommandLineOption*> options() = 0;
    virtual const QString name() = 0;
    ErrorCode parse(const QStringList& commandLine);
    bool checkStandardOptions();
    void showHelp();

public:
    virtual ErrorCode process(const QStringList& commandLine) = 0;
};

class CommandFactory
{
//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual std::unique_ptr<Command> produce(Core& coreRef) = 0;
};

#endif // COMMAND_H
