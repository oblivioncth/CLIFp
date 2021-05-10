#ifndef FLASHPOINT_INSTALL_H
#define FLASHPOINT_INSTALL_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QtSql>
#include "qx.h"

namespace FP
{

class Install
{
//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum class CompatLevel{ Execution, Full };
    enum class LibraryFilter{ Game, Anim, Either };

//-Class Structs-------------------------------------------------------------------------------------------------
public:
    struct InclusionOptions
    {
        bool includeExtreme;
        bool includeAnimations;
    };

    struct DBTableSpecs
    {
        QString name;
        QStringList columns;
    };

    struct DBQueryBuffer
    {
        QString source;
        QSqlQuery result;
        int size = 0;
    };

    struct Config
    {
        QString imageFolderPath;
        bool startServer;
        QString server;
    };

    struct ServerDaemon
    {
        QString name;
        QString path;
        QString filename;
        QStringList arguments;
        bool kill;
    };
    struct StartStop
    {
        QString path;
        QString filename;
        QStringList arguments;

        friend bool operator== (const StartStop& lhs, const StartStop& rhs) noexcept;
        friend size_t qHash(const StartStop& key, size_t seed) noexcept;
    };

    struct Services
    {
        //QSet<Watch> watches;
        QHash<QString, ServerDaemon> servers;
        QHash<QString, ServerDaemon> daemons;
        QSet<StartStop> starts;
        QSet<StartStop> stops;
    };

    struct ValidityReport
    {
        bool installValid;
        QString details;
    };

//-Inner Classes-------------------------------------------------------------------------------------------------
public:
    class DBTable_Game
    {
    public:
        static inline const QString NAME = "game";

        static inline const QString COL_ID = "id";
        static inline const QString COL_TITLE = "title";
        static inline const QString COL_SERIES = "series";
        static inline const QString COL_DEVELOPER = "developer";
        static inline const QString COL_PUBLISHER = "publisher";
        static inline const QString COL_DATE_ADDED = "dateAdded";
        static inline const QString COL_DATE_MODIFIED = "dateModified";
        static inline const QString COL_PLATFORM = "platform";
        static inline const QString COL_BROKEN = "broken";
        static inline const QString COL_EXTREME = "extreme";
        static inline const QString COL_PLAY_MODE = "playMode";
        static inline const QString COL_STATUS = "status";
        static inline const QString COL_NOTES = "notes";
        static inline const QString COL_SOURCE = "source";
        static inline const QString COL_APP_PATH = "applicationPath";
        static inline const QString COL_LAUNCH_COMMAND = "launchCommand";
        static inline const QString COL_RELEASE_DATE = "releaseDate";
        static inline const QString COL_VERSION = "version";
        static inline const QString COL_ORIGINAL_DESC = "originalDescription";
        static inline const QString COL_LANGUAGE = "language";
        static inline const QString COL_LIBRARY = "library";
        static inline const QString COL_ORDER_TITLE = "orderTitle";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_TITLE, COL_SERIES, COL_DEVELOPER, COL_PUBLISHER, COL_DATE_ADDED, COL_DATE_MODIFIED, COL_PLATFORM,
                                               COL_BROKEN, COL_EXTREME, COL_PLAY_MODE, COL_STATUS, COL_NOTES, COL_SOURCE, COL_APP_PATH, COL_LAUNCH_COMMAND, COL_RELEASE_DATE,
                                               COL_VERSION, COL_ORIGINAL_DESC, COL_LANGUAGE, COL_LIBRARY, COL_ORDER_TITLE};

        static inline const QString ENTRY_GAME_LIBRARY = "arcade";
        static inline const QString ENTRY_ANIM_LIBRARY = "theatre";
        static inline const QString ENTRY_NOT_WORK = "Not Working";
    };

    class DBTable_Add_App
    {
    public:
        static inline const QString NAME = "additional_app";

        static inline const QString COL_ID = "id";
        static inline const QString COL_APP_PATH = "applicationPath";
        static inline const QString COL_AUTORUN = "autoRunBefore";
        static inline const QString COL_LAUNCH_COMMAND = "launchCommand";
        static inline const QString COL_NAME = "name";
        static inline const QString COL_WAIT_EXIT = "waitForExit";
        static inline const QString COL_PARENT_ID = "parentGameId";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_APP_PATH, COL_AUTORUN, COL_LAUNCH_COMMAND, COL_NAME, COL_WAIT_EXIT, COL_PARENT_ID};

        static inline const QString ENTRY_EXTRAS = ":extras:";
        static inline const QString ENTRY_MESSAGE = ":message:";
    };

    class DBTable_Playlist
    {
    public:
        static inline const QString NAME = "playlist";
        static inline const QString COL_ID = "id";
        static inline const QString COL_TITLE = "title";
        static inline const QString COL_DESCRIPTION = "description";
        static inline const QString COL_AUTHOR = "author";
        static inline const QString COL_LIBRARY = "library";

        static inline const QString ENTRY_GAME_LIBRARY = "arcade";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_TITLE, COL_DESCRIPTION, COL_AUTHOR, COL_LIBRARY};
    };

    class DBTable_Playlist_Game
    {
    public:
        static inline const QString NAME = "playlist_game";

        static inline const QString COL_ID = "id";
        static inline const QString COL_PLAYLIST_ID = "playlistId";
        static inline const QString COL_ORDER = "order";
        static inline const QString COL_GAME_ID = "gameId";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_PLAYLIST_ID, COL_ORDER, COL_GAME_ID};
    };

    class JSONObject_Config
    {
    public:
        static inline const QString KEY_IMAGE_FOLDER_PATH = "imageFolderPath";
        static inline const QString KEY_START_SERVER = "startServer";
        static inline const QString KEY_SERVER = "server";
    };

    class JSONObject_Server
    {
    public:
        static inline const QString KEY_NAME = "name";
        static inline const QString KEY_PATH = "path";
        static inline const QString KEY_FILENAME = "filename";
        static inline const QString KEY_ARGUMENTS = "arguments";
        static inline const QString KEY_KILL = "kill";
    };

    class JSONObject_StartStop
    {
    public:
        static inline const QString KEY_PATH = "path";
        static inline const QString KEY_FILENAME = "filename";
        static inline const QString KEY_ARGUMENTS = "arguments";
    };

    class JSONObject_Daemon
    {
    public:
        static inline const QString KEY_NAME = "name";
        static inline const QString KEY_PATH = "path";
        static inline const QString KEY_FILENAME = "filename";
        static inline const QString KEY_ARGUMENTS = "arguments";
        static inline const QString KEY_KILL = "kill";
    };

    class JSONObject_Services
    {
    public:
        static inline const QString KEY_WATCH = "watch";
        static inline const QString KEY_SERVER = "server";
        static inline const QString KEY_DAEMON = "daemon";
        static inline const QString KEY_START = "start";
        static inline const QString KEY_STOP = "stop";
    };

    class JSONServicesReader
    {
    //-Class variables-----------------------------------------------------------------------------------------------------
    public:
        static inline const QString ERR_PARSING_JSON_DOC = "Error parsing JSON Document: %1";
        static inline const QString ERR_JSON_UNEXP_FORMAT = "Unexpected document format";
        static inline const QString MACRO_FP_PATH = "<fpPath>";

    //-Instance Variables--------------------------------------------------------------------------------------------------
    private:
        const Install* mHostInstall;
        Services* mTargetServices;
        std::shared_ptr<QFile> mTargetJSONFile;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        JSONServicesReader(const Install* hostInstall, Services* targetServices, std::shared_ptr<QFile> targetJSONFile);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        QString resolveFlashpointMacros(QString macroString);
        Qx::GenericError parseServicesDocument(const QJsonDocument& servicesDoc);
        Qx::GenericError parseServerDaemon(ServerDaemon& serverBuffer, const QJsonValue& jvServer);
        Qx::GenericError parseStartStop(StartStop& startStopBuffer, const QJsonValue& jvStartStop);

    public:
        Qx::GenericError readInto();
    };

    class JSONConfigReader
    {
    //-Class variables-----------------------------------------------------------------------------------------------------
    public:
        static inline const QString ERR_PARSING_JSON_DOC = "Error parsing JSON Document: %1";
        static inline const QString ERR_JSON_UNEXP_FORMAT = "Unexpected document format";

    //-Instance Variables--------------------------------------------------------------------------------------------------
    private:
        Config* mTargetConfig;
        std::shared_ptr<QFile> mTargetJSONFile;

    //-Constructor--------------------------------------------------------------------------------------------------------
    public:
        JSONConfigReader(Config* targetServices, std::shared_ptr<QFile> targetJSONFile);

    //-Instance Functions-------------------------------------------------------------------------------------------------
    private:
        Qx::GenericError parseConfigDocument(const QJsonDocument& configDoc);
    public:
        Qx::GenericError readInto();
    };

    class CLIFp
    {
    // Class members
    public:
        static inline const QString EXE_NAME = "CLIFp.exe";
        static inline const QString APP_ARG = R"(--exe="%1")";
        static inline const QString PARAM_ARG = R"(--param="%1")";
        static inline const QString EXTRA_ARG = R"(--extra-"%1")";
        static inline const QString MSG_ARG = R"(--msg="%1")";

    // Class functions
    public:
        static QString parametersFromStandard(QString originalAppPath, QString originalAppParams);
    };

//-Class Variables-----------------------------------------------------------------------------------------------
private:
    // Validity check fail reasons
    static inline const QString FILE_DNE = "A required file does not exist: %1";

public:
    // Paths
    static inline const QString MAIN_EXE_PATH = "Launcher/Flashpoint.exe";
    static inline const QString DATABASE_PATH = "Data/flashpoint.sqlite";
    static inline const QString SERVICES_JSON_PATH = "Data/services.json";
    static inline const QString CONFIG_JSON_PATH = "Launcher/config.json";
    static inline const QString VER_TXT_PATH = "version.txt";

    // Folders
    static inline const QString LOGOS_FOLDER_NAME = "Logos";
    static inline const QString SCREENSHOTS_FOLDER_NAME = "Screenshots";
    static inline const QString EXTRAS_FOLDER_NAME = "Extras";

    // Version check
    static inline const QByteArray TARGET_EXE_SHA256 = Qx::ByteArray::RAWFromStringHex("6a598f6de11473c2a8a005646dcaff6cccc8d376b2f3f3953ba96dd6a362daa3");
    static inline const QString TARGET_ULT_VER_STRING = R"(Flashpoint 9.0 Ultimate - "Glorious Sunset")";
    static inline const QString TARGET_INF_VER_STRING = R"(Flashpoint 9.0 Infinity - "Glorious Sunset")";

    // Database
    static inline const QString DATABASE_CONNECTION_NAME = "Flashpoint Database";
    static inline const QList<DBTableSpecs> DATABASE_SPECS_LIST = {{DBTable_Game::NAME, DBTable_Game::COLUMN_LIST},
                                                                        {DBTable_Add_App::NAME, DBTable_Add_App::COLUMN_LIST},
                                                                        {DBTable_Playlist::NAME, DBTable_Playlist::COLUMN_LIST},
                                                                        {DBTable_Playlist_Game::NAME, DBTable_Playlist_Game::COLUMN_LIST}};
    static inline const QString GENERAL_QUERY_SIZE_COMMAND = "COUNT(1)";

    static inline const QString GAME_ONLY_FILTER = DBTable_Game::COL_LIBRARY + " = '" + DBTable_Game::ENTRY_GAME_LIBRARY + "'";
    static inline const QString ANIM_ONLY_FILTER = DBTable_Game::COL_LIBRARY + " = '" + DBTable_Game::ENTRY_ANIM_LIBRARY + "'";
    static inline const QString GAME_AND_ANIM_FILTER = "(" + GAME_ONLY_FILTER + " OR " + ANIM_ONLY_FILTER + ")";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Files and directories
    QDir mRootDirectory;
    QDir mLogosDirectory;
    QDir mScreenshotsDirectory;
    QDir mExtrasDirectory;
    std::unique_ptr<QFile> mMainEXEFile;
    std::unique_ptr<QFile> mCLIFpEXEFile;
    std::unique_ptr<QFile> mDatabaseFile;
    std::shared_ptr<QFile> mServicesJSONFile;
    std::shared_ptr<QFile> mConfigJSONFile;
    std::unique_ptr<QFile> mVersionTXTFile;

    // Database information
    QStringList mPlatformList;
    QStringList mPlaylistList;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(QString installPath, QString clifpSubPath = QString());

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~Install();

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    static ValidityReport checkInstallValidity(QString installPath, CompatLevel compatLevel);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    QSqlDatabase getThreadedDatabaseConnection() const;
    QSqlError makeNonBindQuery(DBQueryBuffer& resultBuffer, QSqlDatabase* database, QString queryCommand, QString sizeQueryCommand) const;

public:
    // General Information
    bool matchesTargetVersion() const;
    bool hasCLIFp() const;
    Qx::MMRB currentCLIFpVersion() const;

    // Connection
    QSqlError openThreadDatabaseConnection();
    void closeThreadedDatabaseConnection();
    bool databaseConnectionOpenInThisThread();

    // Support Application Checks
    Qx::GenericError getConfig(Config& configBuffer);
    Qx::GenericError getServices(Services& servicesBuffer);

    // Requirement Checking
    QSqlError checkDatabaseForRequiredTables(QSet<QString>& missingTablesBuffer) const;
    QSqlError checkDatabaseForRequiredColumns(QSet<QString>& missingColumsBuffer) const;

    // Commands
    QSqlError populateAvailableItems();
    bool deployCLIFp(QString &errorMessage);

    // Queries - OFLIb
    QSqlError queryGamesByPlatform(QList<DBQueryBuffer>& resultBuffer, QStringList platforms, InclusionOptions inclusionOptions,
                                   const QList<QUuid>& idFilter = {}) const;
    QSqlError queryAllAddApps(DBQueryBuffer& resultBuffer) const;
    QSqlError queryPlaylistsByName(DBQueryBuffer& resultBuffer, QStringList playlists) const;
    QSqlError queryPlaylistGamesByPlaylist(QList<DBQueryBuffer>& resultBuffer, const QList<QUuid>& playlistIDs) const;
    QSqlError queryPlaylistGameIDs(DBQueryBuffer& resultBuffer, const QList<QUuid>& playlistIDs) const;

    // Queries - CLIFp
    QSqlError queryEntryByID(DBQueryBuffer& resultBuffer, QUuid appID) const;
    QSqlError queryEntryAddApps(DBQueryBuffer& resultBuffer, QUuid appID, bool playableOnly = false) const;
    QSqlError queryAllGameIDs(DBQueryBuffer& resultBuffer, LibraryFilter filter) const;
//    QSqlError queryAllMainAddAppIDs(DBQueryBuffer& resultBuffer, LibraryFilter filter) const;

    // Data access
    QString getPath() const;
    QStringList getPlatformList() const;
    QStringList getPlaylistList() const;
    QDir getLogosDirectory() const;
    QDir getScrenshootsDirectory() const;
    QDir getExtrasDirectory() const;
    QString getCLIFpPath() const;
};

}



#endif // FLASHPOINT_INSTALL_H
