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
enum ErrorCode //TODO: RE-DISTRIBUTE VALUES
{
    NO_ERR = 0x00,
    INVALID_ARGS = 0x02,
    INSTALL_INVALID = 0x04,
    CANT_PARSE_CONFIG = 0x000000,
    CANT_PARSE_SERVICES = 0x0000000,
    CONFIG_SERVER_MISSING = 0x0000000,
    AUTO_ID_NOT_VALID = 0x00000000,
    CORE_APP_NOT_FOUND = 0x08,
    CORE_APP_NOT_STARTED = 0x10,
    PRIMARY_APP_NOT_FOUND = 0x20,
    PRIMARY_APP_NOT_STARTED = 0x40,
    CORE_APP_NOT_STARTED_FOR_SHUTDOWN = 0x80,
    BATCH_PROCESS_NOT_FOUND = 0x100,
    BATCH_PROCESS_NOT_HANDLED = 0x200,
    BATCH_PROCESS_NOT_HOOKED = 0x400
};
Q_DECLARE_FLAGS(ErrorCodes, ErrorCode)
Q_DECLARE_OPERATORS_FOR_FLAGS(ErrorCodes);

enum class OperationMode { Invalid, Normal, Auto, Message, Information };
enum class TaskType { Startup, Primary, Auxiliary, Wait, Shutdown };

//-Structs---------------------------------------------------------------------
struct AppTask
{
    TaskType type;
    QString path;
    QString filename;
    QStringList param;
    bool waitForExit;
    bool killOnExit;
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
const QString ERR_INSTALL_INVALID = "CLIFp does not appear to be deployed in a valid Flashpoint install. Check its location and compatability with "
                                    "your Flashpoint version.";

const QString ERR_CANT_PARSE_FILE = "Failed to parse %1 ! It may be corrupted or not compatible with this version of CLIFp.";
const QString ERR_CONFIG_SERVER_MISSING = "The server specified in the Flashpoint config was not found within the Flashpoint services store.";


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
ErrorCode enqueueStartupTasks(std::queue<AppTask>& taskQueue, FP::Install::Config fpConfig, FP::Install::Services fpServices);
ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, const FP::Install& fpInstall);

ErrorCode startupProcedure();
ErrorCodes shutdownProcedure(bool silent);
ErrorCode shutdownApplication(QString exePath, QStringList shutdownArgs, bool& silent);
ErrorCode primaryApplicationExecution(QFile& primaryApp, QStringList primaryAppParameters);
ErrorCode waitOnBatchProcess();

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
            appTaskQueue.push({TaskType::Primary, appInfo.absolutePath(), appInfo.fileName(), clParser.value(CL_OPTION_PARAM).split(" "), true, false});
            break;

        case OperationMode::Auto:
            if((autoID = QUuid(clParser.value(CL_OPTION_AUTO))).isNull())
                return AUTO_ID_NOT_VALID;

            if((enqueueError = enqueueStartupTasks(appTaskQueue, flashpointConfig, flashpointServices)) != NO_ERR)
                return enqueueError;

            if((enqueueError = enqueueAutomaticTasks(appTaskQueue, flashpointInstall)) != NO_ERR)
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


    // NEW CODE...

    // Handle primary CLI options
    QFile primaryApp(clParser.value(CL_OPTION_APP));
    QString primaryAppParam(clParser.value(CL_OPTION_PARAM));

    //-Check for existance of required core applications-----------------------------------
    for(QString coreApp : CORE_APP_PATHS)
    {
         QString fullAppPath = QDir::toNativeSeparators(CLIFP_PATH + "/" + coreApp);
         if(!QFileInfo::exists(fullAppPath) || !QFileInfo(fullAppPath).isFile())
         {
             QMessageBox::critical(nullptr, QCoreApplication::applicationName(), ERR_EXE_NOT_FOUND.arg(fullAppPath));
             return CORE_APP_NOT_FOUND;
         }
    }

    // Working variables
    ErrorCodes currentStatus = ErrorCode::NO_ERR;

    //-Startup Procedure-------------------------------------------------------------------
    currentStatus = startupProcedure();

    //-Primary Application-------------------------------------------------------------------
    if(currentStatus == NO_ERR)
    {
        currentStatus |= primaryApplicationExecution(primaryApp, primaryAppParam.split(" "));

        //-Wait If Batch-------------------------------------------------------------------------
        if(currentStatus == NO_ERR && QFileInfo(primaryApp).suffix() == ".bat")
             currentStatus |= waitOnBatchProcess();
    }

    //-Shutdown Procedure--------------------------------------------------------------------
    currentStatus |= shutdownProcedure(currentStatus != NO_ERR);

    // Close interface
    return currentStatus;
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

ErrorCode enqueueStartupTasks(std::queue<AppTask>& taskQueue, FP::Install::Config fpConfig, FP::Install::Services fpServices)
{
    // Add Start entries from services
    for(const FP::Install::StartStop& startEntry : fpServices.starts)
        taskQueue.push({TaskType::Startup, CLIFP_PATH + '/' + startEntry.path, startEntry.filename, startEntry.arguments, true, false});

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
                        configuredServer.arguments, false, configuredServer.kill});
    }

    // Return success
    return NO_ERR;
}

ErrorCode enqueueAutomaticTasks(std::queue<AppTask>& taskQueue, const FP::Install& fpInstall)
{

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

ErrorCode waitOnBatchProcess()
{
    // Find process ID by name
    DWORD processID = Qx::getProcessIDByName(BATCH_WAIT_EXE);

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

