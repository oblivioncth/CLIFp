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
#include <QFileDialog>

// Qx Includes
#include <qx/io/qx-applicationlogger.h>
#include <qx/utility/qx-macros.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "task/task.h"
#include "project_vars.h"

class Core : public QObject
{
    Q_OBJECT;
//-Class Enums-----------------------------------------------------------------------
public:
    enum class NotificationVerbosity { Full, Quiet, Silent };

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

    struct SaveFileRequest
    {
        QString caption;
        QString dir;
        QString filter;
        QString* selectedFilter = nullptr;
        QFileDialog::Options options;
    };

    struct ItemSelectionRequest
    {
        QString caption;
        QString label;
        QStringList items;
    };

//-Class Variables------------------------------------------------------------------------------------------------------
public:
    // Status
    static inline const QString STATUS_DISPLAY = QSL("Displaying");
    static inline const QString STATUS_DISPLAY_HELP = QSL("Help");
    static inline const QString STATUS_DISPLAY_VERSION = QSL("Version");

    // Error Messages - Prep
    static inline const QString ERR_UNEXPECTED_SQL = QSL("Unexpected SQL error while querying the Flashpoint database:");
    static inline const QString ERR_SQL_MISMATCH = QSL("Received a different form of result from an SQL query than expected.");
    static inline const QString ERR_CONFIG_SERVER_MISSING = QSL("The server specified in the Flashpoint config was not found within the Flashpoint services store.");
    static inline const QString ERR_ID_INVALID = QSL("The provided string was not a valid GUID/UUID.");
    static inline const QString ERR_ID_NOT_FOUND = QSL("An entry matching the specified ID could not be found in the Flashpoint database.");
    static inline const QString ERR_TITLE_NOT_FOUND = QSL("The provided title was not found in the Flashpoint database.");
    static inline const QString WRN_EXIST_PACK_SUM_MISMATCH = QSL("The existing Data Pack of the selected title does not contain the data expected. It will be re-downloaded.");

    // Error Messages - Helper
    static inline const QString ERR_FIND_TOO_MANY_RESULTS = QSL("Too many titles matched the title-based query.");

    // Logging - Primary Labels
    static inline const QString COMMAND_LABEL = QSL("Command: %1");
    static inline const QString COMMAND_OPT_LABEL = QSL("Command Options: %1");

    // Logging - Primary Values
    static inline const QString LOG_FILE_EXT = QSL("log");
    static inline const QString LOG_NO_PARAMS = QSL("*None*");
    static const int LOG_MAX_ENTRIES = 50;

    // Logging - Errors
    static inline const QString LOG_ERR_INVALID_PARAM = QSL("Invalid parameters provided");
    static inline const QString LOG_ERR_CRITICAL = QSL("Aborting execution due to previous critical errors");

    // Logging - Messages
    static inline const QString LOG_EVENT_INIT = QSL("Initializing CLIFp...");
    static inline const QString LOG_EVENT_GLOBAL_OPT = QSL("Global Options: %1");
    static inline const QString LOG_EVENT_G_HELP_SHOWN = QSL("Displayed general help information");
    static inline const QString LOG_EVENT_VER_SHOWN = QSL("Displayed version information");
    static inline const QString LOG_EVENT_NOTIFCATION_LEVEL = QSL("Notification Level is: %1");
    static inline const QString LOG_EVENT_ENQ_START = QSL("Enqueuing startup tasks...");
    static inline const QString LOG_EVENT_ENQ_STOP = QSL("Enqueuing shutdown tasks...");
    static inline const QString LOG_EVENT_ENQ_DATA_PACK = QSL("Enqueuing Data Pack tasks...");
    static inline const QString LOG_EVENT_DATA_PACK_MISS = QSL("Title Data Pack is not available locally");
    static inline const QString LOG_EVENT_DATA_PACK_FOUND = QSL("Title Data Pack with correct hash is already present, no need to download");
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_MOUNT = QSL("Title Data Pack requires mounting");
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_EXTRACT = QSL("Title Data Pack requires extraction");
    static inline const QString LOG_EVENT_TASK_ENQ = QSL("Enqueued %1: {%2}");
    static inline const QString LOG_EVENT_APP_PATH_ALT = QSL("App path \"%1\" maps to alternative \"%2\".");

    // Logging - Title Search
    static inline const QString LOG_EVENT_GAME_SEARCH = QSL("Searching for game with title '%1'");
    static inline const QString LOG_EVENT_ADD_APP_SEARCH = QSL("Searching for additional-app with title '%1' and parent %2");
    static inline const QString LOG_EVENT_TITLE_ID_COUNT = QSL("Found %1 ID(s) when searching for title %2");
    static inline const QString LOG_EVENT_TITLE_SEL_PROMNPT = QSL("Prompting user to disambiguate multiple IDs...");
    static inline const QString LOG_EVENT_TITLE_ID_DETERMINED = QSL("ID of title %1 determined to be %2");
    static inline const QString LOG_EVENT_TITLE_SEL_CANCELED = QSL("Title selection was canceled by the user.");

    // Global command line option strings
    static inline const QString CL_OPT_HELP_S_NAME = QSL("h");
    static inline const QString CL_OPT_HELP_L_NAME = QSL("help");
    static inline const QString CL_OPT_HELP_E_NAME = QSL("?");
    static inline const QString CL_OPT_HELP_DESC = QSL("Prints this help message.");

    static inline const QString CL_OPT_VERSION_S_NAME = QSL("v");
    static inline const QString CL_OPT_VERSION_L_NAME = QSL("version");
    static inline const QString CL_OPT_VERSION_DESC = QSL("Prints the current version of this tool.");

    static inline const QString CL_OPT_QUIET_S_NAME = QSL("q");
    static inline const QString CL_OPT_QUIET_L_NAME = QSL("quiet");
    static inline const QString CL_OPT_QUIET_DESC = QSL("Silences all non-critical messages.");

    static inline const QString CL_OPT_SILENT_S_NAME = QSL("s");
    static inline const QString CL_OPT_SILENT_L_NAME = QSL("silent");
    static inline const QString CL_OPT_SILENT_DESC = QSL("Silences all messages (takes precedence over quiet mode).");

    // Global command line options
    static inline const QCommandLineOption CL_OPTION_HELP{{CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_VERSION{{CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_QUIET{{CL_OPT_QUIET_S_NAME, CL_OPT_QUIET_L_NAME}, CL_OPT_QUIET_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_SILENT{{CL_OPT_SILENT_S_NAME, CL_OPT_SILENT_L_NAME}, CL_OPT_SILENT_DESC}; // Boolean option

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_ALL{&CL_OPTION_HELP, &CL_OPTION_VERSION, &CL_OPTION_QUIET, &CL_OPTION_SILENT};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_ACTIONABLE{&CL_OPTION_HELP, &CL_OPTION_VERSION};

    // Help template
    static inline const QString HELP_TEMPL = QSL("<u>Usage:</u><br>"
                                                 PROJECT_SHORT_NAME "&lt;global options&gt; <i>command</i> &lt;command options&gt;<br>"
                                                 "<br>"
                                                 "<u>Global Options:</u>%1<br>"
                                                 "<br>"
                                                 "<u>Commands:</u>%2<br>"
                                                 "<br>"
                                                 "Use the <b>-h</b> switch after a command to see it's specific usage notes");
    static inline const QString HELP_OPT_TEMPL = QSL("<br><b>%1:</b> &nbsp;%2");
    static inline const QString HELP_COMMAND_TEMPL = QSL("<br><b>%1:</b> &nbsp;%2");

    // Command line messages
    static inline const QString CL_VERSION_MESSAGE = QSL("CLI Flashpoint version " PROJECT_VERSION_STR ", designed for use with BlueMaxima's Flashpoint " PROJECT_TARGET_FP_VER_PFX_STR " series");

    // Input strings
    static inline const QString MULTI_TITLE_SEL_CAP = QSL("Title Disambiguation");
    static inline const QString MULTI_TITLE_SEL_LABEL = QSL("Title to start:");
    static inline const QString MULTI_GAME_SEL_TEMP = QSL("[%1] %2 (%3) {%4}");
    static inline const QString MULTI_ADD_APP_SEL_TEMP = QSL("%1 {%2}");

    // Helper
    static const int FIND_ENTRY_LIMIT = 20;

    // Meta
    static inline const QString NAME = QSL("core");

//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    // Handles
    std::unique_ptr<Fp::Install> mFlashpointInstall;
    std::unique_ptr<Qx::ApplicationLogger> mLogger;

    // Processing
    bool mCriticalErrorOccured;
    NotificationVerbosity mNotificationVerbosity;
    std::queue<Task*> mTaskQueue;

    // Info
    QString mStatusHeading;
    QString mStatusMessage;

    // Other
    QProcessEnvironment mChildTitleProcEnv;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    explicit Core(QObject* parent);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    bool isActionableOptionSet(const QCommandLineParser& clParser) const;
    void showHelp();
    void showVersion();

    // Helper
    ErrorCode searchAndFilterEntity(QUuid& returnBuffer, QString name, bool exactName, QUuid parent = QUuid());

public:
    // Setup
    ErrorCode initialize(QStringList& commandLine);
    void attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall);

    // Helper
    QString resolveTrueAppPath(const QString& appPath, const QString& platform);
    ErrorCode findGameIdFromTitle(QUuid& returnBuffer, QString title, bool exactTitle = true);
    ErrorCode findAddAppIdFromName(QUuid& returnBuffer, QUuid parent, QString name, bool exactName = true);

    // Common
    ErrorCode enqueueStartupTasks();
    void enqueueShutdownTasks();
#ifdef _WIN32
    ErrorCode conditionallyEnqueueBideTask(QFileInfo precedingAppInfo);
#endif
    ErrorCode enqueueDataPackTasks(QUuid targetId);
    void enqueueSingleTask(Task* task);
    void clearTaskQueue(); // TODO: See if this can be done away with, it's awkward (i.e. not fill queue in first place). Think I tried to before though.

    // Notifications/Logging
    void logCommand(QString src, QString commandName);
    void logCommandOptions(QString src, QString commandOptions);
    void logError(QString src, Qx::GenericError error);
    void logEvent(QString src, QString event);
    void logTask(QString src, const Task* task);
    ErrorCode logFinish(QString src, ErrorCode exitCode);
    void postError(QString src, Qx::GenericError error, bool log = true);
    int postBlockingError(QString src, Qx::GenericError error, bool log = true, QMessageBox::StandardButtons bs = QMessageBox::Ok, QMessageBox::StandardButton def = QMessageBox::NoButton);
    void postMessage(QString msg);
    QString requestSaveFilePath(const SaveFileRequest& request);
    QString requestItemSelection(const ItemSelectionRequest& request);

    // Member access
    Fp::Install& fpInstall();
    const QProcessEnvironment& childTitleProcessEnvironment();
    NotificationVerbosity notifcationVerbosity() const;
    size_t taskCount() const;
    bool hasTasks() const;
    Task* frontTask();
    void removeFrontTask();

    // Status
    QString statusHeading();
    QString statusMessage();
    void setStatus(QString heading, QString message);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void statusChanged(const QString& statusHeading, const QString& statusMessage);
    void errorOccured(const Core::Error& error);
    void blockingErrorOccured(QSharedPointer<int> response, const Core::BlockingError& blockingError);
    void saveFileRequested(QSharedPointer<QString> file, const Core::SaveFileRequest& request);
    void itemSelectionRequested(QSharedPointer<QString> item, const Core::ItemSelectionRequest& request);
    void message(const QString& message);
};

//-Metatype Declarations-----------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(Core::Error);
Q_DECLARE_METATYPE(Core::BlockingError);

#endif // CORE_H
