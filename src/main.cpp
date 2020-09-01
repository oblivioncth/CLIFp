#include "version.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QCommandLineParser>
#include <QDebug>
#include "qx-windows.h"

//-Enums-----------------------------------------------------------------------
enum ErrorCode
{
    NO_ERR = 0x00,
    CORE_APP_NOT_FOUND = 0x02,
    CORE_APP_NOT_STARTED = 0x04,
    PRIMARY_APP_NOT_FOUND = 0x08,
    PRIMARY_APP_NOT_STARTED = 0x10,
    CORE_APP_NOT_STARTED_FOR_SHUTDOWN = 0x20,
    BATCH_PROCESS_NOT_FOUND = 0x40,
    BATCH_PROCESS_NOT_HANDLED = 0x80,
    BATCH_PROCESS_NOT_HOOKED = 0x100
};
Q_DECLARE_FLAGS(ErrorCodes, ErrorCode)
Q_DECLARE_OPERATORS_FOR_FLAGS(ErrorCodes);

//-Constants-------------------------------------------------------------------

// Command line strings
const QString CL_OPTION_HELP_SHORT_NAME = "h";
const QString CL_OPTION_HELP_LONG_NAME = "help";
const QString CL_OPTION_HELP_EXTRA_NAME = "?";
const QString CL_OPTION_HELP_DESCRIPTION = "Prints this help message";

const QString CL_OPTION_VERSION_SHORT_NAME = "v";
const QString CL_OPTION_VERSION_LONG_NAME = "version";
const QString CL_OPTION_VERSION_DESCRIPTION = "Prints the current version of this tool";

const QString CL_OPTION_APP_SHORT_NAME = "a";
const QString CL_OPTION_APP_LONG_NAME = "app";
const QString CL_OPTION_APP_DESCRIPTION = "Path of primary application to launch";

const QString CL_OPTION_PARAM_SHORT_NAME = "p";
const QString CL_OPTION_PARAM_LONG_NAME = "param";
const QString CL_OPTION_PARAM_DESCRIPTION = "Command-line parameters to use when starting the primary application";

const QString CL_OPTION_MSG_SHORT_NAME = "m";
const QString CL_OPTION_MSG_LONG_NAME = "msg";
const QString CL_OPTION_MSG_DESCRIPTION = "Displays an pop-up dialog with the supplied message. Used primarily for some additional apps";


// Command line messages
const QString CL_HELP_MESSAGE = "CLIFp Usage:\n"
                                "\n"
                                "-" + CL_OPTION_HELP_SHORT_NAME + " | --" + CL_OPTION_HELP_LONG_NAME + " | -" + CL_OPTION_HELP_EXTRA_NAME + ": " + CL_OPTION_HELP_DESCRIPTION + "\n"
                                "-" + CL_OPTION_VERSION_SHORT_NAME + " | --" + CL_OPTION_VERSION_LONG_NAME + ": " + CL_OPTION_VERSION_DESCRIPTION + "\n"
                                "-" + CL_OPTION_APP_SHORT_NAME + " | --" + CL_OPTION_APP_LONG_NAME + ": " + CL_OPTION_APP_DESCRIPTION + "\n"
                                "-" + CL_OPTION_PARAM_SHORT_NAME + " | --" + CL_OPTION_PARAM_LONG_NAME + ": " + CL_OPTION_PARAM_DESCRIPTION + "\n"
                                "-" + CL_OPTION_MSG_SHORT_NAME + " | --" + CL_OPTION_MSG_LONG_NAME + ": " + CL_OPTION_MSG_DESCRIPTION + "\n"
                                "\n"
                                "The Application and Parameter options are required (unless using help, version, or msg). If the help or version options are specified, all other options are ignored.";

const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " VER_PRODUCTVERSION_STR ", designed for use with BlueMaxima's Flashpoint " VER_PRODUCTVERSION_STR;

// FP Server Applications
const QString PHP_EXE_PATH = "Server\\php.exe";
const QString HTTPD_EXE_PATH = "Server\\httpd.exe";
const QString BATCH_WAIT_EXE = "FlashpointSecurePlayer.exe";

// FP Server App Arguments
const QStringList PHP_ARGS_STARTUP = {"-f", "update_httpdconf_main_dir.php"};
const QStringList PHP_ARGS_SHUTDOWN = {"-f", "reset_httpdconf_main_dir.php"};
const QStringList HTTPD_ARGS_STARTUP = {"-f", "..//Server/conf/httpd.conf", "-X"};

// FPSoftware Applications
const QString FIDDLER_EXE_PATH = "FPSoftware\\Fiddler2Portable\\App\\Fiddler\\ExecAction.exe";
const QString REDIRECTOR_EXE_PATH = "FPSoftware\\Redirector\\Redirect.exe";

// FPSoftware App Arguments
const QStringList FIDDLER_ARGS_SHUTDOWN = {"quit"};
const QStringList REDIRECTOR_ARGS_SHUTDOWN = {"/close"};

// Core Application Paths
const QStringList CORE_APP_PATHS = {PHP_EXE_PATH, HTTPD_EXE_PATH};

// Error Messages
const QString EXE_NOT_FOUND_ERROR = "Could not find %1!\n\nExecution will now be aborted.";
const QString EXE_NOT_STARTED_ERROR = "Could not start %1!\n\nExecution will now be aborted.";
const QString BATCH_ERR_SUFFIX = "after a primary application utilizing a batch file was started. The game may not work correctly.";
const QString BATCH_WAIT_PROCESS_NOT_FOUND_ERROR  = "Could not find the wait-on process to hook to " + BATCH_ERR_SUFFIX;
const QString BATCH_WAIT_PROCESS_NOT_HANDLED_ERROR  = "Could not get a handle to the wait-on process " + BATCH_ERR_SUFFIX;
const QString BATCH_WAIT_PROCESS_NOT_HOOKED_ERROR  = "Could not hook the wait-on process " + BATCH_ERR_SUFFIX;

// Working variables
ErrorCodes currentStatus = ErrorCode::NO_ERR;

// Prototypes
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

    // CLI Options
    QCommandLineOption clOptionApp({CL_OPTION_APP_SHORT_NAME, CL_OPTION_APP_LONG_NAME}, CL_OPTION_APP_DESCRIPTION, "application"); // Takes value
    QCommandLineOption clOptionParam({CL_OPTION_PARAM_SHORT_NAME, CL_OPTION_PARAM_LONG_NAME}, CL_OPTION_PARAM_DESCRIPTION, "parameters"); // Takes value
    QCommandLineOption clOptionMsg({CL_OPTION_MSG_SHORT_NAME, CL_OPTION_MSG_LONG_NAME}, CL_OPTION_MSG_DESCRIPTION, "message"); // Takes value
    QCommandLineOption clOptionHelp({CL_OPTION_HELP_SHORT_NAME, CL_OPTION_HELP_LONG_NAME, CL_OPTION_HELP_EXTRA_NAME}, CL_OPTION_HELP_DESCRIPTION); // Boolean option
    QCommandLineOption clOptionVersion({CL_OPTION_VERSION_SHORT_NAME, CL_OPTION_VERSION_LONG_NAME}, CL_OPTION_VERSION_DESCRIPTION); // Boolean option

    // CLI Parser
    QCommandLineParser clParser;
    clParser.setApplicationDescription(VER_FILEDESCRIPTION_STR);
    clParser.addOptions({clOptionApp, clOptionParam, clOptionMsg, clOptionHelp, clOptionVersion});
    clParser.process(app);

    // Handle informative CLI options
    bool minimumRunArgsPresent = clParser.isSet(clOptionApp) && clParser.isSet(clOptionParam);
    bool partialRunArgsPresent = (clParser.isSet(clOptionApp) && !clParser.isSet(clOptionParam)) || (!clParser.isSet(clOptionApp) && clParser.isSet(clOptionParam));

    if(clParser.isSet(clOptionVersion))
        QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_VERSION_MESSAGE);

    if(clParser.isSet(clOptionHelp) || (!minimumRunArgsPresent && !clParser.isSet(clOptionMsg)) || partialRunArgsPresent)
        QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);

    if(clParser.isSet(clOptionMsg) && !clParser.isSet(clOptionHelp) && !clParser.isSet(clOptionVersion) && !partialRunArgsPresent)
        QMessageBox::information(nullptr, QCoreApplication::applicationName(), clParser.value(clOptionMsg));

    if(!minimumRunArgsPresent)
        return NO_ERR;

    // Handle primary CLI options
    QFile primaryApp(clParser.value(clOptionApp));
    QString primaryAppParam(clParser.value(clOptionParam));

    //-Check for existance of required core applications-----------------------------------
    for(QString coreApp : CORE_APP_PATHS)
    {
         QString fullAppPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + coreApp);
         if(!QFileInfo::exists(fullAppPath) || !QFileInfo(fullAppPath).isFile())
         {
             QMessageBox::critical(nullptr, QCoreApplication::applicationName(), EXE_NOT_FOUND_ERROR.arg(fullAppPath));
             return CORE_APP_NOT_FOUND;
         }
    }

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

ErrorCode startupProcedure()
{
    // Go to Server directory
    QDir::setCurrent(QCoreApplication::applicationDirPath() + "\\" + QFileInfo(PHP_EXE_PATH).dir().path());

    // Initialize php data
    if(QProcess::execute(QFileInfo(PHP_EXE_PATH).fileName(), PHP_ARGS_STARTUP) < 0)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + PHP_EXE_PATH)));
        return CORE_APP_NOT_STARTED;
    }

    // Start core background processes
    QProcess* httpdProcess = new QProcess(); // Don't delete since this kills the child process and auto deallocation on exit is fine here
    httpdProcess->start(QFileInfo(HTTPD_EXE_PATH).fileName(), HTTPD_ARGS_STARTUP);
    if(!httpdProcess->waitForStarted())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + HTTPD_EXE_PATH)));
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
    QDir::setCurrent(QCoreApplication::applicationDirPath() + "\\" + QFileInfo(exePath).absolutePath());

    // Shutdown app
    if(QProcess::execute(QFileInfo(exePath).fileName(), shutdownArgs) < 0)
    {
        if(!silent)
        {
            QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                  EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + exePath)));
            silent = true;
        }
        return CORE_APP_NOT_STARTED_FOR_SHUTDOWN;
    }

    return NO_ERR;
}

ErrorCode primaryApplicationExecution(QFile& primaryApp, QStringList primaryAppParameters)
{
    // Ensure primary app exists
    QString fullAppPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + primaryApp.fileName());

    if(!QFileInfo::exists(fullAppPath) || !QFileInfo(fullAppPath).isFile())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_FOUND_ERROR.arg(fullAppPath));
        return PRIMARY_APP_NOT_FOUND;
    }

    // Go to primary app directory
    QDir::setCurrent(QFileInfo(fullAppPath).dir().absolutePath());

    // Start primary application
    if(QProcess::execute(QFileInfo(primaryApp).fileName(), primaryAppParameters) < 0)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_STARTED_ERROR.arg(fullAppPath));
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
        QMessageBox::warning(nullptr, QCoreApplication::applicationName(), BATCH_WAIT_PROCESS_NOT_FOUND_ERROR);
        return BATCH_PROCESS_NOT_FOUND;
    }

    // Get process handle and see if it is valid
    HANDLE batchProcessHandle;
    if((batchProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, processID)) == NULL)
    {
        QMessageBox::warning(nullptr, QCoreApplication::applicationName(), BATCH_WAIT_PROCESS_NOT_HANDLED_ERROR);
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
        QMessageBox::warning(nullptr, QCoreApplication::applicationName(), BATCH_WAIT_PROCESS_NOT_HOOKED_ERROR);
        return BATCH_PROCESS_NOT_HOOKED;
    }
}

