#ifndef CORE_H
#define CORE_H

// Standard Library Includes
#include <queue>

// Qt Includes
#include <QString>
#include <QList>
#include <QCommandLineParser>
#include <QProcessEnvironment>
#include <QUuid>

// Qx Includes
#include <qx/core/qx-processbider.h>

// Project Includes
#include "kernel/buildinfo.h"
#include "kernel/directorate.h"
#include "task/task.h"
#include "_backend_project_vars.h"

/* TODO: It's time to cleanout the old unused mounter support (i.e. docker, QEMU, etc),
 * though maybe keep them a bit longer for if we ever get to adding support for other
 * BM projects, as those might still use that tech
 */

class QX_ERROR_TYPE(CoreError, "CoreError", 1200)
{
    friend class Core;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        CompanionModeLauncherClose,
        CompanionModeServerOverride,
        InvalidOptions,
        TitleNotFound,
        TooManyResults,
        ConfiguredServerMissing,
        UnknownDatapackParam,
        CannotObtainDatapack
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {CompanionModeLauncherClose, u"The standard launcher was closed while in companion mode."_s},
        {CompanionModeServerOverride, u"Cannot enact game server override in companion mode."_s},
        {InvalidOptions, u"Invalid global options provided."_s},
        {TitleNotFound, u"Could not find the title in the Flashpoint database."_s},
        {TooManyResults, u"More results than can be presented were returned in a search."_s},
        {ConfiguredServerMissing, u"The configured server was not found within the Flashpoint services store."_s},
        {UnknownDatapackParam, u"Unrecognized datapack parameters were present. The game likely won't work correctly."_s},
        {CannotObtainDatapack, u"The specified datapack could not be obtained via edition appropriate means."_s}
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

namespace Fp
{
class Install;
class GameData;
}
class TExec;
class ArchiveAccess;

class Core : public QObject, public Directorate
{
    Q_OBJECT;
//-Class Enums-----------------------------------------------------------------------
public:
    enum ServicesMode { Standalone, Companion };

//-Class Variables------------------------------------------------------------------------------------------------------
public:
    // Single Instance ID
    static inline const QString SINGLE_INSTANCE_ID = u"CLIFp_ONE_INSTANCE"_s; // Basically never change this

    // Status
    static inline const QString STATUS_DISPLAY = u"Displaying"_s;
    static inline const QString STATUS_DISPLAY_HELP = u"Help"_s;
    static inline const QString STATUS_DISPLAY_VERSION = u"Version"_s;

    // Logging - Primary Values

    static inline const QString LOG_NO_PARAMS = u"*None*"_s;

    // Logging - Errors
    static inline const QString LOG_ERR_INVALID_PARAM = u"Invalid parameters provided"_s;
    static inline const QString LOG_ERR_FAILED_SETTING_RUFFLE_PERMS= u"Failed to mark ruffle as executable!"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_INIT = u"Initializing CLIFp..."_s;
    static inline const QString LOG_EVENT_MODE_SET = u"Services mode set: %1"_s;
    static inline const QString LOG_EVENT_GLOBAL_OPT = u"Global Options: %1"_s;
    static inline const QString LOG_EVENT_FURTHER_INSTANCE_BLOCK_SUCC = u"Successfully locked standard instance count..."_s;
    static inline const QString LOG_EVENT_FURTHER_INSTANCE_BLOCK_FAIL = u"Failed to lock standard instance count"_s;
    static inline const QString LOG_EVENT_G_HELP_SHOWN = u"Displayed general help information"_s;
    static inline const QString LOG_EVENT_VER_SHOWN = u"Displayed version information"_s;
    static inline const QString LOG_EVENT_PROTOCOL_FORWARD = u"Delegated protocol request to 'play'"_s;
    static inline const QString LOG_EVENT_FLASHPOINT_VERSION_TXT = u"Flashpoint version.txt: %1"_s;
    static inline const QString LOG_EVENT_FLASHPOINT_VERSION = u"Flashpoint version: %1"_s;
    static inline const QString LOG_EVENT_FLASHPOINT_EDITION = u"Flashpoint edition: %1"_s;
    static inline const QString LOG_EVENT_OUTFITTED_DAEMON = u"Recognized daemon: %1"_s;
    static inline const QString LOG_EVENT_ENQ_START = u"Enqueuing startup tasks..."_s;
    static inline const QString LOG_EVENT_ENQ_STOP = u"Enqueuing shutdown tasks..."_s;
    static inline const QString LOG_EVENT_ENQ_DATA_PACK = u"Enqueuing Data Pack tasks..."_s;
    static inline const QString LOG_EVENT_TASK_ENQ = u"Enqueued %1: {%2}"_s;
    static inline const QString LOG_EVENT_DATA_PACK_PATH = u"Title Data Pack path is: %1"_s;
    static inline const QString LOG_EVENT_DATA_PACK_MISS = u"Title Data Pack is not available locally"_s;
    static inline const QString LOG_EVENT_DATA_PACK_FOUND = u"Title Data Pack with correct hash is already present, no need to download"_s;
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_MOUNT = u"Title Data Pack requires mounting"_s;
    static inline const QString LOG_EVENT_DATA_PACK_NEEDS_EXTRACT = u"Title Data Pack requires extraction"_s;
    static inline const QString LOG_EVENT_DATA_PACK_ALREADY_EXTRACTED = u"Extracted files already present"_s;
    static inline const QString LOG_EVENT_DATA_PACK_FROM_ARCHIVE = u"Retrieving Data Pack from archive"_s;
    static inline const QString LOG_EVENT_DATA_PACK_FROM_ARCHIVE_NOT_FOUND = u"Data Pack could not be found in the archive!"_s;
    static inline const QString LOG_EVENT_DATA_PACK_FROM_ARCHIVE_CORRUPT = u"Data Pack from archive is corrupted!"_s;
    static inline const QString LOG_EVENT_DATA_PACK_FROM_ARCHIVE_FOUND = u"Data Pack found in archive."_s;
    static inline const QString LOG_EVENT_DATA_PACK_FROM_ARCHIVE_SAVED = u"Sourced Data Pack from archive."_s;
    static inline const QString LOG_EVENT_APP_PATH_ALT = u"App path \"%1\" maps to alternative \"%2\"."_s;
    static inline const QString LOG_EVENT_SERVICES_FROM_LAUNCHER = u"Using services from standard Launcher due to companion mode."_s;
    static inline const QString LOG_EVENT_LAUNCHER_WATCH = u"Starting bide on Launcher process..."_s;
    static inline const QString LOG_EVENT_LAUNCHER_WATCH_HOOKED = u"Launcher hooked for waiting"_s;
    static inline const QString LOG_EVENT_LAUNCHER_CLOSED_RESULT = u"CLIFp cannot continue running in companion mode without the launcher's services."_s;

    // Logging - Linux Specific Startup Steps
    static inline const QString LOG_EVENT_LINUX_SPECIFIC_STARTUP_STEPS= u"Handling Linux specific startup steps..."_s;
    static inline const QString LOG_EVENT_LINUX_BUILD_TYPE = u"Linux build %1 mutable."_s;
    static inline const QString LOG_EVENT_LINUX_GTK3_MISSING = u"GTK3 isn't installed, setting GTK_USE_PORTAL=1"_s;
    static inline const QString LOG_EVENT_LINUX_SERVER_NOOP = u"Skipping server start ('read' no-op detected)"_s;

    // Logging - Title Search
    static inline const QString LOG_EVENT_GAME_SEARCH = u"Searching for game with title '%1'"_s;
    static inline const QString LOG_EVENT_ADD_APP_SEARCH = u"Searching for additional-app with title '%1' and parent %2"_s;
    static inline const QString LOG_EVENT_TITLE_ID_COUNT = u"Found %1 ID(s) when searching for title %2"_s;
    static inline const QString LOG_EVENT_TITLE_SEL_PROMNPT = u"Prompting user to disambiguate multiple IDs..."_s;
    static inline const QString LOG_EVENT_TITLE_ID_DETERMINED = u"ID of title %1 determined to be %2"_s;
    static inline const QString LOG_EVENT_TITLE_SEL_CANCELED = u"Title selection was canceled by the user."_s;

    // Global command line option strings
    static inline const QString CL_OPT_HELP_S_NAME = u"h"_s;
    static inline const QString CL_OPT_HELP_E_NAME = u"?"_s;
    static inline const QString CL_OPT_HELP_L_NAME = u"help"_s;
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
    static inline const QCommandLineOption CL_OPTION_HELP{{CL_OPT_HELP_S_NAME, CL_OPT_HELP_E_NAME, CL_OPT_HELP_L_NAME}, CL_OPT_HELP_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_VERSION{{CL_OPT_VERSION_S_NAME, CL_OPT_VERSION_L_NAME}, CL_OPT_VERSION_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_QUIET{{CL_OPT_QUIET_S_NAME, CL_OPT_QUIET_L_NAME}, CL_OPT_QUIET_DESC}; // Boolean option
    static inline const QCommandLineOption CL_OPTION_SILENT{{CL_OPT_SILENT_S_NAME, CL_OPT_SILENT_L_NAME}, CL_OPT_SILENT_DESC}; // Boolean option

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_ALL{&CL_OPTION_HELP, &CL_OPTION_VERSION, &CL_OPTION_QUIET, &CL_OPTION_SILENT};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_ACTIONABLE{&CL_OPTION_HELP, &CL_OPTION_VERSION};

    // Help template
    static inline const QString HELP_TEMPL = u"<u>Usage:</u><br>"
                                              PROJECT_SHORT_NAME " &lt;global options&gt; <i>command</i> &lt;command options&gt;<br>"
                                              "<br>"
                                              "<u>Global Options:</u>%1<br>"
                                              "<br>"
                                              "<u>Commands:</u>%2<br>"
                                              "<br>"
                                              "Use the <b>-h</b> switch after a command to see it's specific usage notes"_s;
    static inline const QString HELP_OPT_TEMPL = u"<br><b>%1:</b> &nbsp;%2"_s;
    static inline const QString HELP_COMMAND_TEMPL = u"<br><b>%1:</b> &nbsp;%2"_s;

    // Command line messages
    static inline const QString CL_VERSION_MESSAGE = u"CLI Flashpoint " PROJECT_VERSION_STR ", designed for use with Flashpoint Archive " PROJECT_TARGET_FP_VER_PFX_STR " series"_s;

    // Input strings
    static inline const QString MULTI_TITLE_SEL_CAP = u"Title Disambiguation"_s;
    static inline const QString MULTI_TITLE_SEL_LABEL = u"Title to start:"_s;
    static inline const QString MULTI_GAME_SEL_TEMP = u"[%1] %2 (%3) {%4}"_s;
    static inline const QString MULTI_ADD_APP_SEL_TEMP = u"%1 {%2}"_s;

    // Helper
    static const int FIND_ENTRY_LIMIT = 20;
    static inline const QString DATA_PACK_FROM_ARCHIVE_TEMPLATE = u"Flashpoint Ultimate/Data/Games/%1"_s;

    // Protocol
    static inline const QString FLASHPOINT_PROTOCOL_SCHEME = u"flashpoint://"_s;

    // Meta
    static inline const QString NAME = u"core"_s;

//-Instance Variables------------------------------------------------------------------------------------------------------
private:
    // Director
    Director mDirector;

    // Handles
    std::unique_ptr<Fp::Install> mFlashpointInstall;
    std::unique_ptr<ArchiveAccess> mGamesArchive;

    // Processing
    ServicesMode mServicesMode;
    std::queue<Task*> mTaskQueue;

    // Other
    QProcessEnvironment mChildTitleProcEnv;
    Qx::ProcessBider mLauncherWatcher;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    explicit Core();

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    ~Core();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    QString name() const override;
    bool isActionableOptionSet(const QCommandLineParser& clParser) const;
    void showHelp();
    void showVersion();

    // Helper
    Qx::Error searchAndFilterEntity(QUuid& returnBuffer, QString name, bool exactName, QUuid parent = QUuid());
    void addOnDiskUpdateTask(int gameDataId);
    void logTask(const Task* task);

public:
    // Setup
    Qx::Error initialize(QStringList& commandLine);
    void setServicesMode(ServicesMode mode = ServicesMode::Standalone);
    void watchLauncher();
    void attachFlashpoint(std::unique_ptr<Fp::Install> flashpointInstall);

    // Helper (TODO: Move some of these to libfp Toolkit)
    QString resolveFullAppPath(const QString& appPath, const QString& platform);
    Qx::Error findGameIdFromTitle(QUuid& returnBuffer, QString title, bool exactTitle = true);
    Qx::Error findAddAppIdFromName(QUuid& returnBuffer, QUuid parent, QString name, bool exactName = true);

    // Common
    bool blockNewInstances();
    CoreError enqueueStartupTasks(const QString& serverOverride = {});
    void enqueueShutdownTasks();
#ifdef _WIN32
    Qx::Error conditionallyEnqueueBideTask(const TExec* precedingTask);
#endif
    Qx::Error enqueueDataPackTasks(const Fp::GameData& gameData);
    void enqueueSingleTask(Task* task);

    // Member access
    Director* director();
    ServicesMode mode() const;
    Fp::Install& fpInstall();
    const QProcessEnvironment& childTitleProcessEnvironment();
    size_t taskCount() const;
    bool hasTasks() const;
    Task* frontTask();
    void removeFrontTask();

    // Other
    BuildInfo buildInfo() const;

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void abort(CoreError err);
};

#endif // CORE_H
