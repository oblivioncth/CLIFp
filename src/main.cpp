#include "version.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <queue>
#include "qx-windows.h"
#include "qx-io.h"
#include "flashpointinstall.h"

#define CLIFP_PATH QCoreApplication::applicationDirPath()

//-Enums-----------------------------------------------------------------------
enum ErrorCode //TODO: Update documentation with current vals
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

enum class OperationMode { Invalid, Normal, Auto, Message, Extra, Information };
enum class TaskType { Startup, Primary, Auxiliary, Wait, Shutdown };
enum class ProcessType { Blocking, Deferred, Detached };

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

// Command line messages
const QString CL_HELP_MESSAGE = "CLIFp Usage:<br>"
                                "<br>"
                                "<b>-" + CL_OPT_HELP_S_NAME + " | --" + CL_OPT_HELP_L_NAME + " | -" + CL_OPT_HELP_E_NAME + ":</b> &nbsp;" + CL_OPT_HELP_DESC + "<br>"
                                "<b>-" + CL_OPT_VERSION_S_NAME + " | --" + CL_OPT_VERSION_L_NAME + ":</b> &nbsp;" + CL_OPT_VERSION_DESC + "<br>"
                                "<b>-" + CL_OPT_APP_S_NAME + " | --" + CL_OPT_APP_L_NAME + ":</b> &nbsp;" + CL_OPT_APP_DESC + "<br>"
                                "<b>-" + CL_OPT_PARAM_S_NAME + " | --" + CL_OPT_PARAM_L_NAME + ":</b> &nbsp;" + CL_OPT_PARAM_DESC + "<br>"
                                "<b>-" + CL_OPT_AUTO_S_NAME + " | --" + CL_OPT_AUTO_L_NAME + ":</b> &nbsp;" + CL_OPT_AUTO_DESC + "<br>"
                                "<b>-" + CL_OPT_MSG_S_NAME + " | --" + CL_OPT_MSG_L_NAME + ":</b> &nbsp;" + CL_OPT_MSG_DESC + "<br>"
                                "<b>-" + CL_OPT_EXTRA_S_NAME + " | --" + CL_OPT_EXTRA_L_NAME + ":</b> &nbsp;" + CL_OPT_EXTRA_DESC + "<br>"
                                "<br>"
                                "Use '" + CL_OPT_APP_L_NAME + "' and '" + CL_OPT_PARAM_L_NAME + "' for normal operation, use '" + CL_OPT_AUTO_L_NAME + "' by itself "
                                "for automatic operation, use '" + CL_OPT_MSG_L_NAME  + "' to display a popup message, use '" + CL_OPT_EXTRA_L_NAME +
                                "' to view an extra, or use '" + CL_OPT_HELP_L_NAME + "' and/or '" + CL_OPT_VERSION_L_NAME + "' for information.";

const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " VER_PRODUCTVERSION_STR ", designed for use with BlueMaxima's Flashpoint " VER_PRODUCTVERSION_STR;

// FP Server Applications
const QFileInfo SECURE_PLAYER_INFO = QFileInfo("FlashpointSecurePlayer.exe");

// Error Messages - Prep
const QString ERR_ALREADY_OPEN = "Only one instance of CLIFp can be used at a time!";
const QString ERR_LAUNCHER_RUNNING = "The CLI cannot be used while the Flashpoint Launcher is running.\n"
                                     "\n"
                                     "Please close the Launcher first.";
const QString ERR_INSTALL_INVALID = "CLIFp does not appear to be deployed in a valid Flashpoint install.\n"
                                    "\n"
                                    "Check its location and compatability with your Flashpoint version.";

const QString ERR_CANT_PARSE_FILE = "Failed to parse %1 ! It may be corrupted or not compatible with this version of CLIFp.";
const QString ERR_CONFIG_SERVER_MISSING = "The server specified in the Flashpoint config was not found within the Flashpoint services store.";
const QString ERR_UNEXPECTED_SQL = "Unexpected SQL error while querying the Flashpoint database:";
const QString ERR_DB_MISSING_TABLE = "The Flashpoint database is missing expected tables.";
const QString ERR_DB_TABLE_MISSING_COLUMN = "The Flashpoint database tables are missing expected columns.";
const QString ERR_AUTO_NOT_FOUND = "An entry matching the specified auto ID could not be found in the Flashpoint database.";
const QString ERR_AUTO_INVALID = "The provided string for auto operation was not a valid GUID/UUID.";
const QString ERR_MORE_THAN_ONE_AUTO = "Multiple entries with the specified auto ID were found.\n"
                                       "\n"
                                       "This should not be possible and may indicate an error within the Flashpoint database";

// Error Messages - Execution
const QString ERR_EXTRA_NOT_FOUND = "The extra %1 does not exist!";
const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";
const QString WRN_WAIT_PROCESS_NOT_HANDLED  = "Could not get a wait handle to %1.\n"
                                                    "\n"
                                                    "The title may not work correctly";
const QString WRN_WAIT_PROCESS_NOT_HOOKED  = "Could not hook %1 for waiting.\n"
                                                   "\n"
                                                   "The title may not work correctly";

// CLI Options
const QCommandLineOption CL_OPTION_APP({CL_OPT_APP_S_NAME, CL_OPT_APP_L_NAME}, CL_OPT_APP_DESC, "application"); // Takes value
const QCommandLineOption CL_OPTION_PARAM({CL_OPT_PARAM_S_NAME, CL_OPT_PARAM_L_NAME}, CL_OPT_PARAM_DESC, "parameters"); // Takes value
const QCommandLineOption CL_OPTION_AUTO({CL_OPT_AUTO_S_NAME, CL_OPT_AUTO_L_NAME}, CL_OPT_AUTO_DESC, "id"); // Takes value
const QCommandLineOption CL_OPTION_MSG({CL_OPT_MSG_S_NAME, CL_OPT_MSG_L_NAME}, CL_OPT_MSG_DESC, "message"); // Takes value
const QCommandLineOption CL_OPTION_EXTRA({CL_OPT_EXTRA_S_NAME, CL_OPT_EXTRA_L_NAME}, CL_OPT_EXTRA_DESC, "extra"); // Takes value
const QCommandLineOption CL_OPTION_HELP({CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC); // Boolean option
const QCommandLineOption CL_OPTION_VERSION({CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC); // Boolean option
const QList<QCommandLineOption> ALL_CL_OPTIONS{CL_OPTION_APP, CL_OPTION_PARAM, CL_OPTION_AUTO, CL_OPTION_MSG, CL_OPTION_EXTRA, CL_OPTION_HELP, CL_OPTION_VERSION};

// CLI Option Operation Mode Map TODO: Submit a patch for Qt6 to make QCommandLineOption directly hashable (implement == and qHash)
const QHash<QSet<QString>, OperationMode> CL_OPTIONS_OP_MODE_MAP{
    {{CL_OPT_APP_S_NAME, CL_OPT_PARAM_S_NAME}, OperationMode::Normal},
    {{CL_OPT_AUTO_S_NAME}, OperationMode::Auto},
    {{CL_OPT_MSG_S_NAME}, OperationMode::Message},
    {{CL_OPT_EXTRA_S_NAME}, OperationMode::Extra},
    {{CL_OPT_HELP_S_NAME}, OperationMode::Information},
    {{CL_OPT_VERSION_S_NAME}, OperationMode::Information},
    {{CL_OPT_HELP_S_NAME, CL_OPT_VERSION_S_NAME}, OperationMode::Information}
};

// Suffixes
const QString EXE_SUFX = "exe";
const QString BAT_SUFX = "bat";

// Wait timing
const int SECURE_PLAYER_GRACE = 2; // Seconds to allow the secure player to restart in cases it does

// Prototypes
int postGenericError(Qx::GenericError error, QMessageBox::StandardButtons choices);
ErrorCode openAndVerifyProperDatabase(FP::Install& fpInstall);
ErrorCode enqueueStartupTasks(std::queue<AppTask>& taskQueue, FP::Install::Config fpConfig, FP::Install::Services fpServices);
ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, QUuid targetID, FP::Install& fpInstall);
ErrorCode enqueueAdditionalApp(std::queue<AppTask>& taskQueue, FP::Install::DBQueryBuffer addAppResult, TaskType taskType, const FP::Install& fpInstall);
void enqueueShutdownTasks(std::queue<AppTask>& taskQueue, FP::Install::Services fpServices);
ErrorCode enqueueConditionalWaitTask(std::queue<AppTask>& taskQueue, QFileInfo precedingAppInfo);
Qx::GenericError appInvolvesSecurePlayer(bool& involvesBuffer, QFileInfo appInfo);
ErrorCode processTaskQueue(std::queue<AppTask>& taskQueue, QList<QProcess*>& childProcesses);
void handleExecutionError(std::queue<AppTask>& taskQueue, ErrorCode& currentError, ErrorCode newError);
bool cleanStartProcess(QProcess* process, QFileInfo exeInfo);
ErrorCode waitOnProcess(QString processName, int graceSecs);
void cleanup(FP::Install& fpInstall, QList<QProcess*>& childProcesses);

int main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // QApplication Object
    QApplication app(argc, argv);

    // Set application name
    QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
    QCoreApplication::setApplicationVersion(VER_FILEVERSION_STR);

    //-Restrict app to only one instance---------------------------------------------------
    if(!Qx::enforceSingleInstance())
    {
        QMessageBox::critical(nullptr, QApplication::applicationName(), ERR_ALREADY_OPEN);
        return ALREADY_OPEN;
    }

    // Ensure Flashpoint Launcher isn't running
    if(Qx::processIsRunning(QFileInfo(FP::Install::MAIN_EXE_PATH).fileName()))
    {
        QMessageBox::critical(nullptr, QApplication::applicationName(), ERR_LAUNCHER_RUNNING);
        return LAUNCHER_OPEN;
    }

    // CLI Parser
    QCommandLineParser clParser;
    clParser.setApplicationDescription(VER_FILEDESCRIPTION_STR);
    clParser.addOptions({CL_OPTION_APP, CL_OPTION_PARAM, CL_OPTION_AUTO, CL_OPTION_MSG, CL_OPTION_EXTRA, CL_OPTION_HELP, CL_OPTION_VERSION});
    clParser.process(app);

    //-Link to Flashpoint Install----------------------------------------------------------
    if(!FP::Install::pathIsValidInstall(CLIFP_PATH, FP::Install::CompatLevel::Execution))
    {
        QMessageBox::critical(nullptr, QApplication::applicationName(), ERR_INSTALL_INVALID);
        return INSTALL_INVALID;
    }

    FP::Install flashpointInstall(CLIFP_PATH);

    //-Determine Operation Mode------------------------------------------------------------
    OperationMode operationMode;
    QSet<QString> providedArgs;
    for(const QCommandLineOption& clOption : ALL_CL_OPTIONS)
        if(clParser.isSet(clOption))
            providedArgs.insert(clOption.names().value(0)); // Add options shortname to set

    if(CL_OPTIONS_OP_MODE_MAP.contains(providedArgs))
        operationMode = CL_OPTIONS_OP_MODE_MAP.value(providedArgs);
    else
        operationMode = OperationMode::Invalid;

    //-Get Install Settings----------------------------------------------------------------
    Qx::GenericError settingsReadError;
    FP::Install::Config flashpointConfig;
    FP::Install::Services flashpointServices;

    if((settingsReadError = flashpointInstall.getConfig(flashpointConfig)).isValid())
    {
        postGenericError(settingsReadError, QMessageBox::Ok);
        return CANT_PARSE_CONFIG;
    }

    if((settingsReadError = flashpointInstall.getServices(flashpointServices)).isValid())
    {
        postGenericError(settingsReadError, QMessageBox::Ok);
        return CANT_PARSE_CONFIG;
    }

    //-Proccess Tracking-------------------------------------------------------------------
    QList<QProcess*> activeChildProcesses;
    std::queue<AppTask> appTaskQueue;

    //-Handle Mode Specific Operations-----------------------------------------------------
    ErrorCode enqueueError;
    QFileInfo inputInfo;
    QUuid autoID;

    switch(operationMode)
    {    
        case OperationMode::Invalid:
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);
            return INVALID_ARGS;

        case OperationMode::Normal: 
            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)))
                return enqueueError;

            inputInfo = QFileInfo(CLIFP_PATH + '/' + clParser.value(CL_OPTION_APP));
            appTaskQueue.push({TaskType::Primary, inputInfo.absolutePath(), inputInfo.fileName(),
                               QStringList(), clParser.value(CL_OPTION_PARAM),
                               ProcessType::Blocking});

            // Add wait task if required
            if((enqueueError = enqueueConditionalWaitTask(appTaskQueue, inputInfo)))
                return enqueueError;
            break;

        case OperationMode::Auto:
            if((autoID = QUuid(clParser.value(CL_OPTION_AUTO))).isNull())
            {
                QMessageBox::critical(nullptr, QCoreApplication::applicationName(), ERR_AUTO_INVALID);
                return AUTO_ID_NOT_VALID;
            }

            if((enqueueError = openAndVerifyProperDatabase(flashpointInstall)))
                return enqueueError;

            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)))
                return enqueueError;

            if((enqueueError = enqueueAutomaticTasks(appTaskQueue, autoID, flashpointInstall)))
                return enqueueError;
            break;

        case OperationMode::Message:
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), clParser.value(CL_OPTION_MSG));
            return NO_ERR;

        case OperationMode::Extra:
            // Ensure extra exists
            inputInfo = QFileInfo(flashpointInstall.getExtrasDirectory().absolutePath() + '/' + clParser.value(CL_OPTION_EXTRA));
            if(!inputInfo.exists() || !inputInfo.isDir())
            {
                QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                      ERR_EXTRA_NOT_FOUND.arg(inputInfo.fileName()));
                return EXTRA_NOT_FOUND;
            }

            // Open extra
            QDesktopServices::openUrl(QUrl::fromLocalFile(inputInfo.absoluteFilePath()));
            return NO_ERR;

        case OperationMode::Information:
            if(clParser.isSet(CL_OPTION_VERSION))
                QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_VERSION_MESSAGE);
            if(clParser.isSet(CL_OPTION_HELP))
                QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);
            return NO_ERR;
    }

    // Enqueue Shudown Tasks if main task isn't message/extra
    if(appTaskQueue.size() != 1 ||
       (appTaskQueue.front().path != FP::Install::DBTable_Add_App::ENTRY_MESSAGE &&
       appTaskQueue.front().path != FP::Install::DBTable_Add_App::ENTRY_EXTRAS))
        enqueueShutdownTasks(appTaskQueue, flashpointServices);

    // Process app task queue
    ErrorCode executionError = processTaskQueue(appTaskQueue, activeChildProcesses);

    // Cleanup
    cleanup(flashpointInstall, activeChildProcesses);

    // Return error status
    return executionError;
}

int postGenericError(Qx::GenericError error, QMessageBox::StandardButtons choices)
{
    // Prepare dialog
    QMessageBox genericErrorMessage;
    if(!error.getCaption().isEmpty())
        genericErrorMessage.setWindowTitle(error.getCaption());
    if(!error.getPrimaryInfo().isEmpty())
        genericErrorMessage.setText(error.getPrimaryInfo());
    if(!error.getSecondaryInfo().isEmpty())
        genericErrorMessage.setInformativeText(error.getSecondaryInfo());
    if(!error.getDetailedInfo().isEmpty())
        genericErrorMessage.setDetailedText(error.getDetailedInfo());
    genericErrorMessage.setStandardButtons(choices);
    genericErrorMessage.setIcon(QMessageBox::Critical);

    return genericErrorMessage.exec();
}

ErrorCode openAndVerifyProperDatabase(FP::Install& fpInstall)
{
    // Open database connection
    QSqlError databaseError = fpInstall.openThreadDatabaseConnection();
    if(databaseError.isValid())
    {
        postGenericError(Qx::GenericError(QString(), ERR_UNEXPECTED_SQL, databaseError.text()), QMessageBox::Ok);
        return SQL_ERROR;
    }

    // Ensure required database tables are present
    QSet<QString> missingTables;
    if((databaseError = fpInstall.checkDatabaseForRequiredTables(missingTables)).isValid())
    {
        postGenericError(Qx::GenericError(QString(), ERR_UNEXPECTED_SQL, databaseError.text()), QMessageBox::Ok);
        return SQL_ERROR;
    }

    // Check if tables are missing
    if(!missingTables.isEmpty())
    {
        postGenericError(Qx::GenericError(QString(), ERR_DB_MISSING_TABLE, QString(),
                         QStringList(missingTables.begin(), missingTables.end()).join("\n")), QMessageBox::Ok);
        return DB_MISSING_TABLES;
    }

    // Ensure the database contains the required columns
    QSet<QString> missingColumns;
    if((databaseError = fpInstall.checkDatabaseForRequiredColumns(missingColumns)).isValid())
    {
        postGenericError(Qx::GenericError(QString(), ERR_UNEXPECTED_SQL, databaseError.text()), QMessageBox::Ok);
        return SQL_ERROR;
    }

    // Check if columns are missing
    if(!missingColumns.isEmpty())
    {
        postGenericError(Qx::GenericError(QString(), ERR_DB_TABLE_MISSING_COLUMN, QString(),
                         QStringList(missingColumns.begin(), missingColumns.end()).join("\n")), QMessageBox::Ok);
        return DB_MISSING_COLUMNS;
    }

    // Return success
    return NO_ERR;
}

ErrorCode enqueueStartupTasks(std::queue<AppTask>& taskQueue, FP::Install::Config fpConfig, FP::Install::Services fpServices)
{
    // Add Start entries from services
    for(const FP::Install::StartStop& startEntry : fpServices.starts)
        taskQueue.push({TaskType::Startup, CLIFP_PATH + '/' + startEntry.path, startEntry.filename,
                        startEntry.arguments, QString(),
                        ProcessType::Blocking});

    // Add Server entry from services if applicable
    if(fpConfig.startServer)
    {
        if(!fpServices.servers.contains(fpConfig.server))
        {
            QMessageBox::critical(nullptr, QCoreApplication::applicationName(), ERR_CONFIG_SERVER_MISSING);
            return CONFIG_SERVER_MISSING;
        }

        FP::Install::Server configuredServer = fpServices.servers.value(fpConfig.server);

        taskQueue.push({TaskType::Startup, CLIFP_PATH + '/' + configuredServer.path, configuredServer.filename,
                        configuredServer.arguments, QString(),
                        configuredServer.kill ? ProcessType::Deferred : ProcessType::Detached});
    }

    // Return success
    return NO_ERR;
}

ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, QUuid targetID, FP::Install& fpInstall)
{
    // Search FP database for entry via ID
    QSqlError searchError;
    FP::Install::DBQueryBuffer searchResult;
    ErrorCode enqueueError;

    searchError = fpInstall.queryEntryID(searchResult, targetID);
    if(searchError.isValid())
    {
        postGenericError(Qx::GenericError(QString(), ERR_UNEXPECTED_SQL, searchError.text()), QMessageBox::Ok);
        return SQL_ERROR;
    }

    // Check if ID was found and that only one instance was found
    if(searchResult.size == 0)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), ERR_AUTO_NOT_FOUND);
        return AUTO_NOT_FOUND;
    }
    else if(searchResult.size > 1)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), ERR_MORE_THAN_ONE_AUTO);
        return MORE_THAN_ONE_AUTO;
    }

    // Advance result to only record
    searchResult.result.next();

    // Enqueue if result is additional app
    if(searchResult.source == FP::Install::DBTable_Add_App::NAME)
    {
        // Replace queue with this single entry if it is a message or extra
        QString appPath = searchResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
        if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE || appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
            taskQueue = std::queue<AppTask>();

        ErrorCode enqueueError = enqueueAdditionalApp(taskQueue, searchResult, TaskType::Primary, fpInstall);
        if(enqueueError)
            return enqueueError;
    }
    else if(searchResult.source == FP::Install::DBTable_Game::NAME) // Get autorun additional apps if result is game
    {
        // Get game's additional apps
        QSqlError addAppSearchError;
        FP::Install::DBQueryBuffer addAppSearchResult;

        addAppSearchError = fpInstall.queryEntryAddApps(addAppSearchResult, targetID);
        if(addAppSearchError.isValid())
        {
            postGenericError(Qx::GenericError(QString(), ERR_UNEXPECTED_SQL, addAppSearchError.text()), QMessageBox::Ok);
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
                enqueueError = enqueueAdditionalApp(taskQueue, addAppSearchResult, TaskType::Auxiliary, fpInstall);
                if(enqueueError)
                    return enqueueError;
            }
        }

        // Enqueue game
        QString gamePath = searchResult.result.value(FP::Install::DBTable_Game::COL_APP_PATH).toString();
        QString gameArgs = searchResult.result.value(FP::Install::DBTable_Game::COL_LAUNCH_COMMAND).toString();
        QFileInfo gameInfo(CLIFP_PATH + '/' + gamePath);
        taskQueue.push({TaskType::Primary, gameInfo.absolutePath(), gameInfo.fileName(),
                        QStringList(), gameArgs, ProcessType::Blocking});

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
        taskQueue.push({TaskType::Primary, appPath, QString(),
                        QStringList(), appArgs,
                        waitForExit ? ProcessType::Blocking : ProcessType::Deferred});
    }
    else if(appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
    {
        taskQueue.push({TaskType::Primary, appPath, QString(),
                        QStringList(), fpInstall.getExtrasDirectory().absolutePath() + "/" + appArgs,
                        ProcessType::Detached});
    }
    else
    {
        QFileInfo addAppInfo(CLIFP_PATH + '/' + appPath);
        taskQueue.push({taskType, addAppInfo.absolutePath(), addAppInfo.fileName(),
                        QStringList(), appArgs,
                        waitForExit ? ProcessType::Blocking : ProcessType::Deferred});

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
    // Add Stop entries from services
    for(const FP::Install::StartStop& stopEntry : fpServices.stops)
    {
        taskQueue.push({TaskType::Shutdown, CLIFP_PATH + '/' + stopEntry.path, stopEntry.filename,
                        stopEntry.arguments, QString(),
                        ProcessType::Blocking});
    }
}

ErrorCode enqueueConditionalWaitTask(std::queue<AppTask>& taskQueue, QFileInfo precedingAppInfo)
{
    // Add wait for apps that involve secure player
    bool involvesSecurePlayer;
    Qx::GenericError securePlayerCheckError = appInvolvesSecurePlayer(involvesSecurePlayer, precedingAppInfo);
    if(securePlayerCheckError.isValid())
    {
        postGenericError(securePlayerCheckError, QMessageBox::Ok);
        return CANT_READ_BAT_FILE;
    }

    if(involvesSecurePlayer)
        taskQueue.push({TaskType::Wait, QString(), SECURE_PLAYER_INFO.fileName(),
                        QStringList(), QString(),
                        ProcessType::Blocking});

    // Return success
    return NO_ERR;

    // Possible future waits...
}

ErrorCode processTaskQueue(std::queue<AppTask>& taskQueue, QList<QProcess*>& childProcesses)
{
    // Error tracker
    ErrorCode executionError = NO_ERR;

    // Exhaust queue
    while(!taskQueue.empty())
    {
        // Handle task at front of queue
        AppTask currentTask = taskQueue.front();

        // Only execute task after an error if it is a Shutdown task
        if(!executionError || currentTask.type == TaskType::Shutdown)
        {
            // Handle general and special case(s)
            if(currentTask.path == FP::Install::DBTable_Add_App::ENTRY_MESSAGE) // Message (currently ignores process type)
                QMessageBox::information(nullptr, QCoreApplication::applicationName(), currentTask.nativeParam);
            else if(currentTask.path == FP::Install::DBTable_Add_App::ENTRY_EXTRAS) // Extra
            {
                // Ensure extra exists
                QFileInfo extraInfo(currentTask.nativeParam);
                if(!extraInfo.exists() || !extraInfo.isDir())
                {
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                          ERR_EXTRA_NOT_FOUND.arg(extraInfo.fileName()));
                    handleExecutionError(taskQueue, executionError, EXTRA_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Open extra
                QDesktopServices::openUrl(QUrl::fromLocalFile(extraInfo.absoluteFilePath()));
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
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                          ERR_EXE_NOT_FOUND.arg(executableInfo.absoluteFilePath()));
                    handleExecutionError(taskQueue, executionError, EXECUTABLE_NOT_FOUND);
                    continue; // Continue to next task
                }

                // Ensure executable is valid
                if(!executableInfo.isExecutable())
                {
                    QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                          ERR_EXE_NOT_VALID.arg(executableInfo.absoluteFilePath()));
                    handleExecutionError(taskQueue, executionError, EXECUTABLE_NOT_VALID);
                    continue; // Continue to next task
                }

                // Move to executable directory
                QDir::setCurrent(currentTask.path);

                // Create process handle
                QProcess* taskProcess = new QProcess();
                taskProcess->setProgram(currentTask.filename);
                taskProcess->setArguments(currentTask.param);
                taskProcess->setNativeArguments(currentTask.nativeParam);
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

                        taskProcess->waitForFinished(-1); // Wait for process to end, don't time out
                        delete taskProcess; // Clear finished process handle from heap
                        break;

                    case ProcessType::Deferred:
                        if(!cleanStartProcess(taskProcess, executableInfo))
                        {
                            handleExecutionError(taskQueue, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }

                        childProcesses.append(taskProcess); // Add process to list for deferred termination
                        break;

                    case ProcessType::Detached:
                        if(!taskProcess->startDetached())
                        {
                            handleExecutionError(taskQueue, executionError, PROCESS_START_FAIL);
                            continue; // Continue to next task
                        }
                        break;
                }
            }
        }

        // Remove handled task
        taskQueue.pop();
    }

    // Return error status
    return executionError;
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
        // Read bat file
        QFile batFile(appInfo.absoluteFilePath());
        QString batScript;
        Qx::IOOpReport readReport = Qx::readAllTextFromFile(batScript, batFile);

        // Check for read errors
        if(!readReport.wasSuccessful())
            return Qx::GenericError(QString(), readReport.getOutcome(), readReport.getOutcomeInfo());

        // Check if bat uses secure player
        involvesBuffer = batScript.contains(SECURE_PLAYER_INFO.baseName());
        return Qx::GenericError();
    }
    else
        return Qx::GenericError();
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
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(exeInfo.absoluteFilePath())));
        delete process; // Clear finished process handle from heap
        return false;
    }

    // Return success
    return true;
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
        delete childProcess;
    }
}

ErrorCode waitOnProcess(QString processName, int graceSecs)
{

    // Wait until secure player has stopped running for grace period
    DWORD spProcessID;
    do
    {
        // Yield for grace period
        if(graceSecs > 0)
            QThread::sleep(graceSecs);

        // Find process ID by name
        spProcessID = Qx::getProcessIDByName(processName);

        // Check that process was found (is running)
        if(spProcessID)
        {
            // Get process handle and see if it is valid
            HANDLE batchProcessHandle;
            if((batchProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, spProcessID)) == NULL)
            {
                QMessageBox::warning(nullptr, QCoreApplication::applicationName(), WRN_WAIT_PROCESS_NOT_HANDLED.arg(processName));
                return WAIT_PROCESS_NOT_HANDLED;
            }

            // Attempt to wait on process to terminate
            DWORD waitError = WaitForSingleObject(batchProcessHandle, INFINITE);

            // Close handle to process
            CloseHandle(batchProcessHandle);

            if(waitError != WAIT_OBJECT_0)
            {
                QMessageBox::warning(nullptr, QCoreApplication::applicationName(), WRN_WAIT_PROCESS_NOT_HOOKED.arg(processName));
                return WAIT_PROCESS_NOT_HOOKED;
            }
        }
    }
    while(spProcessID);

    // Return success
    return NO_ERR;
}

