#include "version.h"
#include <QApplication>
#include <QProcess>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QProgressDialog>
#include <queue>
#include "qx-windows.h"
#include "qx-io.h"
#include "qx-net.h"
#include "qx-widgets.h"
#include "flashpoint-install.h"
#include "logger.h"
#include "magic_enum.hpp"
#include "errors.h"

#define CLIFP_DIR_PATH QCoreApplication::applicationDirPath()
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
    CANT_PARSE_PREF = 0x06,
    CANT_PARSE_SERVICES = 0x07,
    CONFIG_SERVER_MISSING = 0x08,
    ID_NOT_VALID = 0x09,
    RAND_FILTER_NOT_VALID = 0x0A,
    SQL_ERROR = 0x0B,
    SQL_MISMATCH = 0x0C,
    DB_MISSING_TABLES = 0x0D,
    DB_MISSING_COLUMNS = 0x0E,
    ID_NOT_FOUND = 0x0F,
    MORE_THAN_ONE_AUTO = 0x10,
    EXTRA_NOT_FOUND = 0x11,
    EXECUTABLE_NOT_FOUND = 0x12,
    EXECUTABLE_NOT_VALID = 0x13,
    PROCESS_START_FAIL = 0x14,
    WAIT_PROCESS_NOT_HANDLED = 0x15,
    WAIT_PROCESS_NOT_HOOKED = 0x16,
    CANT_READ_BAT_FILE = 0x17,
    PARENT_INVALID = 0x18,
    CANT_OBTAIN_DATA_PACK = 0x19,
    DATA_PACK_INVALID = 0x20
};

enum class OperationMode { Invalid, Direct, Auto, Random, Prepare, Message, Extra, Information };
enum class TaskStage { Startup, Primary, Auxiliary, Shutdown };
enum class ProcessType { Blocking, Deferred, Detached };
enum class NotificationVerbosity { Full, Quiet, Silent };

//-Structs---------------------------------------------------------------------
struct Task
{
    TaskStage stage;

    virtual QString name() const = 0;
    virtual QStringList members() const { return {".stage = " + ENUM_NAME(stage)}; }
};

struct ExecTask : public Task
{
    QString path;
    QString filename;
    QStringList param;
    QString nativeParam;
    ProcessType processType;

    QString name() const { return "ExecTask"; }
    QStringList members() const
    {
        QStringList ml = Task::members();
        ml << ".path = \"" << path << "\"";
        ml << ".filename = \"" << filename << "\"";
        ml << ".param = {\"" << param.join(R"(", ")") << "\"}";
        ml << ".nativeParam = \"" << nativeParam << "\"";
        ml << ".processType = " << ENUM_NAME(processType);
        return ml;
    }
};

struct MessageTask : public Task
{
    QString message;
    bool modal;

    QString name() const { return "MessageTask"; }
    QStringList members() const
    {
        QStringList ml = Task::members();
        ml << ".message = \"" << message << "\"";
        ml << ".modal = \"" << (modal ? "true" : "false");
        return ml;
    }
};

struct ExtraTask : public Task
{
    QDir dir;

    QString name() const { return "ExtraTask"; }
    QStringList members() const
    {
        QStringList ml = Task::members();
        ml << ".extraDir = \"" << dir.path() << "\"";
        return ml;
    }
};

struct WaitTask : public Task
{
    QString processName;

    QString name() const { return "WaitTask"; }
    QStringList members() const
    {
        QStringList ml = Task::members();
        ml << ".processName = \"" << processName << "\"";
        return ml;
    }
};

struct DownloadTask : public Task
{
    QString destPath;
    QString destFileName;
    QUrl targetFile;
    QString sha256;

    QString name() const { return "DownloadTask"; }
    QStringList members() const
    {
        QStringList ml = Task::members();
        ml << ".destPath = \"" << destPath << "\"";
        ml << ".destFileName = \"" << destFileName << "\"";
        ml << ".targetFile = \"" << targetFile.toString() << "\"";
        ml << ".sha256 = " << sha256;
        return ml;
    }
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
const QString CL_OPT_RAND_DESC = "Selects a random game UUID from the database and starts it in the same manner as using the --" + CL_OPT_AUTO_L_NAME + " switch. Required value options for this argument (filters): all/any, game/arcade, animation/theatre";

const QString CL_OPT_PREP_S_NAME = "i";
const QString CL_OPT_PREP_L_NAME = "prepare";
const QString CL_OPT_PREP_DESC = "Initializes Flashpoint for playing the provided Data Pack based title by UUID. If the title does not use a Data Pack this option has no effect.";

const QString CL_OPT_QUIET_S_NAME = "q";
const QString CL_OPT_QUIET_L_NAME = "quiet";
const QString CL_OPT_QUIET_DESC = "Silences all non-critical messages.";

const QString CL_OPT_SILENT_S_NAME = "s";
const QString CL_OPT_SILENT_L_NAME = "silent";
const QString CL_OPT_SILENT_DESC = "Silences all messages (takes precedence over quiet mode).";

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
        "<b>-" + CL_OPT_PREP_S_NAME + " | --" + CL_OPT_PREP_L_NAME + ":</b> &nbsp;" + CL_OPT_PREP_DESC + "<br>"
        "<b>-" + CL_OPT_MSG_S_NAME + " | --" + CL_OPT_MSG_L_NAME + ":</b> &nbsp;" + CL_OPT_MSG_DESC + "<br>"
        "<b>-" + CL_OPT_EXTRA_S_NAME + " | --" + CL_OPT_EXTRA_L_NAME + ":</b> &nbsp;" + CL_OPT_EXTRA_DESC + "<br>"
        "<b>-" + CL_OPT_QUIET_S_NAME + " | --" + CL_OPT_QUIET_L_NAME + ":</b> &nbsp;" + CL_OPT_QUIET_DESC + "<br>"
        "<b>-" + CL_OPT_SILENT_S_NAME + " | --" + CL_OPT_SILENT_L_NAME + ":</b> &nbsp;" + CL_OPT_SILENT_DESC + "<br>"
        "Use <b>'" + CL_OPT_APP_L_NAME + "'</b> and <b>'" + CL_OPT_PARAM_L_NAME + "'</b> for direct operation, use <b>'" + CL_OPT_AUTO_L_NAME +
        "'</b> by itself for automatic operation, use <b>'" + CL_OPT_MSG_L_NAME  + "'</b> by itself for random operation,  use <b>'" + CL_OPT_MSG_L_NAME  +
        "'</b> to display a popup message, use <b>'" + CL_OPT_EXTRA_L_NAME + "'</b> to view an extra, use <b>'" + CL_OPT_PREP_L_NAME +
        "'</b> to prepare a Data Pack game, or use <b>'" + CL_OPT_HELP_L_NAME + "'</b> and/or <b>'" + CL_OPT_VERSION_L_NAME + "'</b> for information.";

const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " VER_FILEVERSION_STR ", designed for use with BlueMaxima's Flashpoint " VER_PRODUCTVERSION_STR "+";

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
const QString ERR_SQL_MISMATCH = "Received a different form of result from an SQL query than expected.";
const QString ERR_DB_MISSING_TABLE = "The Flashpoint database is missing expected tables.";
const QString ERR_DB_TABLE_MISSING_COLUMN = "The Flashpoint database tables are missing expected columns.";
const QString ERR_ID_NOT_FOUND = "An entry matching the specified ID could not be found in the Flashpoint database.";
const QString ERR_ID_INVALID = "The provided string was not a valid GUID/UUID.";
const QString ERR_RAND_FILTER_INVALID = "The provided string for random operation was not a valid filter.";
const QString ERR_MORE_THAN_ONE_AUTO_P = "Multiple entries with the specified auto ID were found.";
const QString ERR_MORE_THAN_ONE_AUTO_S = "This should not be possible and may indicate an error within the Flashpoint database";
const QString ERR_PARENT_INVALID = "The parent ID of the target additional app was not valid.";
const QString WRN_EXIST_PACK_SUM_MISMATCH = "The existing Data Pack of the selected title does not contain the data expected. It will be re-downloaded.";

// Error Messages - Execution
const QString ERR_EXTRA_NOT_FOUND = "The extra %1 does not exist!";
const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";
const QString ERR_PACK_SUM_MISMATCH = "The title's Data Pack checksum does not match its record!";
const QString WRN_WAIT_PROCESS_NOT_HANDLED_P  = "Could not get a wait handle to %1.";
const QString WRN_WAIT_PROCESS_NOT_HANDLED_S = "The title may not work correctly";
const QString WRN_WAIT_PROCESS_NOT_HOOKED_P  = "Could not hook %1 for waiting.";
const QString WRN_WAIT_PROCESS_NOT_HOOKED_S = "The title may not work correctly";

// CLI Options
const QCommandLineOption CL_OPTION_APP({CL_OPT_APP_S_NAME, CL_OPT_APP_L_NAME}, CL_OPT_APP_DESC, "application"); // Takes value
const QCommandLineOption CL_OPTION_PARAM({CL_OPT_PARAM_S_NAME, CL_OPT_PARAM_L_NAME}, CL_OPT_PARAM_DESC, "parameters"); // Takes value
const QCommandLineOption CL_OPTION_AUTO({CL_OPT_AUTO_S_NAME, CL_OPT_AUTO_L_NAME}, CL_OPT_AUTO_DESC, "id"); // Takes value
const QCommandLineOption CL_OPTION_RAND({CL_OPT_RAND_S_NAME, CL_OPT_RAND_L_NAME}, CL_OPT_RAND_DESC, "random"); // Takes value
const QCommandLineOption CL_OPTION_PREP({CL_OPT_PREP_S_NAME, CL_OPT_PREP_L_NAME}, CL_OPT_PREP_DESC, "prep"); // Takes value
const QCommandLineOption CL_OPTION_MSG({CL_OPT_MSG_S_NAME, CL_OPT_MSG_L_NAME}, CL_OPT_MSG_DESC, "message"); // Takes value
const QCommandLineOption CL_OPTION_EXTRA({CL_OPT_EXTRA_S_NAME, CL_OPT_EXTRA_L_NAME}, CL_OPT_EXTRA_DESC, "extra"); // Takes value
const QCommandLineOption CL_OPTION_HELP({CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC); // Boolean option
const QCommandLineOption CL_OPTION_VERSION({CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC); // Boolean option
const QCommandLineOption CL_OPTION_QUIET({CL_OPT_QUIET_S_NAME, CL_OPT_QUIET_L_NAME}, CL_OPT_QUIET_DESC); // Boolean option
const QCommandLineOption CL_OPTION_SILENT({CL_OPT_SILENT_S_NAME, CL_OPT_SILENT_L_NAME}, CL_OPT_SILENT_DESC); // Boolean option
const QList<const QCommandLineOption*> CL_OPTIONS_MAIN{&CL_OPTION_APP, &CL_OPTION_PARAM, &CL_OPTION_AUTO, &CL_OPTION_MSG,
                                                       &CL_OPTION_EXTRA, &CL_OPTION_HELP, &CL_OPTION_VERSION, &CL_OPTION_RAND, &CL_OPTION_PREP};
const QList<const QCommandLineOption*> CL_OPTIONS_ALL = CL_OPTIONS_MAIN + QList<const QCommandLineOption*>{&CL_OPTION_QUIET, &CL_OPTION_SILENT};

// CLI Option Operation Mode Map TODO: Submit a patch for Qt6 to make QCommandLineOption directly hashable (implement == and qHash)
const QHash<QSet<QString>, OperationMode> CL_MAIN_OPTIONS_OP_MODE_MAP{
    {{CL_OPT_APP_S_NAME, CL_OPT_PARAM_S_NAME}, OperationMode::Direct},
    {{CL_OPT_AUTO_S_NAME}, OperationMode::Auto},
    {{CL_OPT_RAND_S_NAME}, OperationMode::Random},
    {{CL_OPT_PREP_S_NAME}, OperationMode::Prepare},
    {{CL_OPT_MSG_S_NAME}, OperationMode::Message},
    {{CL_OPT_EXTRA_S_NAME}, OperationMode::Extra},
    {{CL_OPT_HELP_S_NAME}, OperationMode::Information},
    {{CL_OPT_VERSION_S_NAME}, OperationMode::Information},
    {{CL_OPT_HELP_S_NAME, CL_OPT_VERSION_S_NAME}, OperationMode::Information}
};

// Random Filter Sets
const QSet<QString> RAND_ALL_FILTER_SET{"all", "any"};
const QSet<QString> RAND_GAME_FILTER_SET{"game", "arcade"};
const QSet<QString> RAND_ANIM_FILTER_SET{"animation", "theatre"};

// Random Selection Info
const QString RAND_SEL_INFO =
        "<b>Randomly Selected Game</b><br>"
        "<br>"
        "<b>Title:</b> %1<br>"
        "<b>Developer:</b> %2<br>"
        "<b>Publisher:</b> %3<br>"
        "<b>Library:</b> %4<br>"
        "<b>Variant:</b> %5<br>";

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
const QString LOG_WRN_PREP_NOT_DATA_PACK = "The provided ID does not belong to a Data Pack based game (%1). No action will be taken.";
const QString LOG_EVENT_FLASHPOINT_SEARCH = "Searching for Flashpoint root...";
const QString LOG_EVENT_FLASHPOINT_ROOT_CHECK = "Checking if \"%1\" is flashpoint root";
const QString LOG_EVENT_FLASHPOINT_LINK = "Linked to Flashpoint install at: %1";
const QString LOG_EVENT_OP_MODE = "Operation Mode: %1";
const QString LOG_EVENT_TASK_ENQ = "Enqueued %1: {%2}";
const QString LOG_EVENT_SHOW_MESSAGE = "Displayed message";
const QString LOG_EVENT_SHOW_EXTRA = "Opened folder of extra %1";
const QString LOG_EVENT_HELP_SHOWN = "Displayed help information";
const QString LOG_EVENT_VER_SHOWN = "Displayed version information";
const QString LOG_EVENT_INIT = "Initializing CLIFp...";
const QString LOG_EVENT_GET_SET = "Reading Flashpoint configuration...";
const QString LOG_EVENT_SEL_RAND = "Selecting a playable title at random...";
const QString LOG_EVENT_INIT_RAND_ID = "Randomly chose primary title is \"%1\"";
const QString LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT = "Chosen title has %1 playable additional-apps";
const QString LOG_EVENT_RAND_DET_PRIM = "Selected primary title";
const QString LOG_EVENT_RAND_DET_ADD_APP = "Selected additional-app \"%1\"";
const QString LOG_EVENT_RAND_GET_INFO = "Querying random game info...";
const QString LOG_EVENT_PLAYABLE_COUNT = "Found %1 playable primary titles";
const QString LOG_EVENT_DATA_PACK_TITLE = "Selected title uses a data pack";
const QString LOG_EVENT_ENQ_START = "Enqueuing startup tasks...";
const QString LOG_EVENT_ENQ_AUTO = "Enqueuing automatic tasks...";
const QString LOG_EVENT_ENQ_DATA_PACK = "Enqueuing Data Pack tasks...";
const QString LOG_EVENT_ENQ_STOP = "Enqueuing shutdown tasks...";
const QString LOG_EVENT_ID_MATCH_TITLE = "Auto ID matches main title: %1";
const QString LOG_EVENT_ID_MATCH_ADDAPP = "Auto ID matches additional app: %1";
const QString LOG_EVENT_QUEUE_CLEARED = "Previous queue entries cleared due to auto task being a Message/Extra";
const QString LOG_EVENT_FOUND_AUTORUN = "Found autorun-before additional app: %1";
const QString LOG_EVENT_DATA_PACK_MISS = "Title Data Pack is not available locally";
const QString LOG_EVENT_DATA_PACK_FOUND = "Title Data Pack with correct hash is already present, no need to download";
const QString LOG_EVENT_QUEUE_START = "Processing App Task queue";
const QString LOG_EVENT_TASK_START = "Handling task %1 (%2)";
const QString LOG_EVENT_TASK_FINISH = "End of task %1";
const QString LOG_EVENT_TASK_FINISH_ERR = "Premature end of task %1";
const QString LOG_EVENT_QUEUE_FINISH = "Finished processing App Task queue";
const QString LOG_EVENT_CLEANUP_FINISH = "Finished cleanup";
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
const QString LOG_EVENT_DOWNLOADING_DATA_PACK = "Downloading Data Pack %1";
const QString LOG_EVENT_DOWNLOAD_AUTH = "Authentication required to download Data Pack, requesting credentials...";
const QString LOG_EVENT_DOWNLOAD_SUCC = "Data Pack downloaded successfully";

// Globals
QApplication* gAppInstancePtr;
NotificationVerbosity gNotificationVerbosity = NotificationVerbosity::Full;
std::unique_ptr<Logger> gLogger;
bool gCritErrOccurred = false;

// Prototypes - Process
ErrorCode openAndVerifyProperDatabase(FP::Install& fpInstall);
ErrorCode enqueueStartupTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QString fpRoot, FP::Install::Config fpConfig, FP::Install::Services fpServices);
ErrorCode enqueueAutomaticTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QUuid targetID, FP::Install& fpInstall);
ErrorCode enqueueAdditionalApp(std::queue<std::shared_ptr<Task>>& taskQueue, FP::Install::DBQueryBuffer addAppResult, TaskStage taskStage, const FP::Install& fpInstall);
void enqueueShutdownTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QString fpRoot, FP::Install::Services fpServices);
ErrorCode enqueueConditionalWaitTask(std::queue<std::shared_ptr<Task>>& taskQueue, QFileInfo precedingAppInfo);
ErrorCode enqueueDataPackTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QUuid targetID, const FP::Install& fpInstall);
ErrorCode processTaskQueue(std::queue<std::shared_ptr<Task>>& taskQueue, QList<QProcess*>& childProcesses);
void handleExecutionError(std::queue<std::shared_ptr<Task>>& taskQueue, int taskNum, ErrorCode& currentError, ErrorCode newError);
bool cleanStartProcess(QProcess* process, QFileInfo exeInfo);
ErrorCode waitOnProcess(QString processName, int graceSecs);
void cleanup(FP::Install& fpInstall, QList<QProcess*>& childProcesses);

// Prototypes - Helper
QString getRawCommandLineParams();
QString findFlashpointRoot();
ErrorCode randomlySelectID(QUuid& mainIDBuffer, QUuid& subIDBuffer, FP::Install& fpInstall, FP::Install::LibraryFilter lbFilter);
ErrorCode getRandomSelectionInfo(QString& infoBuffer, FP::Install& fpInstall, QUuid mainID, QUuid subID);
Qx::GenericError appInvolvesSecurePlayer(bool& involvesBuffer, QFileInfo appInfo);
QString escapeNativeArgsForCMD(QString nativeArgs);
int postError(Qx::GenericError error, bool log = true, QMessageBox::StandardButtons bs = QMessageBox::Ok, QMessageBox::StandardButton def = QMessageBox::NoButton);
void logEvent(QString event);
void logTask(const Task* const task);
void logProcessStart(const QProcess* process, ProcessType type);
void logProcessEnd(const QProcess* process, ProcessType type);
void logError(Qx::GenericError error);
int logFinish(int exitCode);

int main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // QApplication Object
    QApplication app(argc, argv);
    gAppInstancePtr = &app;

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
    gNotificationVerbosity = clParser.isSet(CL_OPTION_SILENT) ? NotificationVerbosity::Silent :
                      clParser.isSet(CL_OPTION_QUIET) ? NotificationVerbosity::Quiet : NotificationVerbosity::Full;

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
    QFile logFile(CLIFP_DIR_PATH + '/' + LOG_FILE_NAME);
    gLogger = std::make_unique<Logger>(&logFile, rawCL, interpCL, LOG_HEADER, LOG_MAX_ENTRIES);

    // Open log
    Qx::IOOpReport logOpen = gLogger->openLog();
    if(!logOpen.wasSuccessful())
        postError(Qx::GenericError(Qx::GenericError::Warning, logOpen.getOutcome(), logOpen.getOutcomeInfo()), false);

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
        return logFinish(NO_ERR);
    }
    else if(operationMode == OperationMode::Invalid)
    {
        QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);
        logError(Qx::GenericError(Qx::GenericError::Error, LOG_ERR_INVALID_PARAM));
        return logFinish(INVALID_ARGS);
    }

    logEvent(LOG_EVENT_INIT);

    //-Restrict app to only one instance---------------------------------------------------
    if(!Qx::enforceSingleInstance())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ALREADY_OPEN));
        return logFinish(ALREADY_OPEN);
    }

    // Ensure Flashpoint Launcher isn't running
    if(Qx::processIsRunning(QFileInfo(FP::Install::LAUNCHER_PATH).fileName()))
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_LAUNCHER_RUNNING_P, ERR_LAUNCHER_RUNNING_S));
        return logFinish(LAUNCHER_OPEN);
    }

    //-Find and link to Flashpoint Install----------------------------------------------------------
    QString fpRoot;
    logEvent(LOG_EVENT_FLASHPOINT_SEARCH);

    if((fpRoot = findFlashpointRoot()).isNull())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_INSTALL_INVALID_P, ERR_INSTALL_INVALID_S));
        return logFinish(INSTALL_INVALID);
    }

    FP::Install flashpointInstall(fpRoot, CLIFP_DIR_PATH.remove(fpRoot).remove(1, 1));
    logEvent(LOG_EVENT_FLASHPOINT_LINK.arg(fpRoot));

    //-Get Install Settings----------------------------------------------------------------
    logEvent(LOG_EVENT_GET_SET);
    Qx::GenericError settingsReadError;
    FP::Install::Config flashpointConfig;
    FP::Install::Services flashpointServices;

    if((settingsReadError = flashpointInstall.getConfig(flashpointConfig)).isValid())
    {
        postError(settingsReadError);
        return logFinish(CANT_PARSE_CONFIG);
    }

    if((settingsReadError = flashpointInstall.getServices(flashpointServices)).isValid())
    {
        postError(settingsReadError);
        return logFinish(CANT_PARSE_SERVICES);
    }

    //-Proccess Tracking-------------------------------------------------------------------
    QList<QProcess*> activeChildProcesses;
    std::queue<std::shared_ptr<Task>> taskQueue;

    //-Handle Mode Specific Operations-----------------------------------------------------
    ErrorCode enqueueError;
    QSqlError sqlError;
    QFileInfo inputInfo;
    QUuid autoID;
    QUuid secondaryID;
    QString rawRandFilter;
    QString selectionInfo;
    FP::Install::LibraryFilter randFilter;
    std::shared_ptr<ExecTask> directTask;

    switch(operationMode)
    {
        case OperationMode::Invalid:
            // Already handled
            break;

        case OperationMode::Direct:
            if((enqueueError = enqueueStartupTasks(taskQueue, fpRoot, flashpointConfig, flashpointServices)))
                return logFinish(enqueueError);

            inputInfo = QFileInfo(fpRoot + '/' + clParser.value(CL_OPTION_APP));

            directTask = std::make_shared<ExecTask>();
            directTask->stage = TaskStage::Primary;
            directTask->path = inputInfo.absolutePath();
            directTask->filename = inputInfo.fileName();
            directTask->param = QStringList();
            directTask->nativeParam = clParser.value(CL_OPTION_PARAM);
            directTask->processType = ProcessType::Blocking;

            taskQueue.push(directTask);
            logTask(directTask.get());

            // Add wait task if required
            if((enqueueError = enqueueConditionalWaitTask(taskQueue, inputInfo)))
                return logFinish(enqueueError);
            break;
        case OperationMode::Random:
            rawRandFilter = clParser.value(CL_OPTION_RAND);

            // Check for valid filter
            if(RAND_ALL_FILTER_SET.contains(rawRandFilter))
                randFilter = FP::Install::LibraryFilter::Either;
            else if(RAND_GAME_FILTER_SET.contains(rawRandFilter))
                randFilter = FP::Install::LibraryFilter::Game;
            else if(RAND_ANIM_FILTER_SET.contains(rawRandFilter))
                randFilter = FP::Install::LibraryFilter::Anim;
            else
            {
                postError(Qx::GenericError(Qx::GenericError::Critical, ERR_RAND_FILTER_INVALID));
                return logFinish(ErrorCode::RAND_FILTER_NOT_VALID);
            }

            if((enqueueError = openAndVerifyProperDatabase(flashpointInstall)))
                return logFinish(enqueueError);

            if((enqueueError = randomlySelectID(autoID, secondaryID, flashpointInstall, randFilter)))
                return logFinish(enqueueError);

            if((enqueueError = enqueueStartupTasks(taskQueue, fpRoot, flashpointConfig, flashpointServices)))
                return logFinish(enqueueError);

            if((enqueueError = enqueueAutomaticTasks(taskQueue, secondaryID.isNull() ? autoID : secondaryID, flashpointInstall)))
                return logFinish(enqueueError);

            // Get selection info if not suppressed
            if(gNotificationVerbosity == NotificationVerbosity::Full)
            {
                if((enqueueError = getRandomSelectionInfo(selectionInfo, flashpointInstall, autoID, secondaryID)))
                    return logFinish(enqueueError);

                // Display selection info
                QMessageBox::information(nullptr, QApplication::applicationName(), selectionInfo);
            }
            break;

        case OperationMode::Auto:
            if((autoID = QUuid(clParser.value(CL_OPTION_AUTO))).isNull())
            {
                postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_INVALID));
                return logFinish(ID_NOT_VALID);
            }

            if((enqueueError = openAndVerifyProperDatabase(flashpointInstall)))
                return logFinish(enqueueError);

            if((enqueueError = enqueueStartupTasks(taskQueue, fpRoot, flashpointConfig, flashpointServices)))
                return logFinish(enqueueError);

            if((enqueueError = enqueueAutomaticTasks(taskQueue, autoID, flashpointInstall)))
                return logFinish(enqueueError);
            break;

        case OperationMode::Prepare:
            if((secondaryID = QUuid(clParser.value(CL_OPTION_AUTO))).isNull())
            {
                postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_INVALID));
                return logFinish(ID_NOT_VALID);
            }

            if((enqueueError = openAndVerifyProperDatabase(flashpointInstall)))
                return logFinish(enqueueError);

            bool titleUsesDataPack;
            if((sqlError = flashpointInstall.entryUsesDataPack(titleUsesDataPack, secondaryID)).isValid())
            {
                postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, sqlError.text()));
                return logFinish(SQL_ERROR);
            }

            if(titleUsesDataPack)
            {
                if((enqueueError = enqueueDataPackTasks(taskQueue, secondaryID, flashpointInstall)))
                    return logFinish(enqueueError);
            }
            else
            {
                logError(Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_PREP_NOT_DATA_PACK.arg(secondaryID.toString(QUuid::WithoutBraces))));
                return logFinish(NO_ERR);
            }
            break;

        case OperationMode::Message:
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), clParser.value(CL_OPTION_MSG));
            logEvent(LOG_EVENT_SHOW_MESSAGE);
            return logFinish(NO_ERR);

        case OperationMode::Extra:
            // Ensure extra exists
            inputInfo = QFileInfo(flashpointInstall.getExtrasDirectory().absolutePath() + '/' + clParser.value(CL_OPTION_EXTRA));
            if(!inputInfo.exists() || !inputInfo.isDir())
            {
                postError(Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(inputInfo.fileName())));
                return logFinish(EXTRA_NOT_FOUND);
            }

            // Open extra
            QDesktopServices::openUrl(QUrl::fromLocalFile(inputInfo.absoluteFilePath()));
            logEvent(LOG_EVENT_SHOW_EXTRA.arg(inputInfo.fileName()));
            return logFinish(NO_ERR);

        case OperationMode::Information:
            // Already handled
            break;
    }

    // Enqueue Shudown Tasks if main task isn't message/extra/prep
    if(taskQueue.size() != 1 ||
       (!std::dynamic_pointer_cast<MessageTask>(taskQueue.front()) &&
       !std::dynamic_pointer_cast<ExtraTask>(taskQueue.front()) &&
       operationMode != OperationMode::Prepare))
        enqueueShutdownTasks(taskQueue, fpRoot, flashpointServices);

    // Process app task queue
    ErrorCode executionError = processTaskQueue(taskQueue, activeChildProcesses);
    logEvent(LOG_EVENT_QUEUE_FINISH);

    // Cleanup
    cleanup(flashpointInstall, activeChildProcesses);
    logEvent(LOG_EVENT_CLEANUP_FINISH);

    // Return error status
    return logFinish(executionError);
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

ErrorCode enqueueStartupTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QString fpRoot, FP::Install::Config fpConfig, FP::Install::Services fpServices)
{
    logEvent(LOG_EVENT_ENQ_START);
    // Add Start entries from services
    for(const FP::Install::StartStop& startEntry : fpServices.starts)
    {        
        std::shared_ptr<ExecTask> currentTask = std::make_shared<ExecTask>();
        currentTask->stage = TaskStage::Startup;
        currentTask->path = fpRoot + '/' + startEntry.path;
        currentTask->filename = startEntry.filename;
        currentTask->param = startEntry.arguments;
        currentTask->nativeParam = QString();
        currentTask->processType = ProcessType::Blocking;

        taskQueue.push(currentTask);
        logTask(currentTask.get());
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

        std::shared_ptr<ExecTask> serverTask = std::make_shared<ExecTask>();
        serverTask->stage = TaskStage::Startup;
        serverTask->path = fpRoot + '/' + configuredServer.path;
        serverTask->filename = configuredServer.filename;
        serverTask->param = configuredServer.arguments;
        serverTask->nativeParam = QString();
        serverTask->processType = configuredServer.kill ? ProcessType::Deferred : ProcessType::Detached;

        taskQueue.push(serverTask);
        logTask(serverTask.get());
    }

    // Add Daemon entry from services
    QHash<QString, FP::Install::ServerDaemon>::const_iterator daemonIt;
    for (daemonIt = fpServices.daemons.constBegin(); daemonIt != fpServices.daemons.constEnd(); ++daemonIt)
    {
        std::shared_ptr<ExecTask> currentTask = std::make_shared<ExecTask>();
        currentTask->stage = TaskStage::Startup;
        currentTask->path = fpRoot + '/' + daemonIt.value().path;
        currentTask->filename = daemonIt.value().filename;
        currentTask->param = daemonIt.value().arguments;
        currentTask->nativeParam = QString();
        currentTask->processType = daemonIt.value().kill ? ProcessType::Deferred : ProcessType::Detached;

        taskQueue.push(currentTask);
        logTask(currentTask.get());
    }

    // Return success
    return NO_ERR;
}

ErrorCode enqueueAutomaticTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QUuid targetID, FP::Install& fpInstall)
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
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_NOT_FOUND));
        return ID_NOT_FOUND;
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

        // Clear queue if this entry is a message or extra
        QString appPath = searchResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
        if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE || appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
            taskQueue = {};

        logEvent(LOG_EVENT_QUEUE_CLEARED);

        // Check if parent entry uses a data pack
        QSqlError packCheckError;
        bool parentUsesDataPack;
        QUuid parentID = searchResult.result.value(FP::Install::DBTable_Add_App::COL_PARENT_ID).toString();

        if(parentID.isNull())
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_PARENT_INVALID, searchResult.result.value(FP::Install::DBTable_Add_App::COL_ID).toString()));
            return PARENT_INVALID;
        }

        if((packCheckError = fpInstall.entryUsesDataPack(parentUsesDataPack, parentID)).isValid())
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, packCheckError.text()));
            return SQL_ERROR;
        }

        if(parentUsesDataPack)
        {
            logEvent(LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = enqueueDataPackTasks(taskQueue, parentID, fpInstall);

            if(enqueueError)
                return enqueueError;
        }

        enqueueError= enqueueAdditionalApp(taskQueue, searchResult, TaskStage::Primary, fpInstall);

        if(enqueueError)
            return enqueueError;
    }
    else if(searchResult.source == FP::Install::DBTable_Game::NAME) // Get autorun additional apps if result is game
    {
        logEvent(LOG_EVENT_ID_MATCH_TITLE.arg(searchResult.result.value(FP::Install::DBTable_Game::COL_TITLE).toString()));

        // Check if entry uses a data pack
        QSqlError packCheckError;
        bool entryUsesDataPack;
        QUuid entryId = searchResult.result.value(FP::Install::DBTable_Game::COL_ID).toString();

        if((packCheckError = fpInstall.entryUsesDataPack(entryUsesDataPack, entryId)).isValid())
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, packCheckError.text()));
            return SQL_ERROR;
        }

        if(entryUsesDataPack)
        {
            logEvent(LOG_EVENT_DATA_PACK_TITLE);
            enqueueError = enqueueDataPackTasks(taskQueue, entryId, fpInstall);

            if(enqueueError)
                return enqueueError;
        }

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
                enqueueError = enqueueAdditionalApp(taskQueue, addAppSearchResult, TaskStage::Auxiliary, fpInstall);
                if(enqueueError)
                    return enqueueError;
            }
        }

        // Enqueue game
        QString gamePath = searchResult.result.value(FP::Install::DBTable_Game::COL_APP_PATH).toString();
        QString gameArgs = searchResult.result.value(FP::Install::DBTable_Game::COL_LAUNCH_COMMAND).toString();
        QFileInfo gameInfo(fpInstall.getPath() + '/' + gamePath);

        std::shared_ptr<ExecTask> gameTask = std::make_shared<ExecTask>();
        gameTask->stage = TaskStage::Primary;
        gameTask->path = gameInfo.absolutePath();
        gameTask->filename = gameInfo.fileName();
        gameTask->param = QStringList();
        gameTask->nativeParam = gameArgs;
        gameTask->processType = ProcessType::Blocking;

        taskQueue.push(gameTask);
        logTask(gameTask.get());

        // Add wait task if required
        if((enqueueError = enqueueConditionalWaitTask(taskQueue, gameInfo)))
            return enqueueError;
    }
    else
        throw std::runtime_error("Auto ID search result source must be 'game' or 'additional_app'");

    // Return success
    return NO_ERR;
}

ErrorCode enqueueAdditionalApp(std::queue<std::shared_ptr<Task>>& taskQueue, FP::Install::DBQueryBuffer addAppResult, TaskStage taskStage, const FP::Install& fpInstall)
{
    // Ensure query result is additional app
    assert(addAppResult.source == FP::Install::DBTable_Add_App::NAME);

    QString appPath = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
    QString appArgs = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_LAUNCH_COMMAND).toString();
    bool waitForExit = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_WAIT_EXIT).toInt() != 0;

    if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE)
    {
        std::shared_ptr<MessageTask> messageTask = std::make_shared<MessageTask>();
        messageTask->stage = taskStage;
        messageTask->message = appArgs;
        messageTask->modal = waitForExit || taskStage == TaskStage::Primary;

        taskQueue.push(messageTask);
        logTask(messageTask.get());
    }
    else if(appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
    {
        std::shared_ptr<ExtraTask> extraTask = std::make_shared<ExtraTask>();
        extraTask->stage = taskStage;
        extraTask->dir = QDir(fpInstall.getExtrasDirectory().absolutePath() + "/" + appArgs);

        taskQueue.push(extraTask);
        logTask(extraTask.get());
    }
    else
    {
        QFileInfo addAppInfo(fpInstall.getPath() + '/' + appPath);

        std::shared_ptr<ExecTask> addAppTask = std::make_shared<ExecTask>();
        addAppTask->stage = taskStage;
        addAppTask->path = addAppInfo.absolutePath();
        addAppTask->filename = addAppInfo.fileName();
        addAppTask->param = QStringList();
        addAppTask->nativeParam = appArgs;
        addAppTask->processType = (waitForExit || taskStage == TaskStage::Primary) ? ProcessType::Blocking : ProcessType::Deferred;

        taskQueue.push(addAppTask);
        logTask(addAppTask.get());

        // Add wait task if required
        ErrorCode enqueueError = enqueueConditionalWaitTask(taskQueue, addAppInfo);
        if(enqueueError)
            return enqueueError;
    }

    // Return success
    return NO_ERR;
}

void enqueueShutdownTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QString fpRoot, FP::Install::Services fpServices)
{
    logEvent(LOG_EVENT_ENQ_STOP);
    // Add Stop entries from services
    for(const FP::Install::StartStop& stopEntry : fpServices.stops)
    {
        std::shared_ptr<ExecTask> shutdownTask = std::make_shared<ExecTask>();
        shutdownTask->stage = TaskStage::Shutdown;
        shutdownTask->path = fpRoot + '/' + stopEntry.path;
        shutdownTask->filename = stopEntry.filename;
        shutdownTask->param = stopEntry.arguments;
        shutdownTask->nativeParam = QString();
        shutdownTask->processType = ProcessType::Blocking;

        taskQueue.push(shutdownTask);
        logTask(shutdownTask.get());
    }
}

ErrorCode enqueueConditionalWaitTask(std::queue<std::shared_ptr<Task>>& taskQueue, QFileInfo precedingAppInfo)
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
        std::shared_ptr<WaitTask> waitTask = std::make_shared<WaitTask>();
        waitTask->stage = TaskStage::Auxiliary;
        waitTask->processName = SECURE_PLAYER_INFO.fileName();

        taskQueue.push(waitTask);
        logTask(waitTask.get());
    }

    // Return success
    return NO_ERR;

    // Possible future waits...
}

ErrorCode enqueueDataPackTasks(std::queue<std::shared_ptr<Task>>& taskQueue, QUuid targetID, const FP::Install& fpInstall)
{
    logEvent(LOG_EVENT_ENQ_DATA_PACK);

    // Get preferences
    FP::Install::Preferences fpPreferences;
    Qx::GenericError prefError;

    if((fpInstall.getPreferences(fpPreferences)).isValid())
    {
        postError(prefError);
        return ErrorCode::CANT_PARSE_PREF;
    }

    // Get entry data
    QSqlError searchError;
    FP::Install::DBQueryBuffer searchResult;

    searchError = fpInstall.queryEntryDataByID(searchResult, targetID);
    if(searchError.isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return SQL_ERROR;
    }

    // Advance result to only record
    searchResult.result.next();

    // Extract relavent data
    QString packDestFolderPath = fpInstall.getPath() + "/" + fpPreferences.dataPacksFolderPath;
    QString packFileName = searchResult.result.value(FP::Install::DBTable_Game_Data::COL_PATH).toString();
    QString packSha256 = searchResult.result.value(FP::Install::DBTable_Game_Data::COL_SHA256).toString();
    QFile packFile(packDestFolderPath + "/" + packFileName);

    // Get current file checksum if it exists
    bool checksumMatches = false;

    if(packFile.exists())
    {
        Qx::IOOpReport checksumReport = Qx::fileMatchesChecksum(checksumMatches, packFile, packSha256, QCryptographicHash::Sha256);
        if(!checksumReport.wasSuccessful())
            logError(Qx::GenericError(Qx::GenericError::Error, checksumReport.getOutcome(), checksumReport.getOutcomeInfo()));

        if(!checksumMatches)
            postError(Qx::GenericError(Qx::GenericError::Warning, WRN_EXIST_PACK_SUM_MISMATCH));

        logEvent(LOG_EVENT_DATA_PACK_FOUND);
    }
    else
        logEvent(LOG_EVENT_DATA_PACK_MISS);

    // Enqueue pack download if it doesn't exist or is different than expected
    if(!packFile.exists() || !checksumMatches)
    {
        // Get Data Pack source info
        searchError = fpInstall.queryEntrySource(searchResult);
        if(searchError.isValid())
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
            return SQL_ERROR;
        }

        // Advance result to only record (or first if there are more than one in future versions)
        searchResult.result.next();

        // Get Data Pack source base URL
        QString sourceBaseUrl = searchResult.result.value(FP::Install::DBTable_Source::COL_BASE_URL).toString();

        // Get title specific Data Pack source info
        searchError = fpInstall.queryEntrySourceData(searchResult, packSha256);
        if(searchError.isValid())
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
            return SQL_ERROR;
        }

        // Advance result to only record
        searchResult.result.next();

        // Get title's Data Pack sub-URL
        QString packSubUrl = searchResult.result.value(FP::Install::DBTable_Source_Data::COL_URL_PATH).toString().replace('\\','/');

        std::shared_ptr<DownloadTask> downloadTask = std::make_shared<DownloadTask>();
        downloadTask->stage = TaskStage::Auxiliary;
        downloadTask->destPath = packDestFolderPath;
        downloadTask->destFileName = packFileName;
        downloadTask->targetFile = sourceBaseUrl + packSubUrl;
        downloadTask->sha256 = packSha256;

        taskQueue.push(downloadTask);
        logTask(downloadTask.get());
    }

    // Enqeue pack mount
    QFileInfo mounterInfo(fpInstall.getDataPackMounterPath());

    std::shared_ptr<ExecTask> mountTask = std::make_shared<ExecTask>();
    mountTask->stage = TaskStage::Auxiliary;
    mountTask->path = mounterInfo.absolutePath();
    mountTask->filename = mounterInfo.fileName();
    mountTask->param = QStringList{targetID.toString(QUuid::WithoutBraces), packDestFolderPath + "/" + packFileName};
    mountTask->nativeParam = QString();
    mountTask->processType = ProcessType::Blocking;

    taskQueue.push(mountTask);
    logTask(mountTask.get());

    // Return success
    return NO_ERR;
}

ErrorCode processTaskQueue(std::queue<std::shared_ptr<Task>>& taskQueue, QList<QProcess*>& childProcesses)
{
    logEvent(LOG_EVENT_QUEUE_START);
    // Error tracker
    ErrorCode executionError = NO_ERR;

    // Exhaust queue
    int taskNum = -1;
    while(!taskQueue.empty())
    {
        // Handle task at front of queue
        ++taskNum;
        std::shared_ptr<Task> currentTask = taskQueue.front();
        logEvent(LOG_EVENT_TASK_START.arg(taskNum).arg(ENUM_NAME(currentTask->stage)));

        // Only execute task after an error if it is a Shutdown task
        if(!executionError || currentTask->stage == TaskStage::Shutdown)
        {
            // Cover each task type
            if(std::dynamic_pointer_cast<MessageTask>(currentTask)) // Message (currently ignores process type)
            {
                std::shared_ptr<MessageTask> messageTask = std::static_pointer_cast<MessageTask>(currentTask);

                QMessageBox::information(nullptr, QCoreApplication::applicationName(), messageTask->message);
                logEvent(LOG_EVENT_SHOW_MESSAGE);
            }
            else if(std::dynamic_pointer_cast<ExtraTask>(currentTask)) // Extra
            {
                std::shared_ptr<ExtraTask> extraTask = std::static_pointer_cast<ExtraTask>(currentTask);

                // Ensure extra exists
                if(!extraTask->dir.exists())
                {
                    postError(Qx::GenericError(Qx::GenericError::Critical, ERR_EXTRA_NOT_FOUND.arg(extraTask->dir.path())));
                    handleExecutionError(taskQueue, taskNum, executionError, EXTRA_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Open extra
                QDesktopServices::openUrl(QUrl::fromLocalFile(extraTask->dir.absolutePath()));
                logEvent(LOG_EVENT_SHOW_EXTRA.arg(extraTask->dir.path()));
            }
            else if(std::dynamic_pointer_cast<WaitTask>(currentTask)) // Wait task
            {
                std::shared_ptr<WaitTask> waitTask = std::static_pointer_cast<WaitTask>(currentTask);

                ErrorCode waitError = waitOnProcess(waitTask->processName, SECURE_PLAYER_GRACE);
                if(waitError)
                {
                    handleExecutionError(taskQueue, taskNum, executionError, waitError);
                    continue; // Continue to next task
                }
            }
            else if(std::dynamic_pointer_cast<DownloadTask>(currentTask))
            {
                std::shared_ptr<DownloadTask> downloadTask = std::static_pointer_cast<DownloadTask>(currentTask);

                // Setup download
                QFile packFile(downloadTask->destPath + "/" + downloadTask->destFileName);
                Qx::SyncDownloadManager dm;
                dm.setOverwrite(true);
                dm.appendTask(Qx::DownloadTask{downloadTask->targetFile, &packFile});

                // Download event handlers
                QObject::connect(&dm, &Qx::SyncDownloadManager::sslErrors, [](Qx::GenericError errorMsg, bool* abort) {
                    int choice = postError(errorMsg, true, QMessageBox::Yes | QMessageBox::Abort, QMessageBox::Abort);
                    *abort = choice == QMessageBox::Abort;
                });

                QObject::connect(&dm, &Qx::SyncDownloadManager::authenticationRequired, [](QString prompt, QString* username, QString* password, bool* abort) {
                    logEvent(LOG_EVENT_DOWNLOAD_AUTH);
                    Qx::LoginDialog ld;
                    ld.setPrompt(prompt);

                    int choice = ld.exec();

                    if(choice == QDialog::Accepted)
                    {
                        *username = ld.getUsername();
                        *password = ld.getPassword();
                    }
                    else
                        *abort = true;
                });

                // Log/label string
                QString label = LOG_EVENT_DOWNLOADING_DATA_PACK.arg(QFileInfo(packFile).fileName());
                logEvent(label);

                // Prepare progress bar
                QProgressDialog pd(label, QString("Cancel"), 0, 0);
                pd.setWindowModality(Qt::WindowModal);
                pd.setMinimumDuration(0);
                QObject::connect(&dm, &Qx::SyncDownloadManager::downloadTotalChanged, &pd, &QProgressDialog::setMaximum);
                QObject::connect(&dm, &Qx::SyncDownloadManager::downloadProgress, &pd, &QProgressDialog::setValue);
                QObject::connect(&pd, &QProgressDialog::canceled, &dm, &Qx::SyncDownloadManager::abort);

                // Start download
                Qx::GenericError downloadError= dm.processQueue();

                // Handle result
                pd.close();
                if(downloadError.isValid())
                {
                    downloadError.setErrorLevel(Qx::GenericError::Critical);
                    postError(downloadError);
                    handleExecutionError(taskQueue, taskNum, executionError, CANT_OBTAIN_DATA_PACK);
                }

                // Confirm checksum
                bool checksumMatch;
                Qx::IOOpReport checksumResult = Qx::fileMatchesChecksum(checksumMatch, packFile, downloadTask->sha256, QCryptographicHash::Sha256);
                if(!checksumResult.wasSuccessful() || !checksumMatch)
                {
                    postError(Qx::GenericError(Qx::GenericError::Critical, ERR_PACK_SUM_MISMATCH));
                    handleExecutionError(taskQueue, taskNum, executionError, DATA_PACK_INVALID);
                }

                logEvent(LOG_EVENT_DOWNLOAD_SUCC);
            }
            else if(std::dynamic_pointer_cast<ExecTask>(currentTask)) // Execution task
            {
                std::shared_ptr<ExecTask> execTask = std::static_pointer_cast<ExecTask>(currentTask);

                // Ensure executable exists
                QFileInfo executableInfo(execTask->path + "/" + execTask->filename);
                if(!executableInfo.exists() || !executableInfo.isFile())
                {
                    postError(Qx::GenericError(execTask->stage == TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                               ERR_EXE_NOT_FOUND.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(taskQueue, taskNum, executionError, EXECUTABLE_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Ensure executable is valid
                if(!executableInfo.isExecutable())
                {
                    postError(Qx::GenericError(execTask->stage == TaskStage::Shutdown ? Qx::GenericError::Error : Qx::GenericError::Critical,
                                               ERR_EXE_NOT_VALID.arg(QDir::toNativeSeparators(executableInfo.absoluteFilePath()))));
                    handleExecutionError(taskQueue, taskNum, executionError, EXECUTABLE_NOT_VALID);
                    continue; // Continue to next task
                }

                // Move to executable directory
                QDir::setCurrent(execTask->path);
                logEvent(LOG_EVENT_CD.arg(execTask->path));

                // Check if task is a batch file
                bool batchTask = QFileInfo(execTask->filename).suffix() == BAT_SUFX;

                // Create process handle
                QProcess* taskProcess = new QProcess();
                taskProcess->setProgram(batchTask ? CMD_EXE : execTask->filename);
                taskProcess->setArguments(execTask->param);
                taskProcess->setNativeArguments(batchTask ? CMD_ARG_TEMPLATE.arg(execTask->filename, escapeNativeArgsForCMD(execTask->nativeParam))
                                                          : execTask->nativeParam);
                taskProcess->setStandardOutputFile(QProcess::nullDevice()); // Don't inhert console window
                taskProcess->setStandardErrorFile(QProcess::nullDevice()); // Don't inhert console window

                // Cover each process type
                switch(execTask->processType)
                {
                    case ProcessType::Blocking:
                        taskProcess->setParent(gAppInstancePtr);
                        if(!cleanStartProcess(taskProcess, executableInfo))
                        {
                            handleExecutionError(taskQueue, taskNum, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, ProcessType::Blocking);

                        taskProcess->waitForFinished(-1); // Wait for process to end, don't time out
                        logProcessEnd(taskProcess, ProcessType::Blocking);
                        delete taskProcess; // Clear finished process handle from heap
                        break;

                    case ProcessType::Deferred:
                        taskProcess->setParent(gAppInstancePtr);
                        if(!cleanStartProcess(taskProcess, executableInfo))
                        {
                            handleExecutionError(taskQueue, taskNum, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, ProcessType::Deferred);

                        childProcesses.append(taskProcess); // Add process to list for deferred termination
                        break;

                    case ProcessType::Detached:
                        if(!taskProcess->startDetached())
                        {
                            handleExecutionError(taskQueue, taskNum, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        logProcessStart(taskProcess, ProcessType::Detached);
                        break;
                }
            }
            else
                throw new std::runtime_error("Unhandled Task child was present in task queue");
        }
        else
            logEvent(LOG_EVENT_TASK_SKIP);

        // Remove handled task
        taskQueue.pop();
        logEvent(LOG_EVENT_TASK_FINISH.arg(taskNum));
    }

    // Return error status
    return executionError;
}

void handleExecutionError(std::queue<std::shared_ptr<Task>>& taskQueue, int taskNum, ErrorCode& currentError, ErrorCode newError)
{
    if(!currentError) // Only record first error
        currentError = newError;

    logEvent(LOG_EVENT_TASK_FINISH_ERR.arg(taskNum)); // Record early end of task
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

QString findFlashpointRoot()
{
    QDir currentDir(CLIFP_DIR_PATH);

    do
    {
        logEvent(LOG_EVENT_FLASHPOINT_ROOT_CHECK.arg(currentDir.absolutePath()));
        if(FP::Install::checkInstallValidity(currentDir.absolutePath(), FP::Install::CompatLevel::Execution).installValid)
            return currentDir.absolutePath();
    }
    while(currentDir.cdUp());

    // Return null string on failure to find root
    return QString();
}

ErrorCode randomlySelectID(QUuid& mainIDBuffer, QUuid& subIDBuffer, FP::Install& fpInstall, FP::Install::LibraryFilter lbFilter)
{
    logEvent(LOG_EVENT_SEL_RAND);

    // Reset buffers
    mainIDBuffer = QUuid();
    subIDBuffer = QUuid();

    // SQL Error tracker
    QSqlError searchError;

    // Query all main games
    FP::Install::DBQueryBuffer mainGameIDQuery;
    searchError = fpInstall.queryAllGameIDs(mainGameIDQuery, lbFilter);
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
    logEvent(LOG_EVENT_PLAYABLE_COUNT.arg(QLocale(QLocale::system()).toString(playableIDs.size())));

    // Select main game
    mainIDBuffer = playableIDs.value(QRandomGenerator::global()->bounded(playableIDs.size()));
    logEvent(LOG_EVENT_INIT_RAND_ID.arg(mainIDBuffer.toString(QUuid::WithoutBraces)));

    // Get entry's playable additional apps
    FP::Install::DBQueryBuffer addAppQuery;
    searchError = fpInstall.queryEntryAddApps(addAppQuery, mainIDBuffer, true);
    if(searchError.isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return SQL_ERROR;
    }
    logEvent(LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT.arg(addAppQuery.size));

    QList<QUuid> playableSubIDs;

    // Enumerate entry's playable additional apps
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Go to next record
        addAppQuery.result.next();

        // Add ID to list
        QString addAppIDString = addAppQuery.result.value(FP::Install::DBTable_Game::COL_ID).toString();
        QUuid addAppID = QUuid(addAppIDString);
        if(!addAppID.isNull())
            playableSubIDs.append(addAppID);
        else
            logError(Qx::GenericError(Qx::GenericError::Warning, LOG_WRN_INVALID_RAND_ID.arg(addAppIDString)));
    }

    // Select final ID
    int randIndex = QRandomGenerator::global()->bounded(playableSubIDs.size() + 1);

    if(randIndex == 0)
        logEvent(LOG_EVENT_RAND_DET_PRIM);
    else
    {
        subIDBuffer = playableSubIDs.value(randIndex - 1);
        logEvent(LOG_EVENT_RAND_DET_ADD_APP.arg(subIDBuffer.toString(QUuid::WithoutBraces)));
    }

    // Return success
    return NO_ERR;
}

ErrorCode getRandomSelectionInfo(QString& infoBuffer, FP::Install& fpInstall, QUuid mainID, QUuid subID)
{
    logEvent(LOG_EVENT_RAND_GET_INFO);

    // Reset buffer
    infoBuffer = QString();

    // Template to fill
    QString infoFillTemplate = RAND_SEL_INFO;

    // SQL Error tracker
    QSqlError searchError;

    // Get main entry info
    FP::Install::DBQueryBuffer mainGameQuery;
    searchError = fpInstall.queryEntryByID(mainGameQuery, mainID);
    if(searchError.isValid())
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
        return SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(mainGameQuery.size == 0)
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_NOT_FOUND));
        return ID_NOT_FOUND;
    }
    else if(mainGameQuery.size > 1)
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_MORE_THAN_ONE_AUTO_P, ERR_MORE_THAN_ONE_AUTO_S));
        return MORE_THAN_ONE_AUTO;
    }

    // Ensure selection is primary app
    if(mainGameQuery.source != FP::Install::DBTable_Game::NAME)
    {
        postError(Qx::GenericError(Qx::GenericError::Critical, ERR_SQL_MISMATCH));
        return SQL_MISMATCH;
    }

    // Advance result to only record
    mainGameQuery.result.next();

    // Populate buffer with primary info
    infoFillTemplate = infoFillTemplate.arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_TITLE).toString())
                               .arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_DEVELOPER).toString())
                               .arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_PUBLISHER).toString())
                               .arg(mainGameQuery.result.value(FP::Install::DBTable_Game::COL_LIBRARY).toString());

    // Determine variant
    if(subID.isNull())
        infoFillTemplate = infoFillTemplate.arg("N/A");
    else
    {
        // Get sub entry info
        FP::Install::DBQueryBuffer addAppQuerry;
        searchError = fpInstall.queryEntryByID(addAppQuerry, subID);
        if(searchError.isValid())
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_UNEXPECTED_SQL, searchError.text()));
            return SQL_ERROR;
        }

        // Check if ID was found and that only one instance was found
        if(addAppQuerry.size == 0)
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_ID_NOT_FOUND));
            return ID_NOT_FOUND;
        }
        else if(addAppQuerry.size > 1)
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_MORE_THAN_ONE_AUTO_P, ERR_MORE_THAN_ONE_AUTO_S));
            return MORE_THAN_ONE_AUTO;
        }

        // Ensure selection is additional app
        if(addAppQuerry.source != FP::Install::DBTable_Add_App::NAME)
        {
            postError(Qx::GenericError(Qx::GenericError::Critical, ERR_SQL_MISMATCH));
            return SQL_MISMATCH;
        }

        // Advance result to only record
        addAppQuerry.result.next();

        // Populate buffer with variant info
        infoFillTemplate = infoFillTemplate.arg(addAppQuerry.result.value(FP::Install::DBTable_Add_App::COL_NAME).toString());
    }

    // Set filled template to buffer
    infoBuffer = infoFillTemplate;

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

int postError(Qx::GenericError error, bool log, QMessageBox::StandardButtons bs, QMessageBox::StandardButton def)
{
    // Logging
    if(log)
        logError(error);

    // Show error if applicable
    if(gNotificationVerbosity == NotificationVerbosity::Full ||
       (gNotificationVerbosity == NotificationVerbosity::Quiet && error.errorLevel() == Qx::GenericError::Critical))
        return error.exec(bs, def);
    else
        return def;
}

void logEvent(QString event)
{
    Qx::IOOpReport logReport = gLogger->recordGeneralEvent(event);
    if(!logReport.wasSuccessful())
        postError(Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);
}

void logTask(const Task* const task) { logEvent(LOG_EVENT_TASK_ENQ.arg(task->name()).arg(task->members().join(", "))); }

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
    Qx::IOOpReport logReport = gLogger->recordErrorEvent(error);

    if(!logReport.wasSuccessful())
        postError(Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);

    if(error.errorLevel() == Qx::GenericError::Critical)
        gCritErrOccurred = true;
}

int logFinish(int exitCode)
{
    if(gCritErrOccurred)
        logEvent(LOG_ERR_CRITICAL);

    Qx::IOOpReport logReport = gLogger->finish(exitCode);
    if(!logReport.wasSuccessful())
        postError(Qx::GenericError(Qx::GenericError::Warning, logReport.getOutcome(), logReport.getOutcomeInfo()), false);

    // Return exit code so main function can return with this one
    return exitCode;
}

