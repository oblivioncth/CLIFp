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
#include "kernel/buildinfo.h"

// General Aliases
using ErrorCode = quint32;

class QX_ERROR_TYPE(CoreError, "CoreError", 1200)
{
    friend class Core;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        InvalidOptions = 1,
        TitleNotFound = 2,
        TooManyResults = 3,
        ConfiguredServerMissing = 4,
        DataPackSumMismatch = 5,
        DataPackSourceMissing = 6
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {InvalidOptions, u"Invalid global options provided."_s},
        {TitleNotFound, u"Could not find the title in the Flashpoint database."_s},
        {TooManyResults, u"More results than can be presented were returned in a search."_s},
        {ConfiguredServerMissing, u"The server specified in the Flashpoint config was not found within the Flashpoint services store."_s},
        {DataPackSumMismatch, u"The existing Data Pack of the selected title does not contain the data expected. It will be re-downloaded."_s},
        {DataPackSourceMissing, u"The expected primary data pack source was missing."_s}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;
    Qx::Severity mSeverity;

    //-Constructor-------------------------------------------------------------
private:
    CoreError(Type t = NoError, const QString& s = {}, Qx::Severity sv = Qx::Critical);

    //-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class Core : public QObject
{
    Q_OBJECT;
//-Class Enums-----------------------------------------------------------------------
public:
    enum class NotificationVerbosity { Full, Quiet, Silent };

//-Class Structs---------------------------------------------------------------------
public:
    /* TODO: These should be made their own files like message.h is in frontend
     * (or one file, like "requests.h"), or message.h should be removed with its
     * struct moved to here
     */
    struct Error
    {
        QString source;
        Qx::Error errorInfo;
    };

    struct BlockingError
    {
        QString source;
        Qx::Error errorInfo;
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
    static inline const QString STATUS_DISPLAY = u"Displaying"_s;
    static inline const QString STATUS_DISPLAY_HELP = u"Help"_s;
    static inline const QString STATUS_DISPLAY_VERSION = u"Version"_s;

    // Logging - Primary Labels
    static inline const QString COMMAND_LABEL = u"Command: %1"_s;
    static inline const QString COMMAND_OPT_LABEL = u"Command Options: %1"_s;

    // Logging - Primary Values
    static inline const QString LOG_FILE_EXT = u"log"_s;
    static inline const QString LOG_NO_PARAMS = u"*None*"_s;
    static const int LOG_MAX_ENTRIES = 50;

    // Logging - Errors
    static inline const QString LOG_ERR_INVALID_PARAM = u"Invalid parameters provided"_s;
    static inline const QString LOG_ERR_CRITICAL = u"Aborting execution due to previous critical errors"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_INIT = u"Initializing CLIFp..."_s;
    static inline const QString LOG_EVENT_GLOBAL_OPT = u"Global Options: %1"_s;
    static inline const QString LOG_EVENT_G_HELP_SHOWN = u"Displayed general help information"_s;
    static inline const QString LOG_EVENT_VER_SHOWN = u"Displayed version information"_s;
    static inline const QString LOG_EVENT_NOTIFCATION_LEVEL = u"Notification Level is: %1"_s;
    static inline const QString LOG_EVENT_PROTOCOL_FORWARD = u"Delegated protocol request to 'play'"_s;
    static inline const QString LOG_EVENT_FLASHPOINT_VERSION_TXT = u"Flashpoint version.txt: %1"_s;
    static inline const QString LOG_EVENT_FLASHPOINT_VERSION = u"Flashpoint version: %1"_s;
    static inline const QString LOG_EVENT_FLASHPOINT_EDITION = u"Flashpoint edition: %1"_s;
    static inline const QString LOG_EVENT_OUTFITTED_DAEMON = u"Recognized daemon: %1"_s;
    static inline const QString LOG_EVENT_ENQ_START = u"Enqueuing startup tasks..."_s;
    static inline const QString LOG_EVENT_ENQ_STOP = u"Enqueuing shutdown tasks..."_s;
    static inline const QString LOG_EVENT_ENQ_DATA_PACK = u"Enqueuing Data Pack tasks..."_s;
    static inline const QString LOG_EVENT_DATA_PACK_MISS = u"Title Data Pack is not available locally"_s;
    static inline const QString LOG_EVENT_DATA_PACK_FOUND = u"Title Data Pack with correct hash is already present, no need to download"_s;
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_MOUNT = u"Title Data Pack requires mounting"_s;
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_EXTRACT = u"Title Data Pack requires extraction"_s;
    static inline const QString LOG_EVENT_TASK_ENQ = u"Enqueued %1: {%2}"_s;
    static inline const QString LOG_EVENT_APP_PATH_ALT = u"App path \"%1\" maps to alternative \"%2\"."_s;

    // Logging - Title Search
    static inline const QString LOG_EVENT_GAME_SEARCH = u"Searching for game with title '%1'"_s;
    static inline const QString LOG_EVENT_ADD_APP_SEARCH = u"Searching for additional-app with title '%1' and parent %2"_s;
    static inline const QString LOG_EVENT_TITLE_ID_COUNT = u"Found %1 ID(s) when searching for title %2"_s;
    static inline const QString LOG_EVENT_TITLE_SEL_PROMNPT = u"Prompting user to disambiguate multiple IDs..."_s;
    static inline const QString LOG_EVENT_TITLE_ID_DETERMINED = u"ID of title %1 determined to be %2"_s;
    static inline const QString LOG_EVENT_TITLE_SEL_CANCELED = u"Title selection was canceled by the user."_s;

    // Global command line option strings
    static inline const QString CL_OPT_HELP_S_NAME = u"h"_s;
    static inline const QString CL_OPT_HELP_L_NAME = u"help"_s;
    static inline const QString CL_OPT_HELP_E_NAME = u"?"_s;
    static inline const QString CL_OPT_HELP_DESC = u"Prints this help message."_s;

    static inline const QString CL_OPT_VERSION_S_NAME = u"v"_s;
    static inline const QString CL_OPT_VERSION_L_NAME = u"version"_s;
    static inline const QString CL_OPT_VERSION_DESC = u"Prints the current version of this tool."_s;

    static inline const QString CL_OPT_QUIET_S_NAME = u"q"_s;
    static inline const QString CL_OPT_QUIET_L_NAME = u"quiet"_s;
    static inline const QString CL_OPT_QUIET_DESC = u"Silences all non-critical messages."_s;

    static inline const QString CL_OPT_SILENT_S_NAME = u"s"_s;
    static inline const QString CL_OPT_SILENT_L_NAME = u"silent"_s;
    static inline const QString CL_OPT_SILENT_DESC = u"Silences all messages (takes precedence over quiet mode)."_s;

    // Global command line options
    static inline const QCommandLineOption CL_OPTION_HELP{{CL_OPT_HELP_S_NAME, CL_OPT_HELP_L_NAME, CL_OPT_HELP_E_NAME}, CL_OPT_HELP_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_VERSION{{CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_QUIET{{CL_OPT_QUIET_S_NAME, CL_OPT_QUIET_L_NAME}, CL_OPT_QUIET_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_SILENT{{CL_OPT_SILENT_S_NAME, CL_OPT_SILENT_L_NAME}, CL_OPT_SILENT_DESC}; // Boolean option

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_ALL{&CL_OPTION_HELP, &CL_OPTION_VERSION, &CL_OPTION_QUIET, &CL_OPTION_SILENT};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_ACTIONABLE{&CL_OPTION_HELP, &CL_OPTION_VERSION};

    // Help template
    static inline const QString HELP_TEMPL = u"<u>Usage:</u><br>"
                                              PROJECT_SHORT_NAME "&lt;global options&gt; <i>command</i> &lt;command options&gt;<br>"
                                              "<br>"
                                              "<u>Global Options:</u>%1<br>"
                                              "<br>"
                                              "<u>Commands:</u>%2<br>"
                                              "<br>"
                                              "Use the <b>-h</b> switch after a command to see it's specific usage notes"_s;
    static inline const QString HELP_OPT_TEMPL = u"<br><b>%1:</b> &nbsp;%2"_s;
    static inline const QString HELP_COMMAND_TEMPL = u"<br><b>%1:</b> &nbsp;%2"_s;

    // Command line messages
    static inline const QString CL_VERSION_MESSAGE = u"CLI Flashpoint version " PROJECT_VERSION_STR ", designed for use with Flashpoint Archive " PROJECT_TARGET_FP_VER_PFX_STR " series"_s;

    // Input strings
    static inline const QString MULTI_TITLE_SEL_CAP = u"Title Disambiguation"_s;
    static inline const QString MULTI_TITLE_SEL_LABEL = u"Title to start:"_s;
    static inline const QString MULTI_GAME_SEL_TEMP = u"[%1] %2 (%3) {%4}"_s;
    static inline const QString MULTI_ADD_APP_SEL_TEMP = u"%1 {%2}"_s;

    // Helper
    static const int FIND_ENTRY_LIMIT = 20;

    // Protocol
    static inline const QString FLASHPOINT_PROTOCOL_SCHEME = u"flashpoint://"_s;

    // Meta
    static inline const QString NAME = u"core"_s;

//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    // Handles
    std::unique_ptr<Fp::Install> mFlashpointInstall;
    std::unique_ptr<Qx::ApplicationLogger> mLogger;

    // Processing
    bool mCriticalErrorOccurred;
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
    Qx::Error searchAndFilterEntity(QUuid& returnBuffer, QString name, bool exactName, QUuid parent = QUuid());

public:
    // Setup
    Qx::Error initialize(QStringList& commandLine);
    void attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall);

    // Helper
    QString resolveTrueAppPath(const QString& appPath, const QString& platform);
    Qx::Error findGameIdFromTitle(QUuid& returnBuffer, QString title, bool exactTitle = true);
    Qx::Error findAddAppIdFromName(QUuid& returnBuffer, QUuid parent, QString name, bool exactName = true);

    // Common
    CoreError enqueueStartupTasks();
    void enqueueShutdownTasks();
#ifdef _WIN32
    Qx::Error conditionallyEnqueueBideTask(QFileInfo precedingAppInfo);
#endif
    Qx::Error enqueueDataPackTasks(const Fp::GameData& gameData);
    void enqueueSingleTask(Task* task);
    void clearTaskQueue(); // TODO: See if this can be done away with, it's awkward (i.e. not fill queue in first place). Think I tried to before though.

    // Notifications/Logging
    void logCommand(QString src, QString commandName);
    void logCommandOptions(QString src, QString commandOptions);
    void logError(QString src, Qx::Error error);
    void logEvent(QString src, QString event);
    void logTask(QString src, const Task* task);
    ErrorCode logFinish(QString src, Qx::Error errorState);
    void postError(QString src, Qx::Error error, bool log = true);
    int postBlockingError(QString src, Qx::Error error, bool log = true, QMessageBox::StandardButtons bs = QMessageBox::Ok, QMessageBox::StandardButton def = QMessageBox::NoButton);
    void postMessage(const Message& msg);
    QString requestSaveFilePath(const SaveFileRequest& request);
    QString requestItemSelection(const ItemSelectionRequest& request);
    void requestClipboardUpdate(const QString& text);

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

    // Other
    BuildInfo buildInfo() const;

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void statusChanged(const QString& statusHeading, const QString& statusMessage);
    void errorOccurred(const Core::Error& error);
    void blockingErrorOccurred(QSharedPointer<int> response, const Core::BlockingError& blockingError);
    void saveFileRequested(QSharedPointer<QString> file, const Core::SaveFileRequest& request);
    void itemSelectionRequested(QSharedPointer<QString> item, const Core::ItemSelectionRequest& request);
    void message(const Message& message);
    void clipboardUpdateRequested(const QString& text);
};

//-Metatype Declarations-----------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(Core::Error);
Q_DECLARE_METATYPE(Core::BlockingError);

#endif // CORE_H
