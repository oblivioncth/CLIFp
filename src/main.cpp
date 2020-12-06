#include "version.h"
#include <QApplication>
#include <QProcess>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <queue>
#include "qx-windows.h"
#include "qx-io.h"
#include "flashpointinstall.h"
#include "logger.h"
#include "magic_enum.hpp"

#define CLIFP_PATH QCoreApplication::applicationDirPath()
#define ENUM_NAME(...) QString::fromStdString(std::string(magic_enum::enum_name(__VA_ARGS__)))

//-Enums-----------------------------------------------------------------------
enum ErrorCode
{
    NO_ERR = 0x00,
    ALREADY_OPEN = 0x01,
    INVALID_ARGS = 0x02,
    LAUNCHER_OPEN = 0x03,
    INSTALL_INVALID = 0x04,
    CANT_PARSE_CONFIG = 0x05,
    CANT_PARSE_SERVICES = 0x06,
    CONFIG_SERVER_MISSING = 0x07,
    AUTO_ID_NOT_VALID = 0x08,
    SQL_ERROR = 0x09,
    DB_MISSING_TABLES = 0x0A,
    DB_MISSING_COLUMNS = 0x0B,
    AUTO_NOT_FOUND = 0x0C,
    MORE_THAN_ONE_AUTO = 0x0D,
    EXTRA_NOT_FOUND = 0x0E,
    EXECUTABLE_NOT_FOUND = 0x0F,
    EXECUTABLE_NOT_VALID = 0x10,
    PROCESS_START_FAIL = 0x11,
    WAIT_PROCESS_NOT_HANDLED = 0x12,
    WAIT_PROCESS_NOT_HOOKED = 0x13,
    CANT_READ_BAT_FILE = 0x14
};

enum class OperationMode { Invalid, Normal, Auto, Random, Message, Extra, Information };
enum class TaskType { Startup, Primary, Auxiliary, Wait, Shutdown };
enum class ProcessType { Blocking, Deferred, Detached };
enum class ErrorVerbosity { Full, Quiet, Silent };

//-Structs---------------------------------------------------------------------
struct AppTask
{
    TaskType type;
    QString path;
    QString filename;
    QStringList param;
    QString nativeParam;
    ProcessType processType;
};

//-Constants-------------------------------------------------------------------

// Command line strings
const QString CL_OPT_HELP_S_NAME = "h";
const QString CL_OPT_HELP_L_NAME = "help";
const QString CL_OPT_HELP_E_NAME = "?";
const QString CL_OPT_HELP_DESC = "Prints this help message.";

const QString CL_OPT_VERSION_S_NAME = "v";
const QString CL_OPT_VERSION_L_NAME = "version";
const QString CL_OPT_VERSION_DESC = "Prints the current version of this tool.";

const QString CL_OPT_APP_S_NAME = "x";
const QString CL_OPT_APP_L_NAME = "exe";
const QString CL_OPT_APP_DESC = "Relative (to Flashpoint directory) path of primary application to launch.";

const QString CL_OPT_PARAM_S_NAME = "p";
const QString CL_OPT_PARAM_L_NAME = "param";
const QString CL_OPT_PARAM_DESC = "Command-line parameters to use when starting the primary application.";

const QString CL_OPT_MSG_S_NAME = "m";
const QString CL_OPT_MSG_L_NAME = "msg";
const QString CL_OPT_MSG_DESC = "Displays an pop-up dialog with the supplied message. Used primarily for some additional apps.";

const QString CL_OPT_EXTRA_S_NAME = "e";
const QString CL_OPT_EXTRA_L_NAME = "extra";
const QString CL_OPT_EXTRA_DESC = "Opens an explorer window to the specified extra. Used primarily for some additional apps.";

const QString CL_OPT_AUTO_S_NAME = "a";
const QString CL_OPT_AUTO_L_NAME = "auto";
const QString CL_OPT_AUTO_DESC = "Finds a game/additional-app by UUID and runs it if found, including run-before additional apps in the case of a game.";

const QString CL_OPT_RAND_S_NAME = "r";
const QString CL_OPT_RAND_L_NAME = "random";
const QString CL_OPT_RAND_DESC = "Selects a random game UUID from the database and starts it in the same manner as using the --" + CL_OPT_AUTO_L_NAME + " switch.";

const QString CL_OPT_QUIET_S_NAME = "q";
const QString CL_OPT_QUIET_L_NAME = "quiet";
const QString CL_OPT_QUIET_DESC = "Silences all non-critical error messages.";

const QString CL_OPT_SILENT_S_NAME = "s";
const QString CL_OPT_SILENT_L_NAME = "silent";
const QString CL_OPT_SILENT_DESC = "Silences all error messages (takes precedence over quiet mode).";

// Command line messages
const QString CL_HELP_MESSAGE =
        "CLIFp Usage:<br>"
        "<br>"
        "<b>-" + CL_OPT_HELP_S_NAME + " | --" + CL_OPT_HELP_L_NAME + " | -" + CL_OPT_HELP_E_NAME + ":</b> &nbsp;" + CL_OPT_HELP_DESC + "<br>"
        "<b>-" + CL_OPT_VERSION_S_NAME + " | --" + CL_OPT_VERSION_L_NAME + ":</b> &nbsp;" + CL_OPT_VERSION_DESC + "<br>"
        "<b>-" + CL_OPT_APP_S_NAME + " | --" + CL_OPT_APP_L_NAME + ":</b> &nbsp;" + CL_OPT_APP_DESC + "<br>"
        "<b>-" + CL_OPT_PARAM_S_NAME + " | --" + CL_OPT_PARAM_L_NAME + ":</b> &nbsp;" + CL_OPT_PARAM_DESC + "<br>"
        "<b>-" + CL_OPT_AUTO_S_NAME + " | --" + CL_OPT_AUTO_L_NAME + ":</b> &nbsp;" + CL_OPT_AUTO_DESC + "<br>"
        "<b>-" + CL_OPT_RAND_S_NAME + " | --" + CL_OPT_RAND_L_NAME + ":</b> &nbsp;" + CL_OPT_RAND_DESC + "<br>"
        "<b>-" + CL_OPT_MSG_S_NAME + " | --" + CL_OPT_MSG_L_NAME + ":</b> &nbsp;" + CL_OPT_MSG_DESC + "<br>"
        "<b>-" + CL_OPT_EXTRA_S_NAME + " | --" + CL_OPT_EXTRA_L_NAME + ":</b> &nbsp;" + CL_OPT_EXTRA_DESC + "<br>"
        "<b>-" + CL_OPT_QUIET_S_NAME + " | --" + CL_OPT_QUIET_L_NAME + ":</b> &nbsp;" + CL_OPT_QUIET_DESC + "<br>"
        "<b>-" + CL_OPT_SILENT_S_NAME + " | --" + CL_OPT_SILENT_L_NAME + ":</b> &nbsp;" + CL_OPT_SILENT_DESC + "<br>"
        "Use <b>'" + CL_OPT_APP_L_NAME + "'</b> and <b>'" + CL_OPT_PARAM_L_NAME + "'</b> for normal operation, use <b>'" + CL_OPT_AUTO_L_NAME +
        "'</b> by itself for automatic operation, use <b>'" + CL_OPT_MSG_L_NAME  + "'</b> by itself for random operation,  use <b>'" + CL_OPT_MSG_L_NAME  +
        "'</b> to display a popup message, use <b>'" + CL_OPT_EXTRA_L_NAME + "'</b> to view an extra, or use <b>'" + CL_OPT_HELP_L_NAME +
        "'</b> and/or <b>'" + CL_OPT_VERSION_L_NAME + "'</b> for information.";

const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " VER_PRODUCTVERSION_STR ", designed for use with BlueMaxima's Flashpoint " VER_PRODUCTVERSION_STR "+";

// FP Server Applications
const QFileInfo SECURE_PLAYER_INFO = QFileInfo("FlashpointSecurePlayer.exe");

// Error Messages - Prep
const QString ERR_ALREADY_OPEN = "Only one instance of CLIFp can be used at a time!";
const QString ERR_LAUNCHER_RUNNING_P = "The CLI cannot be used while the Flashpoint Launcher is running.";
const QString ERR_LAUNCHER_RUNNING_S = "Please close the Launcher first.";
const QString ERR_INSTALL_INVALID_P = "CLIFp does not appear to be deployed in a valid Flashpoint install";
const QString ERR_INSTALL_INVALID_S = "Check its location and compatability with your Flashpoint version.";

const QString ERR_CANT_PARSE_FILE = "Failed to parse %1 ! It may be corrupted or not compatible with this version of CLIFp.";
const QString ERR_CONFIG_SERVER_MISSING = "The server specified in the Flashpoint config was not found within the Flashpoint services store.";
const QString ERR_UNEXPECTED_SQL = "Unexpected SQL error while querying the Flashpoint database:";
const QString ERR_DB_MISSING_TABLE = "The Flashpoint database is missing expected tables.";
const QString ERR_DB_TABLE_MISSING_COLUMN = "The Flashpoint database tables are missing expected columns.";
const QString ERR_AUTO_NOT_FOUND = "An entry matching the specified auto ID could not be found in the Flashpoint database.";
const QString ERR_AUTO_INVALID = "The provided string for auto operation was not a valid GUID/UUID.";
const QString ERR_MORE_THAN_ONE_AUTO_P = "Multiple entries with the specified auto ID were found.";
const QString ERR_MORE_THAN_ONE_AUTO_S = "This should not be possible and may indicate an error within the Flashpoint database";

// Error Messages - Execution
const QString ERR_EXTRA_NOT_FOUND = "The extra %1 does not exist!";
const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";
const QString WRN_WAIT_PROCESS_NOT_HANDLED_P  = "Could not get a wait handle to %1.";
const QString WRN_WAIT_PROCESS_NOT_HANDLED_S = "The title may not work correctly";
const QString WRN_WAIT_PROCESS_NOT_HOOKED_P  = "Could not hook %1 for waiting.";
const QString WRN_WAIT_PROCESS_NOT_HOOKED_S = "The title may not work correctly";

// CLI Options
const QCommandLineOption CL_OPTION_APP({CL_OPT_APP_S_NAME, CL_OPT_APP_L_NAME}, CL_OPT_APP_DESC, "application"); // Takes value
const QCommandLineOption CL_OPTION_PARAM({CL_OPT_PARAM_S_NAME, CL_OPT_PARAM_L_NAME}, CL_OPT_PARAM_DESC, "parameters"); // Takes value
const QCommandLineOption CL_OPTION_AUTO({CL_OPT_AUTO_S_NAME, CL_OPT_AUTO_L_NAME}, CL_OPT_AUTO_DESC, "id"); // Takes value
const QCommandLineOption CL_OPTION_RAND({CL_OPT_RAND_S_NAME, CL_OPT_RAND_L_NAME}, CL_OPT_RAND_DESC); // Boolean option
const QCommandLineOption CL_OPTION_MSG({CL_OPT_MSG_S_NAME, CL_OPT_MSG_L_NAME}, CL_OPT_MSG_DESC, "message"); // Takes value
const QCommandLineOption CL_OPTION_EXTRA({CL_OPT_EXTRA_S_NAME, CL_OPT_EXTRA_L_NAME}, CL_OPT_EXTRA_DESC, "extra"); // Takes value
const QCommandLineOption CL_OPTION_HELP({CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC); // Boolean option
const QCommandLineOption CL_OPTION_VERSION({CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC); // Boolean option
const QCommandLineOption CL_OPTION_QUIET({CL_OPT_QUIET_S_NAME, CL_OPT_QUIET_L_NAME}, CL_OPT_QUIET_DESC); // Boolean option
const QCommandLineOption CL_OPTION_SILENT({CL_OPT_SILENT_S_NAME, CL_OPT_SILENT_L_NAME}, CL_OPT_SILENT_DESC); // Boolean option
const QList<const QCommandLineOption*> CL_OPTIONS_MAIN{&CL_OPTION_APP, &CL_OPTION_PARAM, &CL_OPTION_AUTO, &CL_OPTION_MSG,
                                                       &CL_OPTION_EXTRA, &CL_OPTION_HELP, &CL_OPTION_VERSION, &CL_OPTION_RAND};
const QList<const QCommandLineOption*> CL_OPTIONS_ALL = CL_OPTIONS_MAIN + QList<const QCommandLineOption*>{&CL_OPTION_QUIET, &CL_OPTION_SILENT};

// CLI Option Operation Mode Map TODO: Submit a patch for Qt6 to make QCommandLineOption directly hashable (implement == and qHash)
const QHash<QSet<QString>, OperationMode> CL_MAIN_OPTIONS_OP_MODE_MAP{
    {{CL_OPT_APP_S_NAME, CL_OPT_PARAM_S_NAME}, OperationMode::Normal},
    {{CL_OPT_AUTO_S_NAME}, OperationMode::Auto},
    {{CL_OPT_RAND_S_NAME}, OperationMode::Random},
    {{CL_OPT_MSG_S_NAME}, OperationMode::Message},
    {{CL_OPT_EXTRA_S_NAME}, OperationMode::Extra},
    {{CL_OPT_HELP_S_NAME}, OperationMode::Information},
    {{CL_OPT_VERSION_S_NAME}, OperationMode::Information},
    {{CL_OPT_HELP_S_NAME, CL_OPT_VERSION_S_NAME}, OperationMode::Information}
};

// Suffixes
const QString EXE_SUFX = "exe";
const QString BAT_SUFX = "bat";

// Processing constants
const QString CMD_EXE = "cmd.exe";
const QString CMD_ARG_TEMPLATE = R"(/d /s /c ""%1" %2")";

// Wait timing
const int SECURE_PLAYER_GRACE = 2; // Seconds to allow the secure player to restart in cases it does

// Logging - General
const QString LOG_FILE_NAME = VER_INTERNALNAME_STR ".log";
const QString LOG_HEADER = "CLIFp Execution Log";
const QString LOG_NO_PARAMS = "*None*";
const int LOG_MAX_ENTRIES = 50;

// Logging - Messages
const QString LOG_ERR_INVALID_PARAM = "Invalid combination of parameters used";
const QString LOG_ERR_CRITICAL = "Aborting execution due to previous critical errors";
const QString LOG_WRN_INVALID_RAND_ID = "A UUID found in the database during Random operation is invalid (%1)";
const QString LOG_EVENT_FLASHPOINT_LINK = "Linked to Flashpoint install at: %1";
const QString LOG_EVENT_OP_MODE = "Operation Mode: %1";
const QString LOG_EVENT_APP_TASK = "Enqueued App Task: {.type = %1, .path = \"%2\", .filename = \"%3\", "
                                   ".param = {\"%4\"}, .nativeParam = \"%5\", .processType = %6}";
const QString LOG_EVENT_SHOW_MESSAGE = "Displayed message";
const QString LOG_EVENT_SHOW_EXTRA = "Opened folder of extra %1";
const QString LOG_EVENT_HELP_SHOWN = "Displayed help information";
const QString LOG_EVENT_VER_SHOWN = "Displayed version information";
const QString LOG_EVENT_INIT = "Initializing CLIFp...";
const QString LOG_EVENT_GET_SET = "Reading Flashpoint configuration...";
const QString LOG_EVENT_SEL_RAND = "Selecting a playable game at random...";
const QString LOG_EVENT_RAND_ID = "Randomly chose game \"%1\"";
const QString LOG_EVENT_PLAYABLE_COUNT = "Found %1 playable games";
const QString LOG_EVENT_ENQ_START = "Enqueuing startup tasks...";
const QString LOG_EVENT_ENQ_AUTO = "Enqueuing automatic tasks...";
const QString LOG_EVENT_ENQ_STOP = "Enqueuing shutdown tasks...";
const QString LOG_EVENT_QUEUE_START = "Processing App Task queue";
const QString LOG_EVENT_TASK_START = "Handling task %1 (%2)";
const QString LOG_EVENT_TASK_FINISH = "End of task %1 (%2)";
const QString LOG_EVENT_QUEUE_FINISH = "Finished processing App Task queue";
const QString LOG_EVENT_CLEANUP_FINISH = "Finished cleanup";
const QString LOG_EVENT_ALL_FINISH = "Execution finished successfully";
const QString LOG_EVENT_ID_MATCH_TITLE = "Auto ID matches main title: %1";
const QString LOG_EVENT_ID_MATCH_ADDAPP = "Auto ID matches additional app: %1";
const QString LOG_EVENT_QUEUE_CLEARED = "Previous queue entries cleared due to auto task being a Message/Extra";
const QString LOG_EVENT_FOUND_AUTORUN = "Found autorun-before additional app: %1";
const QString LOG_EVENT_TASK_SKIP = "App Task skipped due to previous errors";
const QString LOG_EVENT_CD = "Changed current directory to: %1";
const QString LOG_EVENT_START_PROCESS = "Started %1 process: %2";
const QString LOG_EVENT_END_PROCESS = "%1 process %2 finished";
const QString LOG_EVENT_ARGS_ESCAPED = "CMD arguments escaped from [[%1]] to [[%2]]";
const QString LOG_EVENT_WAIT_GRACE = "Waiting " + QString::number(SECURE_PLAYER_GRACE) + " seconds for process %1 to be running";
const QString LOG_EVENT_WAIT_RUNNING = "Wait-on process %1 is running";
const QString LOG_EVENT_WAIT_ON = "Waiting for process %1 to finish";
const QString LOG_EVENT_WAIT_QUIT = "Wait-on process %1 has finished";
const QString LOG_EVENT_WAIT_FINISHED = "Wait-on process %1 was not running after the grace period";

// Globals
ErrorVerbosity gErrorVerbosity = ErrorVerbosity::Full;
std::unique_ptr<Logger> gLogger;
bool gCritErrOccured = false;

// Prototypes - Process
ErrorCode openAndVerifyProperDatabase(FP::Install& fpInstall);
ErrorCode enqueueStartupTasks(std::queue<AppTask>& taskQueue, FP::Install::Config fpConfig, FP::Install::Services fpServices);
ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, QUuid targetID, FP::Install& fpInstall);
ErrorCode enqueueAdditionalApp(std::queue<AppTask>& taskQueue, FP::Install::DBQueryBuffer addAppResult, TaskType taskType, const FP::Install& fpInstall);
void enqueueShutdownTasks(std::queue<AppTask>& taskQueue, FP::Install::Services fpServices);
ErrorCode enqueueConditionalWaitTask(std::queue<AppTask>& taskQueue, QFileInfo precedingAppInfo);
ErrorCode processTaskQueue(std::queue<AppTask>& taskQueue, QList<QProcess*>& childProcesses);
void handleExecutionError(std::queue<AppTask>& taskQueue, ErrorCode& currentError, ErrorCode newError);
bool cleanStartProcess(QProcess* process, QFileInfo exeInfo);
ErrorCode waitOnProcess(QString processName, int graceSecs);
void cleanup(FP::Install& fpInstall, QList<QProcess*>& childProcesses);

// Prototypes - Helper
QString getRawCommandLineParams();
ErrorCode randomlySelectID(QUuid& idBuffer, FP::Install& fpInstall);
Qx::GenericError appInvolvesSecurePlayer(bool& involvesBuffer, QFileInfo appInfo);
QString escapeNativeArgsForCMD(QString nativeArgs);
void postError(Qx::GenericError error, bool log = true);
void logEvent(QString event);
void logAppTask(const AppTask& appTask);
void logProcessStart(const QProcess* process, ProcessType type);
void logProcessEnd(const QProcess* process, ProcessType type);
void logError(Qx::GenericError error);
int printLogAndExit(int exitCode);

int main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // QApplication Object
    QApplication app(argc, argv);

    // Set application name
    QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
    QCoreApplication::setApplicationVersion(VER_FILEVERSION_STR);

    // CLI Parser
    QCommandLineParser clParser;
    clParser.setApplicationDescription(VER_FILEDESCRIPTION_STR);
    for(const QCommandLineOption* clOption : CL_OPTIONS_ALL)
        clParser.addOption(*clOption);
    clParser.process(app);

    // Set globals based on general flags
    gErrorVerbosity = clParser.isSet(CL_OPTION_SILENT) ? ErrorVerbosity::Silent :
                      clParser.isSet(CL_OPTION_QUIET) ? ErrorVerbosity::Quiet : ErrorVerbosity::Full;

    //-Create Logger-----------------------------------------------------------------------

    // Command line strings
    QString rawCL = getRawCommandLineParams();
    if(rawCL.isEmpty())
        rawCL = LOG_NO_PARAMS;

    QString interpCL;
    for(const QCommandLineOption* clOption : CL_OPTIONS_ALL)
    {
        if(clParser.isSet(*clOption))
        {
            // Add switch to interp string
            if(!interpCL.isEmpty())
                interpCL += " "; // Space after every switch except first one

            interpCL += "--" + (*clOption).names().at(1); // Always use long name

            // Add value of switch if it takes one
            if(!(*clOption).valueName().isEmpty())
                interpCL += R"(=")" + clParser.value(*clOption) + R"(")";
        }
    }

    if(interpCL.isEmpty())
        interpCL = LOG_NO_PARAMS;

    // Logger instance
    gLogger = std::make_unique<Logger>(CLIFP_PATH + '/' + LOG_FILE_NAME, rawCL, interpCL, LOG_HEADER, LOG_MAX_ENTRIES);

    //-Determine Operation Mode------------------------------------------------------------
    OperationMode operationMode;
    QSet<QString> providedArgs;
    for(const QCommandLineOption* clOption : CL_OPTIONS_ALL)
        if(CL_OPTIONS_MAIN.contains(clOption) && clParser.isSet(*clOption))
            providedArgs.insert((*clOption).names().value(0)); // Add options shortname to set

    if(CL_MAIN_OPTIONS_OP_MODE_MAP.contains(providedArgs))
        operationMode = CL_MAIN_OPTIONS_OP_MODE_MAP.value(providedArgs);
    else
        operationMode = OperationMode::Invalid;
    logEvent(LOG_EVENT_OP_MODE.arg(ENUM_NAME(operationMode)));

    // If mode is Information or Invalid, handle it immediately
    if(operationMode == OperationMode::Information)
    {
        if(clParser.isSet(CL_OPTION_VERSION))
        {
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_VERSION_MESSAGE);
            logEvent(LOG_EVENT_HELP_SHOWN);
        }
        if(clParser.isSet(CL_OPTION_HELP))
        {
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);
            logEvent(LOG_EVENT_HELP_SHOWN);
        }
        return printLogAndExit(NO_ERR);
    }
    else if(operationMode == OperationMode::Invalid)
    {
        QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);
        logError(Qx::GenericError(Qx::GenericError::Error, LOG_ERR_INVALID_PARAM));
        return printLogAndExit(INVALID_ARGS);
    }

    logEvent(LOG_EVENT_INIT);

    //-Restrict app to only one instance---------------------------------------------------
    if(!Qx::enforceSingleInstance())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ALREADY_OPEN));
        return printLogAndExit(ALREADY_OPEN);
    }

    // Ensure Flashpoint Launcher isn't running
    if(Qx::processIsRunning(QFileInfo(FP::Install::MAIN_EXE_PATH).fileName()))
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_LAUNCHER_RUNNING_P, ERR_LAUNCHER_RUNNING_S));
        return printLogAndExit(LAUNCHER_OPEN);
    }

    //-Link to Flashpoint Install----------------------------------------------------------
    if(!FP::Install::checkInstallValidity(CLIFP_PATH, FP::Install::CompatLevel::Execution).installValid)
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_INSTALL_INVALID_P, ERR_INSTALL_INVALID_S));
        return printLogAndExit(INSTALL_INVALID);
    }

    FP::Install flashpointInstall(CLIFP_PATH);
    logEvent(LOG_EVENT_FLASHPOINT_LINK.arg(CLIFP_PATH));

    //-Get Install Settings----------------------------------------------------------------
    logEvent(LOG_EVENT_GET_SET);
    Qx::GenericError settingsReadError;
    FP::Install::Config flashpointConfig;
    FP::Install::Services flashpointServices;

    if((settingsReadError = flashpointInstall.getConfig(flashpointConfig)).isValid())
    {
        postError(settingsReadError);
        return printLogAndExit(CANT_PARSE_CONFIG);
    }

    if((settingsReadError = flashpointInstall.getServices(flashpointServices)).isValid())
    {
        postError(settingsReadError);
        return printLogAndExit(CANT_PARSE_SERVICES);
    }

    //-Proccess Tracking-------------------------------------------------------------------
    QList<QProcess*> activeChildProcesses;
    std::queue<AppTask> appTaskQueue;

    //-Handle Mode Specific Operations-----------------------------------------------------
    ErrorCode enqueueError;
    QFileInfo inputInfo;
    QUuid autoID;
    AppTask normalTask;

    switch(operationMode)
    {    
        case OperationMode::Invalid:
            // Already handled
            break;

        case OperationMode::Normal: 
            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)))
                return printLogAndExit(enqueueError);

            inputInfo = QFileInfo(CLIFP_PATH + '/' + clParser.value(CL_OPTION_APP));
            normalTask = {TaskType::Primary, inputInfo.absolutePath(), inputInfo.fileName(),
                                  QStringList(), clParser.value(CL_OPTION_PARAM),
                                  ProcessType::Blocking};
            appTaskQueue.push(normalTask);
            logAppTask(normalTask);

            // Add wait task if required
            if((enqueueError = enqueueConditionalWaitTask(appTaskQueue, inputInfo)))
                return printLogAndExit(enqueueError);
            break;
        case OperationMode::Random:
            if((enqueueError = openAndVerifyProperDatabase(flashpointInstall)))
                return printLogAndExit(enqueueError);

            if((enqueueError = randomlySelectID(autoID, flashpointInstall)))
                return printLogAndExit(enqueueError);

            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)))
                return printLogAndExit(enqueueError);

            if((enqueueError = enqueueAutomaticTasks(appTaskQueue, autoID, flashpointInstall)))
                return printLogAndExit(enqueueError);
            break;


        case OperationMode::Auto:
            if((autoID = QUuid(clParser.value(CL_OPTION_AUTO))).isNull())
            {
                postError(Qx::GenericError(Qx::GenericError::Critical, ERR_AUTO_INVALID));
                return printLogAndExit(AUTO_ID_NOT_VALID);
            }

            if((enqueueError = openAndVerifyProperDatabase(flashpointInstall)))
                return printLogAndExit(enqueueError);

            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)))
                return printLogAndExit(enqueueError);

            if((enqueueError = enqueueAutomaticTasks(appTaskQueue, autoID, flashpointInstall)))
                return printLogAndExit(enqueueError);
            break;

        case OperationMode::Message:
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), clParser.value(CL_OPTION_MSG));
            logEvent(LOG_EVENT_SHOW_MESSAGE);
            return printLogAndExit(NO_ERR);

        case OperationMode::Extra:
            // Ensure extra exists
            inputInfo = QFileInfo(flashpointInstall.getExtrasDirectory().absolutePath() + '/' + clParser.value(CL_OPTION_EXTRA));
            if(!inputInfo.exists() || !inputInfo.isDir())
            {
                postError(Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(inputInfo.fileName())));
                return printLogAndExit(EXTRA_NOT_FOUND);
            }

            // Open extra
            QDesktopServices::openUrl(QUrl::fromLocalFile(inputInfo.absoluteFilePath()));
            logEvent(LOG_EVENT_SHOW_EXTRA.arg(inputInfo.fileName()));
            return printLogAndExit(NO_ERR);

        case OperationMode::Information:
            // Already handled
            break;
    }

    // Enqueue Shudown Tasks if main task isn't message/extra
    if(appTaskQueue.size() != 1 ||
       (appTaskQueue.front().path != FP::Install::DBTable_Add_App::ENTRY_MESSAGE &&
       appTaskQueue.front().path != FP::Install::DBTable_Add_App::ENTRY_EXTRAS))
        enqueueShutdownTasks(appTaskQueue, flashpointServices);

    // Process app task queue
    ErrorCode executionError = processTaskQueue(appTaskQueue, activeChildProcesses);
    logEvent(LOG_EVENT_QUEUE_FINISH);

    // Cleanup
    cleanup(flashpointInstall, activeChildProcesses);
    logEvent(LOG_EVENT_CLEANUP_FINISH);

    // Return error status
    logEvent(LOG_EVENT_ALL_FINISH);
    return printLogAndExit(executionError);
}

//-Processing Functions-------------------------------------------------------------
ErrorCode openAndVerifyProperDatabase(FP::Install& fpInstall)
{
    // Open database connection
    QSqlError databaseError = fpInstall.openThreadDatabaseConnection();
    if(databaseError.isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, databaseError.text()));
        return SQL_ERROR;
    }

    // Ensure required database tables are present
    QSet<QString> missingTables;
    if((databaseError = fpInstall.checkDatabaseForRequiredTables(missingTables)).isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, databaseError.text()));
        return SQL_ERROR;
    }

    // Check if tables are missing
    if(!missingTables.isEmpty())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_DB_MISSING_TABLE, QString(),
                         QStringList(missingTables.begin(), missingTables.end()).join("\n")));
        return DB_MISSING_TABLES;
    }

    // Ensure the database contains the required columns
    QSet<QString> missingColumns;
    if((databaseError = fpInstall.checkDatabaseForRequiredColumns(missingColumns)).isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, databaseError.text()));
        return SQL_ERROR;
    }

    // Check if columns are missing
    if(!missingColumns.isEmpty())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_DB_TABLE_MISSING_COLUMN, QString(),
                         QStringList(missingColumns.begin(), missingColumns.end()).join("\n")));
        return DB_MISSING_COLUMNS;
    }

    // Return success
    return NO_ERR;
}

ErrorCode enqueueStartupTasks(std::queue<AppTask>& taskQueue, FP::Install::Config fpConfig, FP::Install::Services fpServices)
{
    logEvent(LOG_EVENT_ENQ_START);
    // Add Start entries from services
    for(const FP::Install::StartStop& startEntry : fpServices.starts)
    {
        AppTask currentTask = {TaskType::Startup, CLIFP_PATH + '/' + startEntry.path, startEntry.filename,
                               startEntry.arguments, QString(),
                               ProcessType::Blocking};
        taskQueue.push(currentTask);
        logAppTask(currentTask);
    }

    // Add Server entry from services if applicable
    if(fpConfig.startServer)
    {
        if(!fpServices.servers.contains(fpConfig.server))
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_CONFIG_SERVER_MISSING));
            return CONFIG_SERVER_MISSING;
        }

        FP::Install::ServerDaemon configuredServer = fpServices.servers.value(fpConfig.server);
        AppTask serverTask = {TaskType::Startup, CLIFP_PATH + '/' + configuredServer.path, configuredServer.filename,
                              configuredServer.arguments, QString(),
                              configuredServer.kill ? ProcessType::Deferred : ProcessType::Detached};
        taskQueue.push(serverTask);
        logAppTask(serverTask);
    }

    // Add Daemon entry from services
    QHash<QString, FP::Install::ServerDaemon>::const_iterator daemonIt;
    for (daemonIt = fpServices.daemons.constBegin(); daemonIt != fpServices.daemons.constEnd(); ++daemonIt)
    {
        AppTask currentTask = {TaskType::Startup, CLIFP_PATH + '/' + daemonIt.value().path, daemonIt.value().filename,
                               daemonIt.value().arguments, QString(),
                               daemonIt.value().kill ? ProcessType::Deferred : ProcessType::Detached};
        taskQueue.push(currentTask);
        logAppTask(currentTask);
    }

    // Return success
    return NO_ERR;
}

ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, QUuid targetID, FP::Install& fpInstall)
{
    logEvent(LOG_EVENT_ENQ_AUTO);
    // Search FP database for entry via ID
    QSqlError searchError;
    FP::Install::DBQueryBuffer searchResult;
    ErrorCode enqueueError;

    searchError = fpInstall.queryEntryByID(searchResult, targetID);
    if(searchError.isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(searchResult.size == 0)
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_AUTO_NOT_FOUND));
        return AUTO_NOT_FOUND;
    }
    else if(searchResult.size > 1)
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_MORE_THAN_ONE_AUTO_P, ERR_MORE_THAN_ONE_AUTO_S));
        return MORE_THAN_ONE_AUTO;
    }

    // Advance result to only record
    searchResult.result.next();

    // Enqueue if result is additional app
    if(searchResult.source == FP::Install::DBTable_Add_App::NAME)
    {
        logEvent(LOG_EVENT_ID_MATCH_ADDAPP.arg(searchResult.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString()));

        // Replace queue with this single entry if it is a message or extra
        QString appPath = searchResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
        if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE || appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
            taskQueue = std::queue<AppTask>();

        logEvent(LOG_EVENT_QUEUE_CLEARED);

        ErrorCode enqueueError = enqueueAdditionalApp(taskQueue, searchResult, TaskType::Primary, fpInstall);
        if(enqueueError)
            return enqueueError;
    }
    else if(searchResult.source == FP::Install::DBTable_Game::NAME) // Get autorun additional apps if result is game
    {
        logEvent(LOG_EVENT_ID_MATCH_TITLE.arg(searchResult.result.value(FP::Install::DBTable_Game::COL_TITLE).toString()));

        // Get game's additional apps
        QSqlError addAppSearchError;
        FP::Install::DBQueryBuffer addAppSearchResult;

        addAppSearchError = fpInstall.queryEntryAddApps(addAppSearchResult, targetID);
        if(addAppSearchError.isValid())
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, addAppSearchError.text()));
            return SQL_ERROR;
        }

        // Enqueue autorun before apps
        for(int i = 0; i < addAppSearchResult.size; i++)
        {
            // Go to next record
            addAppSearchResult.result.next();

            // Enqueue if autorun before
            if(addAppSearchResult.result.value(FP::Install::DBTable_Add_App::COL_AUTORUN).toInt() != 0)
            {
                logEvent(LOG_EVENT_FOUND_AUTORUN.arg(addAppSearchResult.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString()));
                enqueueError = enqueueAdditionalApp(taskQueue, addAppSearchResult, TaskType::Auxiliary, fpInstall);
                if(enqueueError)
                    return enqueueError;
            }
        }

        // Enqueue game
        QString gamePath = searchResult.result.value(FP::Install::DBTable_Game::COL_APP_PATH).toString();
        QString gameArgs = searchResult.result.value(FP::Install::DBTable_Game::COL_LAUNCH_COMMAND).toString();
        QFileInfo gameInfo(CLIFP_PATH + '/' + gamePath);
        AppTask gameTask = {TaskType::Primary, gameInfo.absolutePath(), gameInfo.fileName(),
                            QStringList(), gameArgs, ProcessType::Blocking};
        taskQueue.push(gameTask);
        logAppTask(gameTask);

        // Add wait task if required
        if((enqueueError = enqueueConditionalWaitTask(taskQueue, gameInfo)))
            return enqueueError;
    }
    else
        throw std::runtime_error("Auto ID search result source must be 'game' or 'additional_app'");

    // Return success
    return NO_ERR;
}

ErrorCode enqueueAdditionalApp(std::queue<AppTask>& taskQueue, FP::Install::DBQueryBuffer addAppResult, TaskType taskType, const FP::Install& fpInstall)
{
    // Ensure query result is additional app
    assert(addAppResult.source == FP::Install::DBTable_Add_App::NAME);

    QString appPath = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
    QString appArgs = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_LAUNCH_COMMAND).toString();
    bool waitForExit = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_WAIT_EXIT).toInt() != 0;

    if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE)
    {
        AppTask messageTask = {taskType, appPath, QString(),
                               QStringList(), appArgs,
                               (waitForExit || taskType == TaskType::Primary) ? ProcessType::Blocking : ProcessType::Deferred};
        taskQueue.push(messageTask);
        logAppTask(messageTask);
    }
    else if(appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
    {
        AppTask extraTask = {taskType, appPath, QString(),
                             QStringList(), fpInstall.getExtrasDirectory().absolutePath() + "/" + appArgs,
                             ProcessType::Detached};
        taskQueue.push(extraTask);
        logAppTask(extraTask);
    }
    else
    {
        QFileInfo addAppInfo(CLIFP_PATH + '/' + appPath);
        AppTask addAppTask = {taskType, addAppInfo.absolutePath(), addAppInfo.fileName(),
                              QStringList(), appArgs,
                              (waitForExit || taskType == TaskType::Primary) ? ProcessType::Blocking : ProcessType::Deferred};
        taskQueue.push(addAppTask);
        logAppTask(addAppTask);

        // Add wait task if required
        ErrorCode enqueueError = enqueueConditionalWaitTask(taskQueue, addAppInfo);
        if(enqueueError)
            return enqueueError;
    }

    // Return success
    return NO_ERR;
}

void enqueueShutdownTasks(std::queue<AppTask>& taskQueue, FP::Install::Services fpServices)
{
    logEvent(LOG_EVENT_ENQ_STOP);
    // Add Stop entries from services
    for(const FP::Install::StartStop& stopEntry : fpServices.stops)
    {
        AppTask shutdownTask = {TaskType::Shutdown, CLIFP_PATH + '/' + stopEntry.path, stopEntry.filename,
                                stopEntry.arguments, QString(),
                                ProcessType::Blocking};
        taskQueue.push(shutdownTask);
        logAppTask(shutdownTask);
    }
}

ErrorCode enqueueConditionalWaitTask(std::queue<AppTask>& taskQueue, QFileInfo precedingAppInfo)
{
    // Add wait for apps that involve secure player
    bool involvesSecurePlayer;
    Qx::GenericError securePlayerCheckError = appInvolvesSecurePlayer(involvesSecurePlayer, precedingAppInfo);
    if(securePlayerCheckError.isValid())
    {
        postError(securePlayerCheckError);
        return CANT_READ_BAT_FILE;
    }

    if(involvesSecurePlayer)
    {
        AppTask waitTask = {TaskType::Wait, QString(), SECURE_PLAYER_INFO.fileName(),
                            QStringList(), QString(),
                            ProcessType::Blocking};
        taskQueue.push(waitTask);
        logAppTask(waitTask);
    }

    // Return success
    return NO_ERR;

    // Possible future waits...
}

ErrorCode processTaskQueue(std::queue<AppTask>& taskQueue, QList<QProcess*>& childProcesses)
{
    logEvent(LOG_EVENT_QUEUE_START);
    // Error tracker
    ErrorCode executionError = NO_ERR;

    // Exhaust queue
    int tasksHandled = 0;
    while(!taskQueue.empty())
    {
        // Handle task at front of queue
        AppTask currentTask = taskQueue.front();
        logEvent(LOG_EVENT_TASK_START.arg(tasksHandled).arg(ENUM_NAME(currentTask.type)));

        // Only execute task after an error if it is a Shutdown task
        if(!executionError || currentTask.type == TaskType::Shutdown)
        {
            // Handle general and special case(s)
            if(currentTask.path == FP::Install::DBTable_Add_App::ENTRY_MESSAGE) // Message (currently ignores process type)
            {
                QMessageBox::information(nullptr, QCoreApplication::applicationName(), currentTask.nativeParam);
                logEvent(LOG_EVENT_SHOW_MESSAGE);
            }
            else if(currentTask.path == FP::Install::DBTable_Add_App::ENTRY_EXTRAS) // Extra
            {
                // Ensure extra exists
                QFileInfo extraInfo(currentTask.nativeParam);
                if(!extraInfo.exists() || !extraInfo.isDir())
                {
                    postError(Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(extraInfo.fileName())));
                    handleExecutionError(taskQueue, executionError, EXTRA_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Open extra
                QDesktopServices::openUrl(QUrl::fromLocalFile(extraInfo.absoluteFilePath()));
                logEvent(LOG_EVENT_SHOW_EXTRA.arg(extraInfo.fileName()));
            }
            else if(currentTask.type == TaskType::Wait) // Wait task
            {
                ErrorCode waitError = waitOnProcess(currentTask.filename, SECURE_PLAYER_GRACE);
                if(waitError)
                {
                    handleExecutionError(taskQueue, executionError, waitError);
                    continue; // Continue to next task
                }
            }
            else // General case
            {
                // Ensure executable exists
                QFileInfo executableInfo(currentTask.path + "/" + currentTask.filename);
                if(!executableInfo.exists() || !executableInfo.isFile())
                {
                    postError(Qx::GenericError(currentTask.type == TaskType::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                               ERR_EXE_NOT_FOUND.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(taskQueue, executionError, EXECUTABLE_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Ensure executable is valid
                if(!executableInfo.isExecutable())
                {
                    postError(Qx::GenericError(currentTask.type == TaskType::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                               ERR_EXE_NOT_VALID.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(taskQueue, executionError, EXECUTABLE_NOT_VALID);
                    continue; // Continue to next task
                }

                // Move to executable directory
                QDir::setCurrent(currentTask.path);
                logEvent(LOG_EVENT_CD.arg(currentTask.path));

                // Check if task is a batch file
                bool batchTask = QFileInfo(currentTask.filename).suffix() == BAT_SUFX;

                // Create process handle
                QProcess* taskProcess = new QProcess();
                taskProcess->setProgram(batchTask ? CMD_EXE : currentTask.filename);
                taskProcess->setArguments(currentTask.param);
                taskProcess->setNativeArguments(batchTask ? CMD_ARG_TEMPLATE.arg(currentTask.filename, escapeNativeArgsForCMD(currentTask.nativeParam))
                                                          : currentTask.nativeParam);
                taskProcess->setStandardOutputFile(QProcess::nullDevice()); // Don't inhert console window
                taskProcess->setStandardErrorFile(QProcess::nullDevice()); // Don't inhert console window

                // Cover each process type
                switch(currentTask.processType)
                {
                    case ProcessType::Blocking:
                        if(!cleanStartProcess(taskProcess, executableInfo))
                        {
                            handleExecutionError(taskQueue, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, ProcessType::Blocking);

                        taskProcess->waitForFinished(-1); // Wait for process to end, don't time out
                        logProcessEnd(taskProcess, ProcessType::Blocking);
                        delete taskProcess; // Clear finished process handle from heap
                        break;

                    case ProcessType::Deferred:
                        if(!cleanStartProcess(taskProcess, executableInfo))
                        {
                            handleExecutionError(taskQueue, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, ProcessType::Deferred);

                        childProcesses.append(taskProcess); // Add process to list for deferred termination
                        break;

                    case ProcessType::Detached:
                        if(!taskProcess->startDetached())
                        {
                            handleExecutionError(taskQueue, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, ProcessType::Detached);
                        break;
                }
            }
        }
        else
        {
            logEvent(LOG_EVENT_TASK_SKIP);
            ++tasksHandled;
        }

        // Remove handled task
        taskQueue.pop();
        logEvent(LOG_EVENT_TASK_FINISH.arg(tasksHandled++).arg(ENUM_NAME(currentTask.type)));

    }

    // Return error status
    return executionError;
}

void handleExecutionError(std::queue<AppTask>& taskQueue, ErrorCode& currentError, ErrorCode newError)
{
    if(!currentError) // Only record first error
        currentError = newError;

    taskQueue.pop(); // Remove erroring task
}

bool cleanStartProcess(QProcess* process, QFileInfo exeInfo)
{
    // Start process and confirm
    process->start();
    if(!process->waitForStarted())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical,
                                   ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(exeInfo.absoluteFilePath()))));
        delete process; // Clear finished process handle from heap
        return false;
    }

    // Return success
    return true;
}

ErrorCode waitOnProcess(QString processName, int graceSecs)
{
    // Wait until secure player has stopped running for grace period
    DWORD spProcessID;
    do
    {
        // Yield for grace period
        logEvent(LOG_EVENT_WAIT_GRACE.arg(processName));
        if(graceSecs > 0)
            QThread::sleep(graceSecs);

        // Find process ID by name
        spProcessID = Qx::getProcessIDByName(processName);

        // Check that process was found (is running)
        if(spProcessID)
        {
            logEvent(LOG_EVENT_WAIT_RUNNING.arg(processName));

            // Get process handle and see if it is valid
            HANDLE batchProcessHandle;
            if((batchProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, spProcessID)) == NULL)
            {
                postError(Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HANDLED_P.arg(processName),
                                           WRN_WAIT_PROCESS_NOT_HANDLED_S));
                return WAIT_PROCESS_NOT_HANDLED;
            }

            // Attempt to wait on process to terminate
            logEvent(LOG_EVENT_WAIT_ON.arg(processName));
            DWORD waitError = WaitForSingleObject(batchProcessHandle, INFINITE);

            // Close handle to process
            CloseHandle(batchProcessHandle);

            if(waitError != WAIT_OBJECT_0)
            {
                postError(Qx::GenericError(Qx::GenericError::Warning, WRN_WAIT_PROCESS_NOT_HOOKED_P.arg(processName),
                                           WRN_WAIT_PROCESS_NOT_HOOKED_S));
                return WAIT_PROCESS_NOT_HOOKED;
            }
            logEvent(LOG_EVENT_WAIT_QUIT.arg(processName));
        }
    }
    while(spProcessID);

    // Return success
    logEvent(LOG_EVENT_WAIT_FINISHED.arg(processName));
    return NO_ERR;
}


void cleanup(FP::Install& fpInstall, QList<QProcess*>& childProcesses)
{
    // Close database connection if open
    if(fpInstall.databaseConnectionOpenInThisThread())
        fpInstall.closeThreadedDatabaseConnection();

    // Close each remaining child process
    for(QProcess* childProcess : childProcesses)
    {
        childProcess->close();
        logProcessEnd(childProcess, ProcessType::Deferred);
        delete childProcess;
    }
}

//-Helper Functions-----------------------------------------------------------------
QString getRawCommandLineParams()
{
    // Remove application path, based on WINE
    // https://github.com/wine-mirror/wine/blob/a10267172046fc16aaa1cd1237701f6867b92fc0/dlls/shcore/main.c#L259
    const wchar_t *rawCL = GetCommandLineW();
    if (*rawCL == '"')
    {
        ++rawCL;
        while (*rawCL)
            if (*rawCL++ == '"')
                break;
    }
    else
        while (*rawCL && *rawCL != ' ' && *rawCL != '\t')
            ++rawCL;

    // Remove spaces before first arg
    while (*rawCL == ' ' || *rawCL == '\t')
        ++rawCL;

    // Return cropped string
    return QString::fromStdWString(std::wstring(rawCL));
}

ErrorCode randomlySelectID(QUuid& idBuffer, FP::Install& fpInstall)
{
    logEvent(LOG_EVENT_SEL_RAND);

    // Reset buffer
    idBuffer = QUuid();

    // SQL Error tracker
    QSqlError searchError;

    // Query all main games
    FP::Install::DBQueryBuffer mainGameIDQuery;
    searchError = fpInstall.queryAllGameIDs(mainGameIDQuery);
    if(searchError.isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return SQL_ERROR;
    }

    // Query all main additional apps
    FP::Install::DBQueryBuffer mainAddAppIDQuery;
    searchError = fpInstall.queryAllMainAddAppIDs(mainAddAppIDQuery);
    if(searchError.isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return SQL_ERROR;
    }

    QList<QUuid> playableIDs;

    // Enumerate main game IDs
    for(int i = 0; i < mainGameIDQuery.size; i++)
    {
        // Go to next record
        mainGameIDQuery.result.next();

        // Add ID to list
        QString gameIDString = mainGameIDQuery.result.value(FP::Install::DBTable_Game::COL_ID).toString();
        QUuid gameID = QUuid(gameIDString);
        if(!gameID.isNull())
            playableIDs.append(gameID);
        else
            logError(Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(gameIDString)));
    }

    // Enumerate main additional app IDs
    for(int i = 0; i < mainAddAppIDQuery.size; i++)
    {
        // Go to next record
        mainAddAppIDQuery.result.next();

        // Create ID and add if valid (should always be)
        QString addAppIDString = mainAddAppIDQuery.result.value(FP::Install::DBTable_Add_App::COL_ID).toString();
        QUuid addAppID = QUuid(addAppIDString);
        if(!addAppID.isNull())
            playableIDs.append(addAppID);
        else
            logError(Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(addAppIDString)));
    }
    logEvent(LOG_EVENT_PLAYABLE_COUNT.arg(QLocale(QLocale::system()).toString(playableIDs.size())));

    // Set buffer to random ID
    idBuffer = playableIDs.value(QRandomGenerator::global()->bounded(playableIDs.size() - 1));
    logEvent(LOG_EVENT_RAND_ID.arg(idBuffer.toString(QUuid::WithoutBraces)));

    // Return success
    return NO_ERR;
}

Qx::GenericError appInvolvesSecurePlayer(bool& involvesBuffer, QFileInfo appInfo)
{
    // Reset buffer
    involvesBuffer = false;

    if(appInfo.fileName().contains(SECURE_PLAYER_INFO.baseName()))
    {
        involvesBuffer = true;
        return Qx::GenericError();
    }

    else if(appInfo.suffix() == BAT_SUFX)
    {
        // Check if bat uses secure player
        QFile batFile(appInfo.absoluteFilePath());
        Qx::IOOpReport readReport = Qx::fileContainsString(involvesBuffer, batFile, SECURE_PLAYER_INFO.baseName());

        // Check for read errors
        if(!readReport.wasSuccessful())
            return Qx::GenericError(Qx::GenericError::Critical, readReport.getOutcome(), readReport.getOutcomeInfo());
        else
            return Qx::GenericError();
    }
    else
        return Qx::GenericError();
}

QString escapeNativeArgsForCMD(QString nativeArgs)
{
    static const QSet<QChar> escapeChars{'^','&','<','>','|'};
    QString escapedNativeArgs;
    bool inQuotes = false;

    for(int i = 0; i < nativeArgs.size(); i++)
    {
        const QChar& chr = nativeArgs.at(i);
        if(chr== '"' && (inQuotes || i != nativeArgs.lastIndexOf('"')))
            inQuotes = !inQuotes;

        escapedNativeArgs.append((!inQuotes && escapeChars.contains(chr)) ? '^' + chr : chr);
    }

    if(nativeArgs != escapedNativeArgs)
        logEvent(LOG_EVENT_ARGS_ESCAPED.arg(nativeArgs, escapedNativeArgs));

    return escapedNativeArgs;
}

void postError(Qx::GenericError error, bool log)
{
    // Logging
    if(log)
        logError(error);

    // Show error if applicable
    if(gErrorVerbosity == ErrorVerbosity::Full ||
       (gErrorVerbosity == ErrorVerbosity::Quiet && error.errorLevel() == Qx::GenericError::Critical))
        error.exec(QMessageBox::Ok);
}

void logEvent(QString event) { gLogger->appendGeneralEvent(event); }

void logAppTask(const AppTask& appTask)
{
    QString eventStr = LOG_EVENT_APP_TASK
            .arg(ENUM_NAME(appTask.type))
            .arg(appTask.path)
            .arg(appTask.filename)
            .arg(appTask.param.join(R"(", ")"))
            .arg(appTask.nativeParam)
            .arg(ENUM_NAME(appTask.processType));

    logEvent(eventStr);
}

void logProcessStart(const QProcess* process, ProcessType type)
{
    QString eventStr = process->program();
    if(!process->arguments().isEmpty())
        eventStr += " " + process->arguments().join(" ");
    if(!process->nativeArguments().isEmpty())
        eventStr += " " + process->nativeArguments();

    logEvent(LOG_EVENT_START_PROCESS.arg(ENUM_NAME(type), eventStr));
}

void logProcessEnd(const QProcess* process, ProcessType type)
{
    logEvent(LOG_EVENT_END_PROCESS.arg(ENUM_NAME(type), process->program()));
}

void logError(Qx::GenericError error)
{
    gLogger->appendErrorEvent(error);

    if(error.errorLevel() == Qx::GenericError::Critical)
        gCritErrOccured = true;
}

int printLogAndExit(int exitCode)
{
    if(gCritErrOccured)
        logEvent(LOG_ERR_CRITICAL);

    Qx::IOOpReport logPrintReport = gLogger->finish(exitCode);
    if(!logPrintReport.wasSuccessful())
        postError(Qx::GenericError(Qx::GenericError::Warning, logPrintReport.getOutcome(), logPrintReport.getOutcomeInfo()), false);

    // Return exit code so main function can return with this one
    return exitCode;
}

