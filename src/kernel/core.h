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

// libfp Includes
#include <fp/flashpoint/fp-install.h>

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
    static inline const QString LOG_FILE_EXT = "log";
    static inline const QString LOG_NO_PARAMS = "*None*";
    static const int LOG_MAX_ENTRIES = 50;

    // Logging - Errors
    static inline const QString LOG_ERR_INVALID_PARAM = "Invalid parameters provided";
    static inline const QString LOG_ERR_CRITICAL = "Aborting execution due to previous critical errors";

    // Logging - Messages
    static inline const QString LOG_EVENT_INIT = "Initializing CLIFp...";
    static inline const QString LOG_EVENT_GLOBAL_OPT = "Global Options: %1";
    static inline const QString LOG_EVENT_G_HELP_SHOWN = "Displayed general help information";
    static inline const QString LOG_EVENT_VER_SHOWN = "Displayed version information";
    static inline const QString LOG_EVENT_NOTIFCATION_LEVEL = "Notification Level is: %1";
    static inline const QString LOG_EVENT_ENQ_START = "Enqueuing startup tasks...";
    static inline const QString LOG_EVENT_ENQ_STOP = "Enqueuing shutdown tasks...";
    static inline const QString LOG_EVENT_ENQ_DATA_PACK = "Enqueuing Data Pack tasks...";
    static inline const QString LOG_EVENT_DATA_PACK_MISS = "Title Data Pack is not available locally";
    static inline const QString LOG_EVENT_DATA_PACK_FOUND = "Title Data Pack with correct hash is already present, no need to download";
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_MOUNT = "Title Data Pack requires mounting";
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_EXTRACT = "Title Data Pack requires extraction";
    static inline const QString LOG_EVENT_TASK_ENQ = "Enqueued %1: {%2}";
    static inline const QString LOG_EVENT_APP_PATH_ALT = "App path \"%1\" maps to alternative \"%2\".";

    // Logging - Title Search
    static inline const QString LOG_EVENT_GAME_SEARCH = "Searching for game with title '%1'";
    static inline const QString LOG_EVENT_ADD_APP_SEARCH = "Searching for additional-app with title '%1' and parent %2";
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
    static inline const QString CL_VERSION_MESSAGE = "CLI Flashpoint version " PROJECT_VERSION_STR ", designed for use with BlueMaxima's Flashpoint " PROJECT_TARGET_FP_VER_PFX_STR " series";

    // Input strings
    static inline const QString MULTI_TITLE_SEL_CAP = "Title Disambiguation";
    static inline const QString MULTI_TITLE_SEL_LABEL = "Title to start:";
    static inline const QString MULTI_TITLE_SEL_TEMP = "[%1] %2 (%3) {%4}";

    // Meta
    static inline const QString NAME = "core";

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
    ErrorCode searchAndFilterEntity(QUuid& returnBuffer, QString name, QUuid parent = QUuid());

public:
    // Setup
    ErrorCode initialize(QStringList& commandLine);
    void attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall);

    // Helper
    QString resolveTrueAppPath(const QString& appPath, const QString& platform);
    /* TODO: These 2 functions are here because they need end up emitting a Core signal;
     * however, it may make more sense to move them to CPlay (where they are used) and simply
     * call the core function(s) that emit the signals from there
     */
    ErrorCode getGameIdFromTitle(QUuid& returnBuffer, QString title);
    ErrorCode getAddAppIdFromName(QUuid& returnBuffer, QUuid parent, QString name);

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
