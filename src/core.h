#ifndef CORE_H
#define CORE_H

// Standard Library Includes
#include <queue>

// Qt Includes
#include <QString>
#include <QList>
#include <QDir>
#include <QUrl>
#include <QCommandLineParser>
#include <QMessageBox>

// External Includes
#include "magic_enum.hpp"

// Project Includes
#include "fp/flashpoint/fp-install.h"
#include "logger.h"
#include "project_vars.h"

//-Macros----------------------------------------------------------------------
#define ENUM_NAME(eenum) QString::fromStdString(std::string(magic_enum::enum_name(eenum)))
#define CLIFP_DIR_PATH QCoreApplication::applicationDirPath()

//-Typedef---------------------------------------------------------------------
typedef int ErrorCode;

class Core : public QObject
{
    Q_OBJECT;
//-Class Enums-----------------------------------------------------------------------
public:
    enum class NotificationVerbosity { Full, Quiet, Silent };
    enum class TaskStage { Startup, Primary, Auxiliary, Shutdown };
    enum class ProcessType { Blocking, Deferred, Detached };

//-Class Structs---------------------------------------------------------------------
public:
    struct Error
    {
        QString source;
        Qx::GenericError errorInfo;
    };

    struct BlockingError
    {
        QString source;
        Qx::GenericError errorInfo;
        QMessageBox::StandardButtons choices;
        QMessageBox::StandardButton defaultChoice;
    };

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
            ml.append(".path = \"" + QDir::toNativeSeparators(path) + "\"");
            ml.append(".filename = \"" + filename + "\"");
            ml.append(".param = {\"" + param.join(R"(", ")") + "\"}");
            ml.append(".nativeParam = \"" + nativeParam + "\"");
            ml.append(".processType = " + ENUM_NAME(processType));
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
            ml.append(".message = \"" + message + "\"");
            ml.append(".modal = \"" + QString(modal ? "true" : "false"));
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
            ml.append(".extraDir = \"" + QDir::toNativeSeparators(dir.path()) + "\"");
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
            ml.append(".processName = \"" + processName + "\"");
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
            ml.append(".destPath = \"" + QDir::toNativeSeparators(destPath) + "\"");
            ml.append(".destFileName = \"" + destFileName + "\"");
            ml.append(".targetFile = \"" + targetFile.toString() + "\"");
            ml.append(".sha256 = " + sha256);
            return ml;
        }
    };

//-Inner Classes--------------------------------------------------------------------------------------------------------
public:
    class ErrorCodes
    {
    //-Class Variables--------------------------------------------------------------------------------------------------
    public:
        static const ErrorCode NO_ERR = 0;
        static const ErrorCode ALREADY_OPEN = 1;
        static const ErrorCode INVALID_ARGS = 2;
        static const ErrorCode LAUNCHER_OPEN = 3;
        static const ErrorCode INSTALL_INVALID = 4;
        static const ErrorCode CONFIG_SERVER_MISSING = 5;
        static const ErrorCode SQL_ERROR = 6;
        static const ErrorCode SQL_MISMATCH = 7;
        static const ErrorCode EXECUTABLE_NOT_FOUND = 8;
        static const ErrorCode EXECUTABLE_NOT_VALID = 9;
        static const ErrorCode PROCESS_START_FAIL = 10;
        static const ErrorCode WAIT_PROCESS_NOT_HANDLED = 11;
        static const ErrorCode WAIT_PROCESS_NOT_HOOKED = 12;
        static const ErrorCode CANT_READ_BAT_FILE = 13;
        static const ErrorCode ID_NOT_VALID = 14;
        static const ErrorCode ID_NOT_FOUND = 15;
        static const ErrorCode ID_DUPLICATE = 16;
        static const ErrorCode TITLE_NOT_FOUND = 17;
        static const ErrorCode CANT_OBTAIN_DATA_PACK = 18;
        static const ErrorCode DATA_PACK_INVALID = 19;
        static const ErrorCode EXTRA_NOT_FOUND = 20;
    };

//-Class Variables------------------------------------------------------------------------------------------------------
public:
    // Status
    static inline const QString STATUS_DISPLAY = "Displaying";
    static inline const QString STATUS_DISPLAY_HELP = "Help";
    static inline const QString STATUS_DISPLAY_VERSION = "Version";

    // Error Messages - Prep
    static inline const QString ERR_UNEXPECTED_SQL = "Unexpected SQL error while querying the Flashpoint database:";
    static inline const QString ERR_SQL_MISMATCH = "Received a different form of result from an SQL query than expected.";
    static inline const QString ERR_CONFIG_SERVER_MISSING = "The server specified in the Flashpoint config was not found within the Flashpoint services store.";
    static inline const QString ERR_ID_INVALID = "The provided string was not a valid GUID/UUID.";
    static inline const QString ERR_ID_NOT_FOUND = "An entry matching the specified ID could not be found in the Flashpoint database.";
    static inline const QString ERR_TITLE_NOT_FOUND = "The provided title was not found in the Flashpoint database.";
    static inline const QString WRN_EXIST_PACK_SUM_MISMATCH = "The existing Data Pack of the selected title does not contain the data expected. It will be re-downloaded.";

    // Logging - Primary Labels
    static inline const QString COMMAND_LABEL = "Command: %1";
    static inline const QString COMMAND_OPT_LABEL = "Command Options: %1";

    // Logging - Primary Values
    static inline const QString LOG_FILE_NAME = "CLIFp.log";
    static inline const QString LOG_HEADER = "CLIFp Execution Log";
    static inline const QString LOG_NO_PARAMS = "*None*";
    static inline const int LOG_MAX_ENTRIES = 50;

    // Logging - Errors
    static inline const QString LOG_ERR_INVALID_PARAM = "Invalid parameters provided";
    static inline const QString LOG_ERR_CRITICAL = "Aborting execution due to previous critical errors";

    // Logging - Messages
    static inline const QString LOG_EVENT_INIT = "Initializing CLIFp...";
    static inline const QString LOG_EVENT_G_HELP_SHOWN = "Displayed general help information";
    static inline const QString LOG_EVENT_VER_SHOWN = "Displayed version information";
    static inline const QString LOG_EVENT_NOTIFCATION_LEVEL = "Notification Level is: %1";
    static inline const QString LOG_EVENT_ENQ_START = "Enqueuing startup tasks...";
    static inline const QString LOG_EVENT_ENQ_STOP = "Enqueuing shutdown tasks...";
    static inline const QString LOG_EVENT_ENQ_DATA_PACK = "Enqueuing Data Pack tasks...";
    static inline const QString LOG_EVENT_DATA_PACK_MISS = "Title Data Pack is not available locally";
    static inline const QString LOG_EVENT_DATA_PACK_FOUND = "Title Data Pack with correct hash is already present, no need to download";
    static inline const QString LOG_EVENT_TASK_ENQ = "Enqueued %1: {%2}";
    static inline const QString LOG_EVENT_TITLE_ID_COUNT = "Found %1 ID(s) when searching for title %2";
    static inline const QString LOG_EVENT_TITLE_SEL_PROMNPT = "Prompting user to disambiguate multiple IDs...";
    static inline const QString LOG_EVENT_TITLE_ID_DETERMINED = "ID of title %1 determined to be %2";

    // Global command line option strings
    static inline const QString CL_OPT_HELP_S_NAME = "h";
    static inline const QString CL_OPT_HELP_L_NAME = "help";
    static inline const QString CL_OPT_HELP_E_NAME = "?";
    static inline const QString CL_OPT_HELP_DESC = "Prints this help message.";

    static inline const QString CL_OPT_VERSION_S_NAME = "v";
    static inline const QString CL_OPT_VERSION_L_NAME = "version";
    static inline const QString CL_OPT_VERSION_DESC = "Prints the current version of this tool.";

    static inline const QString CL_OPT_QUIET_S_NAME = "q";
    static inline const QString CL_OPT_QUIET_L_NAME = "quiet";
    static inline const QString CL_OPT_QUIET_DESC = "Silences all non-critical messages.";

    static inline const QString CL_OPT_SILENT_S_NAME = "s";
    static inline const QString CL_OPT_SILENT_L_NAME = "silent";
    static inline const QString CL_OPT_SILENT_DESC = "Silences all messages (takes precedence over quiet mode).";

    // Global command line options
    static inline const QCommandLineOption CL_OPTION_HELP{{CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_VERSION{{CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_QUIET{{CL_OPT_QUIET_S_NAME, CL_OPT_QUIET_L_NAME}, CL_OPT_QUIET_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_SILENT{{CL_OPT_SILENT_S_NAME, CL_OPT_SILENT_L_NAME}, CL_OPT_SILENT_DESC}; // Boolean option

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_ALL{&CL_OPTION_HELP, &CL_OPTION_VERSION, &CL_OPTION_QUIET, &CL_OPTION_SILENT};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_ACTIONABLE{&CL_OPTION_HELP, &CL_OPTION_VERSION};

    // Help template
    static inline const QString HELP_TEMPL = "<u>Usage:</u><br>"
                                             PROJECT_SHORT_NAME "&lt;global options&gt; <i>command</i> &lt;command options&gt;<br>"
                                             "<br>"
                                             "<u>Global Options:</u>%1<br>"
                                             "<br>"
                                             "<u>Commands:</u>%2<br>"
                                             "<br>"
                                             "Use the <b>-h</b> switch after a command to see it's specific usage notes";
    static inline const QString HELP_OPT_TEMPL = "<br><b>%1:</b> &nbsp;%2";
    static inline const QString HELP_COMMAND_TEMPL = "<br><b>%1:</b> &nbsp;%2";

    // Command line messages
    static inline const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " PROJECT_VERSION_STR ", designed for use with BlueMaxima's Flashpoint " PROJECT_TARGET_FP_VER_STR "+";

    // Input strings
    static inline const QString MULTI_TITLE_SEL_CAP = "Title Disambiguation";
    static inline const QString MULTI_TITLE_SEL_LABEL = "Title to start:";
    static inline const QString MULTI_TITLE_SEL_TEMP = "[%1] %2 (%3) {%4}";

    // Meta
    static inline const QString NAME = "core";

//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    // For log
    QString mRawCommandLine;

    // Handles
    std::unique_ptr<Fp::Install> mFlashpointInstall;
    std::unique_ptr<QFile> mLogFile;
    std::unique_ptr<Logger> mLogger;

    // Processing
    bool mCriticalErrorOccured = false;
    NotificationVerbosity mNotificationVerbosity;
    std::queue<std::shared_ptr<Task>> mTaskQueue;

    // Info
    QString mStatusHeading;
    QString mStatusMessage;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    explicit Core(QObject* parent, QString rawCommandLineParam);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    bool isActionableOptionSet(const QCommandLineParser& clParser) const;
    void showHelp();
    void showVersion();

public:
    ErrorCode initialize(QStringList& commandLine);
    void attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall);

    ErrorCode getGameIDFromTitle(QUuid& returnBuffer, QString title);

    ErrorCode enqueueStartupTasks();
    void enqueueShutdownTasks();
    ErrorCode enqueueConditionalWaitTask(QFileInfo precedingAppInfo);
    ErrorCode enqueueDataPackTasks(QUuid targetID);
    void enqueueSingleTask(std::shared_ptr<Task> task);
    void clearTaskQueue();

    void logCommand(QString src, QString commandName);
    void logCommandOptions(QString src, QString commandOptions);
    void logError(QString src, Qx::GenericError error);
    void logEvent(QString src, QString event);
    void logTask(QString src, const Task* const task);
    int logFinish(QString src, int exitCode);
    void postError(QString src, Qx::GenericError error, bool log = true);
    int postBlockingError(QString src, Qx::GenericError error, bool log = true, QMessageBox::StandardButtons bs = QMessageBox::Ok, QMessageBox::StandardButton def = QMessageBox::NoButton);
    void postMessage(QString msg);

    Fp::Install& getFlashpointInstall();
    NotificationVerbosity notifcationVerbosity() const;
    size_t taskCount() const;
    bool hasTasks() const;
    std::shared_ptr<Task> takeFrontTask();

    QString statusHeading();
    QString statusMessage();
    void setStatus(QString heading, QString message);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void statusChanged(const QString& statusHeading, const QString& statusMessage);
    void errorOccured(const Core::Error& error);
    void blockingErrorOccured(QSharedPointer<int> response, const Core::BlockingError& blockingError);
    void message(const QString& message);
};

//-Metatype Declarations-----------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(Core::Error);
Q_DECLARE_METATYPE(Core::BlockingError);

#endif // CORE_H
