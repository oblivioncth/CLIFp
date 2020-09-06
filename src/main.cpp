#include "version.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QCommandLineParser>
#include <QDebug>
#include <queue>
#include "qx-windows.h"
#include "flashpointinstall.h"

#define CLIFP_PATH QCoreApplication::applicationDirPath()

//-Enums-----------------------------------------------------------------------
enum ErrorCode //TODO: RE-DISTRIBUTE VALUES, Consider having single return value only (first error)
{
    NO_ERR = 0x00,
    INVALID_ARGS = 0x02,
    INSTALL_INVALID = 0x04,
    CANT_PARSE_CONFIG = 0x000000,
    CANT_PARSE_SERVICES = 0x0000000,
    CONFIG_SERVER_MISSING = 0x0000000,
    AUTO_ID_NOT_VALID = 0x00000000,
    SQL_ERROR = 0x0000000000,
    DB_MISSING_TABLES = 0x000000000000,
    DB_MISSING_COLUMNS = 0x000000000,
    AUTO_NOT_FOUND = 0x0000000000,
    MORE_THAN_ONE_AUTO = 0x00000000000,

    BATCH_PROCESS_NOT_FOUND = 0x100,
    BATCH_PROCESS_NOT_HANDLED = 0x200,
    BATCH_PROCESS_NOT_HOOKED = 0x400
};
Q_DECLARE_FLAGS(ErrorCodes, ErrorCode)
Q_DECLARE_OPERATORS_FOR_FLAGS(ErrorCodes);

enum class OperationMode { Invalid, Normal, Auto, Message, Information };
enum class TaskType { Startup, Primary, Auxiliary, Wait, Shutdown };
enum class ProcessType { Blocking, Deferred, Detached };

//-Structs---------------------------------------------------------------------
struct AppTask
{
    TaskType type;
    QString path;
    QString filename;
    QStringList param;
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

const QString CL_OPT_APP_S_NAME = "e";
const QString CL_OPT_APP_L_NAME = "exe";
const QString CL_OPT_APP_DESC = "Path of primary application to launch.";

const QString CL_OPT_PARAM_S_NAME = "p";
const QString CL_OPT_PARAM_L_NAME = "param";
const QString CL_OPT_PARAM_DESC = "Command-line parameters to use when starting the primary application.";

const QString CL_OPT_MSG_S_NAME = "m";
const QString CL_OPT_MSG_L_NAME = "msg";
const QString CL_OPT_MSG_DESC = "Displays an pop-up dialog with the supplied message. Used primarily for some additional apps.";

const QString CL_OPT_AUTO_S_NAME = "a";
const QString CL_OPT_AUTO_L_NAME = "auto";
const QString CL_OPT_AUTO_DESC = "Finds a game/additional-app by ID and runs it if found, including run-before/run-after additional apps in the case of a game.";

// Command line messages
const QString CL_HELP_MESSAGE = "CLIFp Usage:\n"
                                "\n"
                                "-" + CL_OPT_HELP_S_NAME + " | --" + CL_OPT_HELP_L_NAME + " | -" + CL_OPT_HELP_E_NAME + ": " + CL_OPT_HELP_DESC + "\n"
                                "-" + CL_OPT_VERSION_S_NAME + " | --" + CL_OPT_VERSION_L_NAME + ": " + CL_OPT_VERSION_DESC + "\n"
                                "-" + CL_OPT_APP_S_NAME + " | --" + CL_OPT_APP_L_NAME + ": " + CL_OPT_APP_DESC + "\n"
                                "-" + CL_OPT_PARAM_S_NAME + " | --" + CL_OPT_PARAM_L_NAME + ": " + CL_OPT_PARAM_DESC + "\n"
                                "-" + CL_OPT_MSG_S_NAME + " | --" + CL_OPT_MSG_L_NAME + ": " + CL_OPT_MSG_DESC + "\n"
                                "\n"
                                "Use '" + CL_OPT_APP_L_NAME + "' and '" + CL_OPT_PARAM_L_NAME + "', for normal operation, use '" + CL_OPT_AUTO_L_NAME + "'by itself "
                                "for automatic operation, use '" + CL_OPT_MSG_L_NAME  + "' to display a popup message, or '" + CL_OPT_HELP_L_NAME + "' and/or '"
                                + CL_OPT_VERSION_L_NAME + "' for information.";

const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " VER_PRODUCTVERSION_STR ", designed for use with BlueMaxima's Flashpoint " VER_PRODUCTVERSION_STR;

// FP Server Applications
const QString PHP_EXE_PATH = "Server/php.exe";
const QString HTTPD_EXE_PATH = "Server/httpd.exe";
const QString BATCH_WAIT_EXE = "FlashpointSecurePlayer.exe";

// FP Server App Arguments
const QStringList PHP_ARGS_STARTUP = {"-f", "update_httpdconf_main_dir.php"};
const QStringList PHP_ARGS_SHUTDOWN = {"-f", "reset_httpdconf_main_dir.php"};
const QStringList HTTPD_ARGS_STARTUP = {"-f", "..//Server/conf/httpd.conf", "-X"};

// FPSoftware Applications
const QString FIDDLER_EXE_PATH = "FPSoftware/Fiddler2Portable/App/Fiddler/ExecAction.exe";
const QString REDIRECTOR_EXE_PATH = "FPSoftware/Redirector/Redirect.exe";

// FPSoftware App Arguments
const QStringList FIDDLER_ARGS_SHUTDOWN = {"quit"};
const QStringList REDIRECTOR_ARGS_SHUTDOWN = {"/close"};

// Core Application Paths
const QStringList CORE_APP_PATHS = {PHP_EXE_PATH, HTTPD_EXE_PATH};

// Error Messages - Prep
const QString ERR_INSTALL_INVALID = "CLIFp does not appear to be deployed in a valid Flashpoint install.\n"
                                    "\n"
                                    "Check its location and compatability with your Flashpoint version.";

const QString ERR_CANT_PARSE_FILE = "Failed to parse %1 ! It may be corrupted or not compatible with this version of CLIFp.";
const QString ERR_CONFIG_SERVER_MISSING = "The server specified in the Flashpoint config was not found within the Flashpoint services store.";
const QString ERR_UNEXPECTED_SQL = "Unexpected SQL error while querying the Flashpoint database:";
const QString ERR_DB_MISSING_TABLE = "The Flashpoint database is missing expected tables.";
const QString ERR_DB_TABLE_MISSING_COLUMN = "The Flashpoint database tables are missing expected columns.";
const QString ERR_AUTO_NOT_FOUND = "An entry matching the specified auto ID could not be found in the Flashpoint database.";
const QString ERR_MORE_THAN_ONE_AUTO = "Multiple entries with the specified auto ID were found.\n"
                                       "\n"
                                       "This should not be possible and may indicate an error within the Flashpoint database";

// Error Messages - Execution


// Error Messages
const QString ERR_EXE_NOT_FOUND = "Could not find %1!\n\nExecution will now be aborted.";
const QString ERR_EXE_NOT_STARTED = "Could not start %1!\n\nExecution will now be aborted.";
const QString BATCH_WRN_SUFFIX = "after a primary application utilizing a batch file was started. The game may not work correctly.";
const QString WRN_BATCH_WAIT_PROCESS_NOT_FOUND  = "Could not find the wait-on process to hook to " + BATCH_WRN_SUFFIX;
const QString WRN_BATCH_WAIT_PROCESS_NOT_HANDLED  = "Could not get a handle to the wait-on process " + BATCH_WRN_SUFFIX;
const QString WRN_BATCH_WAIT_PROCESS_NOT_HOOKED  = "Could not hook the wait-on process " + BATCH_WRN_SUFFIX;

// CLI Options
const QCommandLineOption CL_OPTION_APP({CL_OPT_APP_S_NAME, CL_OPT_APP_L_NAME}, CL_OPT_APP_DESC, "application"); // Takes value
const QCommandLineOption CL_OPTION_PARAM({CL_OPT_PARAM_S_NAME, CL_OPT_PARAM_L_NAME}, CL_OPT_PARAM_DESC, "parameters"); // Takes value
const QCommandLineOption CL_OPTION_AUTO({CL_OPT_AUTO_S_NAME, CL_OPT_AUTO_L_NAME}, CL_OPT_AUTO_DESC, "id"); // Takes value
const QCommandLineOption CL_OPTION_MSG({CL_OPT_MSG_S_NAME, CL_OPT_MSG_L_NAME}, CL_OPT_MSG_DESC, "message"); // Takes value
const QCommandLineOption CL_OPTION_HELP({CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC); // Boolean option
const QCommandLineOption CL_OPTION_VERSION({CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC); // Boolean option
const QList<QCommandLineOption> ALL_CL_OPTIONS{CL_OPTION_APP, CL_OPTION_PARAM, CL_OPTION_AUTO, CL_OPTION_MSG, CL_OPTION_HELP, CL_OPTION_VERSION};

// CLI Option Operation Mode Map TODO: Submit a patch for Qt6 to make QCommandLineOption directly hashable (implement == and qHash)
const QHash<QSet<QString>, OperationMode> CL_OPTIONS_OP_MODE_MAP{
    {{CL_OPT_APP_S_NAME, CL_OPT_PARAM_S_NAME}, OperationMode::Normal},
    {{CL_OPT_AUTO_S_NAME}, OperationMode::Auto},
    {{CL_OPT_MSG_S_NAME}, OperationMode::Message},
    {{CL_OPT_HELP_S_NAME}, OperationMode::Information},
    {{CL_OPT_VERSION_S_NAME}, OperationMode::Information},
    {{CL_OPT_HELP_S_NAME, CL_OPT_VERSION_S_NAME}, OperationMode::Information}
};

// Prototypes
int postGenericError(Qx::GenericError error, QMessageBox::StandardButtons choices);
ErrorCode openAndVerifyProperDatabase(FP::Install& fpInstall);
ErrorCode enqueueStartupTasks(std::queue<AppTask>& taskQueue, FP::Install::Config fpConfig, FP::Install::Services fpServices);
ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, QUuid targetID, FP::Install& fpInstall);
void enqueueAdditionalApp(std::queue<AppTask>& taskQueue, FP::Install::DBQueryBuffer addAppResult, TaskType taskType);
void enqueueShutdownTasks(std::queue<AppTask>& taskQueue, FP::Install::Services fpServices);
ErrorCode processTaskQueue(std::queue<AppTask>& taskQueue, QList<QProcess*>& childProcesses);
bool cleanStartProcess(QProcess* process, QString programPath);
void cleanup(FP::Install& fpInstall, QList<QProcess*>& childProcesses);

ErrorCode startupProcedure();
ErrorCodes shutdownProcedure(bool silent);
ErrorCode shutdownApplication(QString exePath, QStringList shutdownArgs, bool& silent);
ErrorCode primaryApplicationExecution(QFile& primaryApp, QStringList primaryAppParameters);
ErrorCode waitOnProcess(QString processName);

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
    clParser.addOptions({CL_OPTION_APP, CL_OPTION_PARAM, CL_OPTION_AUTO, CL_OPTION_MSG, CL_OPTION_HELP, CL_OPTION_VERSION});
    clParser.process(app);

    //-Link to Flashpoint Install----------------------------------------------------------
    if(!FP::Install::pathIsValidInstall(CLIFP_PATH))
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
    QFileInfo appInfo;
    QUuid autoID;

    switch(operationMode)
    {    
        case OperationMode::Invalid:
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);
            return INVALID_ARGS;

        case OperationMode::Normal: 
            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)) != NO_ERR)
                return enqueueError;

            appInfo = clParser.value(CL_OPTION_APP);
            appTaskQueue.push({TaskType::Primary, appInfo.absolutePath(), appInfo.fileName(),
                               clParser.value(CL_OPTION_PARAM).split(" "), ProcessType::Blocking});

            // Add wait task if specified app uses a batch file
            if(appInfo.suffix() == ".bat")
                appTaskQueue.push({TaskType::Wait, QString(), BATCH_WAIT_EXE, QStringList(), ProcessType::Blocking});
            break;

        case OperationMode::Auto:
            if((autoID = QUuid(clParser.value(CL_OPTION_AUTO))).isNull())
                return AUTO_ID_NOT_VALID;

            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)) != NO_ERR)
                return enqueueError;

            if((enqueueError = enqueueAutomaticTasks(appTaskQueue, autoID, flashpointInstall)) != NO_ERR)
                return enqueueError;
            break;

        case OperationMode::Message:
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), clParser.value(CL_OPTION_MSG));
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

//    // Handle primary CLI options
//    QFile primaryApp(clParser.value(CL_OPTION_APP));
//    QString primaryAppParam(clParser.value(CL_OPTION_PARAM));

//    //-Check for existance of required core applications-----------------------------------
//    for(QString coreApp : CORE_APP_PATHS)
//    {
//         QString fullAppPath = QDir::toNativeSeparators(CLIFP_PATH + "/" + coreApp);
//         if(!QFileInfo::exists(fullAppPath) || !QFileInfo(fullAppPath).isFile())
//         {
//             QMessageBox::critical(nullptr, QCoreApplication::applicationName(), ERR_EXE_NOT_FOUND.arg(fullAppPath));
//             return CORE_APP_NOT_FOUND;
//         }
//    }

//    // Working variables
//    ErrorCodes currentStatus = ErrorCode::NO_ERR;

//    //-Startup Procedure-------------------------------------------------------------------
//    currentStatus = startupProcedure();

//    //-Primary Application-------------------------------------------------------------------
//    if(currentStatus == NO_ERR)
//    {
//        currentStatus |= primaryApplicationExecution(primaryApp, primaryAppParam.split(" "));

//        //-Wait If Batch-------------------------------------------------------------------------
//        if(currentStatus == NO_ERR && QFileInfo(primaryApp).suffix() == ".bat")
//             currentStatus |= waitOnBatchProcess();
//    }

//    //-Shutdown Procedure--------------------------------------------------------------------
//    currentStatus |= shutdownProcedure(currentStatus != NO_ERR);

//    // Close interface
//    return currentStatus;
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
        taskQueue.push({TaskType::Startup, CLIFP_PATH + '/' + startEntry.path, startEntry.filename, startEntry.arguments, ProcessType::Blocking});

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
                        configuredServer.arguments, configuredServer.kill ? ProcessType::Deferred : ProcessType::Detached});
    }

    // Return success
    return NO_ERR;
}

ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, QUuid targetID, FP::Install& fpInstall)
{
    // Search FP database for entry via ID
    QSqlError searchError;
    FP::Install::DBQueryBuffer searchResult;

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

        enqueueAdditionalApp(taskQueue, searchResult, TaskType::Primary);
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
                enqueueAdditionalApp(taskQueue, addAppSearchResult, TaskType::Auxiliary);
        }

        // Enqueue game
        QString gamePath = searchResult.result.value(FP::Install::DBTable_Game::COL_APP_PATH).toString();
        QString gameArgs = searchResult.result.value(FP::Install::DBTable_Game::COL_LAUNCH_COMMAND).toString();
        QFileInfo gameInfo(gamePath);
        taskQueue.push({TaskType::Primary, CLIFP_PATH + '/' + gameInfo.path(), gameInfo.fileName(), gameArgs.split(" "), ProcessType::Blocking});

        // Add wait task if specified app uses a batch file
        if(gameInfo.suffix() == ".bat")
            taskQueue.push({TaskType::Wait, QString(), BATCH_WAIT_EXE, QStringList(), ProcessType::Blocking});
    }
    else
        throw std::runtime_error("Auto ID search result source must be 'game' or 'additional_app'");

    // Return success
    return NO_ERR;
}

void enqueueAdditionalApp(std::queue<AppTask>& taskQueue, FP::Install::DBQueryBuffer addAppResult, TaskType taskType)
{
    // Ensure query result is additional app
    assert(addAppResult.source == FP::Install::DBTable_Add_App::NAME);

    QString appPath = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_APP_PATH).toString();
    QString appArgs = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_LAUNCH_COMMAND).toString();
    bool waitForExit = addAppResult.result.value(FP::Install::DBTable_Add_App::COL_WAIT_EXIT).toInt() != 0;

    if(appPath == FP::Install::DBTable_Add_App::ENTRY_MESSAGE)
        taskQueue.push({TaskType::Primary, appPath, QString(), {appArgs}, waitForExit ? ProcessType::Blocking : ProcessType::Deferred});
    else if(appPath == FP::Install::DBTable_Add_App::ENTRY_EXTRAS)
    {
        // TODO: Handle extras
    }
    else
    {
        QFileInfo addAppInfo(appPath);
        taskQueue.push({taskType, CLIFP_PATH + '/' + addAppInfo.path(), addAppInfo.fileName(),
                        appArgs.split(" "), waitForExit ? ProcessType::Blocking : ProcessType::Deferred});

        // Add wait task if specified app uses a batch file
        if(addAppInfo.suffix() == ".bat")
            taskQueue.push({TaskType::Wait, QString(), BATCH_WAIT_EXE, QStringList(), ProcessType::Blocking});
    }
}

void enqueueShutdownTasks(std::queue<AppTask>& taskQueue, FP::Install::Services fpServices)
{
    // Add Stop entries from services
    for(const FP::Install::StartStop& stopEntry : fpServices.stops)
        taskQueue.push({TaskType::Shutdown, CLIFP_PATH + '/' + stopEntry.path, stopEntry.filename,
                        stopEntry.arguments, ProcessType::Blocking});
}

ErrorCode processTaskQueue(std::queue<AppTask>& taskQueue, QList<QProcess*>& childProcesses)
{
    // Exhaust queue
    for(unsigned long long i = 0; i < taskQueue.size(); i++)
    {
        // Handle task at front of queue
        AppTask currentTask = taskQueue.front();

        // Handle general and special case(s)
        if(currentTask.path == FP::Install::DBTable_Add_App::ENTRY_MESSAGE) // Message (currently ignores process type)
            QMessageBox::information(nullptr, QCoreApplication::applicationName(), currentTask.param.first());
        else if(currentTask.path == FP::Install::DBTable_Add_App::ENTRY_EXTRAS) // Extra
        {
            // TODO handle extra
        }
        else if(currentTask.type == TaskType::Wait) // Wait task
        {
            ErrorCode waitError = waitOnProcess(currentTask.filename);
            if(waitError != NO_ERR)
                return waitError;
        }
        else // General case
        {
            // Move to executable directory
            QDir::setCurrent(CLIFP_PATH + "/" + currentTask.path);

            // Create process handle
            QProcess* taskProcess = new QProcess();
            taskProcess->setProgram(currentTask.filename);
            taskProcess->setArguments(currentTask.param);
            taskProcess->setStandardOutputFile(QProcess::nullDevice()); // Don't inhert console window
            taskProcess->setStandardErrorFile(QProcess::nullDevice()); // Don't inhert console window

            // Cover each process type
            switch(currentTask.processType)
            {
                case ProcessType::Blocking:
                    if(!cleanStartProcess(taskProcess, currentTask.path))
                        return START_ERROR;

                    taskProcess->waitForFinished(-1); // Wait for process to end, don't time out
                    delete taskProcess; // Clear finished process handle from heap
                    break;

                case ProcessType::Deferred:
                    if(!cleanStartProcess(taskProcess, currentTask.path))
                        return START_ERROR;

                    childProcesses.append(taskProcess); // Add process to list for deferred termination
                    break;

                case ProcessType::Detached:
                    if(!taskProcess->startDetached())
                        return START_ERROR;
                    break;
            }
        }

        // Remove handled task
        taskQueue.pop();
    }

    // Return success
    return NO_ERR;
}

bool cleanStartProcess(QProcess* process, QString programPath)
{
    // Start process and confirm
    process->start();
    if(!process->waitForStarted())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(CLIFP_PATH + "/" +
                                                                               programPath+ "/" +
                                                                               process->program())));
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

    // Kill each remaining child process by destroying handle
    for(QProcess* childProcess : childProcesses)
        delete  childProcess;
}

ErrorCode startupProcedure()
{
    // Go to Server directory
    QDir::setCurrent(CLIFP_PATH + "/" + QFileInfo(PHP_EXE_PATH).dir().path());

    // Initialize php data
    if(QProcess::execute(QFileInfo(PHP_EXE_PATH).fileName(), PHP_ARGS_STARTUP) < 0)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(CLIFP_PATH + "/" + PHP_EXE_PATH)));
        return CORE_APP_NOT_STARTED;
    }

    // Start core background processes
    QProcess* httpdProcess = new QProcess(); // Don't delete since this kills the child process and auto deallocation on exit is fine here
    httpdProcess->start(QFileInfo(HTTPD_EXE_PATH).fileName(), HTTPD_ARGS_STARTUP);
    if(!httpdProcess->waitForStarted())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(CLIFP_PATH + "/" + HTTPD_EXE_PATH)));
        return CORE_APP_NOT_STARTED;
    }

    return NO_ERR;
}

ErrorCodes shutdownProcedure(bool silent)
{
    ErrorCodes shutdownErrors = NO_ERR;

    // Reset php data
    shutdownErrors |= shutdownApplication(PHP_EXE_PATH, PHP_ARGS_SHUTDOWN, silent);

    // Quit Redirector
    shutdownErrors |= shutdownApplication(REDIRECTOR_EXE_PATH, REDIRECTOR_ARGS_SHUTDOWN, silent);

    // Quit Fiddler
    shutdownErrors |= shutdownApplication(FIDDLER_EXE_PATH, FIDDLER_ARGS_SHUTDOWN, silent);

    return shutdownErrors;
}

ErrorCode shutdownApplication(QString exePath, QStringList shutdownArgs, bool& silent)
{
    // Go to app directory
    QDir::setCurrent(CLIFP_PATH + "/" + QFileInfo(exePath).path());

    // Shutdown app
    if(QProcess::execute(QFileInfo(exePath).fileName(), shutdownArgs) < 0)
    {
        if(!silent)
        {
            QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                  ERR_EXE_NOT_STARTED.arg(QDir::toNativeSeparators(CLIFP_PATH + "/" + exePath)));
            silent = true;
        }
        return CORE_APP_NOT_STARTED_FOR_SHUTDOWN;
    }

    return NO_ERR;
}

ErrorCode primaryApplicationExecution(QFile& primaryApp, QStringList primaryAppParameters)
{
    // Ensure primary app exists
    QString fullAppPath = QDir::toNativeSeparators(CLIFP_PATH + "/" + primaryApp.fileName());

    if(!QFileInfo::exists(fullAppPath) || !QFileInfo(fullAppPath).isFile())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              ERR_EXE_NOT_FOUND.arg(fullAppPath));
        return PRIMARY_APP_NOT_FOUND;
    }

    // Go to primary app directory
    QDir::setCurrent(QFileInfo(fullAppPath).dir().absolutePath());

    // Start primary application
    if(QProcess::execute(QFileInfo(primaryApp).fileName(), primaryAppParameters) < 0)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              ERR_EXE_NOT_STARTED.arg(fullAppPath));
        return PRIMARY_APP_NOT_STARTED;
    }

    return NO_ERR;
}

ErrorCode waitOnProcess(QString processName)
{
    // Find process ID by name
    DWORD processID = Qx::getProcessIDByName(processName);

    // Check that process was found
    if(processID)
    {
        QMessageBox::warning(nullptr, QCoreApplication::applicationName(), WRN_BATCH_WAIT_PROCESS_NOT_FOUND);
        return BATCH_PROCESS_NOT_FOUND;
    }

    // Get process handle and see if it is valid
    HANDLE batchProcessHandle;
    if((batchProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, processID)) == NULL)
    {
        QMessageBox::warning(nullptr, QCoreApplication::applicationName(), WRN_BATCH_WAIT_PROCESS_NOT_HANDLED);
        return BATCH_PROCESS_NOT_HANDLED;
    }

    // Attempt to wait on process to terminate
    DWORD waitError = WaitForSingleObject(batchProcessHandle, INFINITE);

    // Close handle to process
    CloseHandle(batchProcessHandle);

    if(waitError == WAIT_OBJECT_0)
        return NO_ERR;
    else
    {
        QMessageBox::warning(nullptr, QCoreApplication::applicationName(), WRN_BATCH_WAIT_PROCESS_NOT_HOOKED);
        return BATCH_PROCESS_NOT_HOOKED;
    }
}

