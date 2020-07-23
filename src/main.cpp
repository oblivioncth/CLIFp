#include "version.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QCommandLineParser>
#include <QDebug>

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


// Command line messages
const QString CL_HELP_MESSAGE = "CLIFp Usage:\n"
                              "\n"
                              "-" + CL_OPTION_HELP_SHORT_NAME + " | --" + CL_OPTION_HELP_LONG_NAME + " | -" + CL_OPTION_HELP_EXTRA_NAME + ": " + CL_OPTION_HELP_DESCRIPTION + "\n"
                              "-" + CL_OPTION_VERSION_SHORT_NAME + " | --" + CL_OPTION_VERSION_LONG_NAME + ": " + CL_OPTION_VERSION_DESCRIPTION + "\n"
                              "-" + CL_OPTION_APP_SHORT_NAME + " | --" + CL_OPTION_APP_LONG_NAME + ": " + CL_OPTION_APP_DESCRIPTION + "\n"
                              "-" + CL_OPTION_PARAM_SHORT_NAME + " | --" + CL_OPTION_PARAM_LONG_NAME + ": " + CL_OPTION_PARAM_DESCRIPTION + "\n"
                              "\n"
                              "The Application and Parameter options are required (unless using help, version, or msg). If the help or version options are specified, all other options are ignored.";

const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " VER_PRODUCTVERSION_STR ", designed for use with BlueMaxima's Flashpoint " VER_PRODUCTVERSION_STR;

// FP Server Applications
const QString PHP_EXE_PATH = "Server\\php.exe";
const QString HTTPD_EXE_PATH = "Server\\httpd.exe";
const QString TAIL_EXE_PATH = "Server\\tail.exe";

// FP Server App Arguments
const QStringList PHP_ARGS_STARTUP = {"-f", "update_httpdconf_main_dir.php"};
const QStringList PHP_ARGS_SHUTDOWN = {"-f", "reset_httpdconf_main_dir.php"};
const QStringList TAILS_ARGS_STARTUP = {"-n0", "-s1", "-f", "logs/access.log"};

// FPSoftware Applications
const QString EXEC_ACTION_EXE_PATH = "FPSoftware\\Fiddler2Portable\\App\\Fiddler\\ExecAction.exe";

// FPSoftware App Arguments
const QStringList EXEC_ACTION_ARGS_SHUTDOWN = {"quit"};

// Core Application Paths
const QStringList CORE_APP_PATHS = {PHP_EXE_PATH, HTTPD_EXE_PATH, TAIL_EXE_PATH, EXEC_ACTION_EXE_PATH};

// Error Messages
const QString EXE_NOT_FOUND_ERROR = "Could not find %1!\n\nExecution will now be aborted.";
const QString EXE_NOT_STARTED_ERROR = "Could not start %1!\n\nExecution will now be aborted.";

// Return Codes
namespace RETCODE
{
    const int SUCCESS = 0;
    const int CORE_APP_NOT_FOUND = 1;
    const int CORE_APP_NOT_STARTED = 2;
    const int PRIMARY_APP_NOT_FOUND = 3;
    const int PRIMARY_APP_NOT_STARTED = 4;
}

// Prototypes
int startupProcedure();
int shutdownProcedure(bool silent);
int primaryApplicationExecution(QFile& primaryApp, QStringList primaryAppParameters);

int main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------

    // QApplication Object
    QApplication app(argc, argv);

    QStringList test(EXEC_ACTION_ARGS_SHUTDOWN);

    // Set application name
    QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
    QCoreApplication::setApplicationVersion(VER_FILEVERSION_STR);

    // CLI Options
    QCommandLineOption clOptionApp({CL_OPTION_APP_SHORT_NAME, CL_OPTION_APP_LONG_NAME}, CL_OPTION_APP_DESCRIPTION, "application"); // Takes value
    QCommandLineOption clOptionParam({CL_OPTION_PARAM_SHORT_NAME, CL_OPTION_PARAM_LONG_NAME}, CL_OPTION_PARAM_DESCRIPTION, "parameters"); // Takes value
    QCommandLineOption clOptionHelp({CL_OPTION_HELP_SHORT_NAME, CL_OPTION_HELP_LONG_NAME, CL_OPTION_HELP_EXTRA_NAME}, CL_OPTION_HELP_DESCRIPTION); // Boolean option
    QCommandLineOption clOptionVersion({CL_OPTION_VERSION_SHORT_NAME, CL_OPTION_VERSION_LONG_NAME}, CL_OPTION_VERSION_DESCRIPTION); // Boolean option

    // CLI Parser
    QCommandLineParser clParser;
    clParser.setApplicationDescription(VER_FILEDESCRIPTION_STR);
    clParser.addOptions({clOptionApp, clOptionParam, clOptionHelp, clOptionVersion});
    clParser.process(app);

    // Handle informative CLI options

    if(clParser.isSet(clOptionVersion))
        QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_VERSION_MESSAGE);

    if(clParser.isSet(clOptionHelp) || !clParser.isSet(clOptionApp) || !clParser.isSet(clOptionParam))
        QMessageBox::information(nullptr, QCoreApplication::applicationName(), CL_HELP_MESSAGE);

    if(clParser.isSet(clOptionHelp) || !clParser.isSet(clOptionApp) || !clParser.isSet(clOptionParam) || clParser.isSet(clOptionVersion))
        return RETCODE::SUCCESS;

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
             return RETCODE::CORE_APP_NOT_FOUND;
         }
    }

    //-Startup Procedure-------------------------------------------------------------------
    int currentExitCode = startupProcedure();

    //-Primary Application-------------------------------------------------------------------
    if(currentExitCode == RETCODE::SUCCESS)
        currentExitCode = primaryApplicationExecution(primaryApp, primaryAppParam.split(" "));

    //-Shutdown Procedure--------------------------------------------------------------------
    int shutdownExitCode = shutdownProcedure(currentExitCode != RETCODE::SUCCESS);

    // Close interface
    if(currentExitCode != RETCODE::SUCCESS)
        return currentExitCode;
    else
        return shutdownExitCode;
}

int startupProcedure()
{
    // Go to Server directory
    QDir::setCurrent(QCoreApplication::applicationDirPath() + "\\" + QFileInfo(PHP_EXE_PATH).dir().path());

    // Initialize php data
    if(QProcess::execute(QFileInfo(PHP_EXE_PATH).fileName(), PHP_ARGS_STARTUP) < 0)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + PHP_EXE_PATH)));
        return RETCODE::CORE_APP_NOT_STARTED;
    }

    // Start core background processes
    QProcess* httpdProcess = new QProcess(); // Don't delete since this kills the child process and auto deallocation on exit is fine here
    httpdProcess->setProgram(QFileInfo(HTTPD_EXE_PATH).fileName());
    httpdProcess->start();
    if(!httpdProcess->waitForStarted())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + HTTPD_EXE_PATH)));
        return RETCODE::CORE_APP_NOT_STARTED;
    }

    QProcess* tailProcess = new QProcess(); // Don't delete since this kills the child process and auto deallocation on exit is fine here
    tailProcess->start(QFileInfo(TAIL_EXE_PATH).fileName(), TAILS_ARGS_STARTUP);
    if(!tailProcess->waitForStarted())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + TAIL_EXE_PATH)));
        return RETCODE::CORE_APP_NOT_STARTED;
    }

    return RETCODE::SUCCESS;
}

int shutdownProcedure(bool silent)
{
    // Go to Server directory
    QDir::setCurrent(QCoreApplication::applicationDirPath() + "\\" + QFileInfo(PHP_EXE_PATH).dir().path());

    // Reset php data
    if(QProcess::execute(QFileInfo(PHP_EXE_PATH).fileName(), PHP_ARGS_SHUTDOWN) < 0)
    {
        if(!silent)
            QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                  EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + PHP_EXE_PATH)));
        return RETCODE::CORE_APP_NOT_STARTED;
    }

    // Go to Fiddler ExecAction directory
    QDir::setCurrent(QCoreApplication::applicationDirPath() + "\\" + QFileInfo(EXEC_ACTION_EXE_PATH).dir().path());
    if(QProcess::execute(QFileInfo(EXEC_ACTION_EXE_PATH).fileName(), EXEC_ACTION_ARGS_SHUTDOWN) < 0)
    {
        if(!silent)
            QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                                  EXE_NOT_STARTED_ERROR.arg(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + EXEC_ACTION_EXE_PATH)));
        return RETCODE::CORE_APP_NOT_STARTED;
    }

    return RETCODE::SUCCESS;
}

int primaryApplicationExecution(QFile& primaryApp, QStringList primaryAppParameters)
{
    // Ensure primary app exists
    QString fullAppPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "\\" + primaryApp.fileName());

    if(!QFileInfo::exists(fullAppPath) || !QFileInfo(fullAppPath).isFile())
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_FOUND_ERROR.arg(fullAppPath));
        return RETCODE::PRIMARY_APP_NOT_FOUND;
    }

    // Go to primary app directory
    QDir::setCurrent(QFileInfo(fullAppPath).dir().absolutePath());

    // Start primary application
    if(QProcess::execute(QFileInfo(primaryApp).fileName(), primaryAppParameters) < 0)
    {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                              EXE_NOT_STARTED_ERROR.arg(fullAppPath));
        return RETCODE::PRIMARY_APP_NOT_STARTED;
    }

    return RETCODE::SUCCESS;
}
