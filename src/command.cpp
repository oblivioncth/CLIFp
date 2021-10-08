#include "command.h"
#include <QApplication>

//===============================================================================================================
// Command
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
Command::Command(Core& coreRef) : mCore(coreRef) {}

//-Class Functions------------------------------------------------------------------
//Private:
QMap<QString, Command::Entry>& Command::registry() { static QMap<QString, Entry> registry; return registry; }

//Public:
void Command::registerCommand(const QString& name, CommandFactory* factory, const QString& desc) { registry()[name] = {factory, desc}; }
bool Command::isRegistered(const QString &name) { return registry().contains(name); }
QList<QString> Command::registered() { return registry().keys(); }
QString Command::describe(const QString& name) { return registry().value(name).description; }
std::unique_ptr<Command> Command::acquire(const QString& name, Core& coreRef) { return registry().value(name).factory->produce(coreRef); }

//-Instance Functions------------------------------------------------------------------------------------------------------
//Protected:
const QList<const QCommandLineOption*> Command::options() { return CL_OPTIONS_STANDARD; }

ErrorCode Command::parse(const QStringList& commandLine)
{
    // Add command options
    for(const QCommandLineOption* clOption : options())
        mParser.addOption(*clOption);

    // Parse
    bool validArgs = mParser.parse(commandLine);

    // Create options string
    QString optionsStr;
    for(const QCommandLineOption* clOption : options())
    {
        if(mParser.isSet(*clOption))
        {
            // Add switch to interp string
            if(!optionsStr.isEmpty())
                optionsStr += " "; // Space after every switch except first one

            optionsStr += "--" + (*clOption).names().at(1); // Always use long name

            // Add value of switch if it takes one
            if(!(*clOption).valueName().isEmpty())
                optionsStr += R"(=")" + mParser.value(*clOption) + R"(")";
        }
    }
    if(optionsStr.isEmpty())
        optionsStr = Core::LOG_NO_PARAMS;

    // Log command
    mCore.logCommand(NAME, commandLine.first());
    mCore.logCommandOptions(NAME, optionsStr);

    if(validArgs)
        return Core::ErrorCodes::NO_ERR;
    else
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, mParser.errorText()));
        return Core::ErrorCodes::INVALID_ARGS;
    }
}

bool Command::checkStandardOptions()
{
    // Show help if requested
    if(mParser.isSet(CL_OPTION_HELP))
    {
        showHelp();
        return true;
    }

    // Default
    return false;
}

void Command::showHelp()
{
    mCore.logEvent(name(), LOG_EVENT_C_HELP_SHOWN.arg(name()));
    // Help string
    static QString helpStr;

    // Update status
    mCore.setStatus(Core::STATUS_DISPLAY, Core::STATUS_DISPLAY_HELP);

    // One time setup
    if(helpStr.isNull())
    {        
        // Help options
        QString optStr;
        for(const QCommandLineOption* clOption : options())
        {
            // Handle names
            QStringList dashedNames;
            for(const QString& name :  qAsConstR(clOption->names()))
                dashedNames << ((name.length() > 1 ? "--" : "-") + name);

            // Add option
            optStr += HELP_OPT_TEMPL.arg(dashedNames.join(" | "), clOption->description());
        }

        // Complete string
        helpStr = HELP_TEMPL.arg(name(), optStr);
    }

    // Show help
    mCore.postMessage(helpStr);
}
