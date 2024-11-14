// Unit Includes
#include "command.h"

// Qx Includes
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "kernel/core.h"
#include "command/c-download.h"
#include "command/c-link.h"
#include "command/c-play.h"
#include "command/c-prepare.h"
#include "command/c-run.h"
#include "command/c-share.h"
#include "command/c-show.h"
#include "command/c-update.h"

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

template<typename... Args>
CommandError CommandError::arged(Args... args) const
{
    CommandError a = *this;
    a.mString = a.mString.arg(args...);
    return a;
}

//Public:
bool CommandError::isValid() const { return mType != NoError; }
CommandError::Type CommandError::type() const { return mType; }
QString CommandError::errorString() const { return mString; }

//===============================================================================================================
// Command
//===============================================================================================================

//-Constructor----------------------------------------------------------------------------------------------------------
//Public:
Command::Command(Core& coreRef) :
    Directorate(coreRef.director()),
    mCore(coreRef)
{}

//-Class Functions------------------------------------------------------------------
//Private:
template<class C>
    requires std::derived_from<C, Command>
void Command::registerCommand()
{
    smRegistry.emplace(C::NAME, Entry{
        .producer = [](Core& core)->std::unique_ptr<Command>{ return std::make_unique<C>(core); },
        .desc = &C::DESCRIPTION
    });
}

template <typename... Cs>
void Command::registerCommands() {
    (registerCommand<Cs>(), ...);
}

//Public:
void Command::registerAllCommands()
{
    /* This used to be handled independently in each Command's header file via the initialization (ctor) of static objects;
     * however, now that this is in a lib, when the lib is static those objects are discarded by the linker since they
     * aren't directly used (so the linker thinks they're worthless as it doesn't consider their side effects); therefore,
     * we have to have code that is reachable from the final application do the registration instead. So, we explicitly register
     * all commands here and then call this function in Driver, which is setup by the application.
     *
     * NOTE: REGISTER ALL COMMANDS HERE
     */
    registerCommands<
        CDownload,
        CLink,
        CPlay,
        CPrepare,
        CRun,
        CShare,
        CShow,
        CUpdate
    >();
}

CommandError Command::isRegistered(const QString &name)
{
    return smRegistry.contains(name) ? CommandError() : ERR_INVALID_COMMAND.arged(name);
}

QList<QString> Command::registered() { return smRegistry.keys(); }

bool Command::hasDescription(const QString& name) { return smRegistry.contains(name) ? !smRegistry.find(name)->desc->isEmpty() : false; }
QString Command::describe(const QString& name) { return smRegistry.contains(name) ? *(smRegistry.find(name)->desc) : QString(); }
std::unique_ptr<Command> Command::acquire(const QString& name, Core& core) { return smRegistry.contains(name) ? smRegistry.find(name)->producer(core) : nullptr; }

//-Instance Functions------------------------------------------------------------------------------------------------------
//Private:
void Command::logCommand(const QString& commandName) { logEvent(COMMAND_LABEL.arg(commandName)); }
void Command::logCommandOptions(const QString& commandOptions) { logEvent(COMMAND_OPT_LABEL.arg(commandOptions)); }

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

    // Log command (ensure "Command" name is used)
    logCommand(commandLine.first());
    logCommandOptions(optionsStr);

    if(validArgs)
        return CommandError();
    else
    {
        CommandError parseErr = CommandError(ERR_INVALID_ARGS).wDetails(mParser.errorText());
        postDirective<DError>(parseErr);
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
    postDirective<DMessage>(helpStr);
}

//Protected:
QList<const QCommandLineOption*> Command::options() const { return CL_OPTIONS_STANDARD; }
QSet<const QCommandLineOption*> Command::requiredOptions() const { return {}; }
QString Command::name() const { return NAME; };

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
        postDirective<DError>(processError);
        return processError;
    }

    // Perform command
    return perform();
}


