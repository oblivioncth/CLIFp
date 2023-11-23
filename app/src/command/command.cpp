// Unit Includes
#include "command.h"

// Qt Includes
#include <QApplication>

// Qx Includes
#include <qx/utility/qx-helpers.h>

//===============================================================================================================
// CommandError
//===============================================================================================================

//-Constructor-----------------------------------------------------------------------------------------------------
//Public:
CommandError::CommandError() :
    mType(NoError)
{}

//Private:
CommandError::CommandError(Type type, const QString& errStr) :
    mType(type),
    mString(errStr)
{}

//-Instance Functions----------------------------------------------------------------------------------------------
//Private:
quint32 CommandError::deriveValue() const { return static_cast<quint32>(mType); }
QString CommandError::derivePrimary() const { return PRIMARY_STRING; }
QString CommandError::deriveSecondary() const { return mString; }
QString CommandError::deriveDetails() const { return mDetails; }
Qx::Severity CommandError::deriveSeverity() const { return Qx::Critical; }
CommandError& CommandError::wDetails(const QString& det) { mDetails = det; return *this; }

//Public:
bool CommandError::isValid() const { return mType != NoError; }
CommandError::Type CommandError::type() const { return mType; }
QString CommandError::errorString() const { return mString; }

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

CommandError Command::isRegistered(const QString &name)
{
    return registry().contains(name) ? CommandError() : ERR_INVALID_COMMAND.arged(name);
}

QList<QString> Command::registered() { return registry().keys(); }
bool Command::hasDescription(const QString& name) { return !registry().value(name).description.isEmpty(); }
QString Command::describe(const QString& name) { return registry().value(name).description; }
std::unique_ptr<Command> Command::acquire(const QString& name, Core& coreRef) { return registry().value(name).factory->produce(coreRef); }

//-Instance Functions------------------------------------------------------------------------------------------------------
//Private:
CommandError Command::parse(const QStringList& commandLine)
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
            // Add switch to interpreted string
            if(!optionsStr.isEmpty())
                optionsStr += ' '; // Space after every switch except first one

            optionsStr += u"--"_s + (*clOption).names().constLast(); // Always use long name

            // Add value of switch if it takes one
            if(!(*clOption).valueName().isEmpty())
                optionsStr += uR"(=")"_s + mParser.value(*clOption) + uR"(")"_s;
        }
    }
    if(optionsStr.isEmpty())
        optionsStr = Core::LOG_NO_PARAMS;

    // Log command
    mCore.logCommand(NAME, commandLine.first());
    mCore.logCommandOptions(NAME, optionsStr);

    if(validArgs)
        return CommandError();
    else
    {
        CommandError parseErr = CommandError(ERR_INVALID_ARGS).wDetails(mParser.errorText());
        postError(parseErr);
        return parseErr;
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

CommandError Command::checkRequiredOptions()
{
    QStringList missing;
    for(auto opt : qxAsConst(requiredOptions()))
        if(!mParser.isSet(*opt))
            missing.append(opt->names().constFirst());

    if(!missing.isEmpty())
        return ERR_MISSING_REQ_OPT.arged(name()).wDetails(u"'"_s + missing.join(u"','"_s) + u"'"_s);

    return CommandError();
}

void Command::showHelp()
{
    logEvent(LOG_EVENT_C_HELP_SHOWN.arg(name()));
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
            QString desc = clOption->description();
            if(desc.isEmpty())
                continue; // Don't show "hidden" options

            // Handle names
            QStringList dashedNames;
            for(const QString& name :  qxAsConst(clOption->names()))
                dashedNames << ((name.length() > 1 ? u"--"_s : u"-"_s) + name);

            // Add option
            QString marker = requiredOptions().contains(clOption) ? u"*"_s : u""_s;
            optStr += HELP_OPT_TEMPL.arg(marker, dashedNames.join(u" | "_s), desc);
        }

        // Complete string
        helpStr = HELP_TEMPL.arg(name(), optStr);
    }

    // Show help
    mCore.postMessage(Message{.text = helpStr});
}

//Protected:
QList<const QCommandLineOption*> Command::options() const { return CL_OPTIONS_STANDARD; }
QSet<const QCommandLineOption*> Command::requiredOptions() const { return {}; }

// Notifications/Logging (core-forwarders)
void Command::logCommand(QString commandName) const {mCore.logCommand(name(), commandName); }
void Command::logCommandOptions(QString commandOptions) const {mCore.logCommandOptions(name(), commandOptions); }
void Command::logError(Qx::Error error) const {mCore.logError(name(), error); }
void Command::logEvent(QString event) const {mCore.logEvent(name(), event); }
void Command::logTask(const Task* task) const {mCore.logTask(name(), task); }
ErrorCode Command::logFinish(Qx::Error errorState) const {return mCore.logFinish(name(), errorState); }
void Command::postError(Qx::Error error, bool log) const {mCore.postError(name(), error, log); }
int Command::postBlockingError(Qx::Error error, bool log, QMessageBox::StandardButtons bs, QMessageBox::StandardButton def) const {return mCore.postBlockingError(name(), error, log); }

//Public:
bool Command::requiresFlashpoint() const { return true; }
bool Command::requiresServices() const { return false; }
bool Command::autoBlockNewInstances() const { return true; }

Qx::Error Command::process(const QStringList& commandLine)
{
    // Parse and check for valid arguments
    CommandError processError = parse(commandLine);
    if(processError.isValid())
        return processError;

    // Handle standard options
    if(checkStandardOptions())
        return Qx::Error();

    // Check for required options
    processError = checkRequiredOptions();
    if(processError.isValid())
    {
        postError(processError);
        return processError;
    }

    // Perform command
    return perform();
}


