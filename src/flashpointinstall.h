#ifndef FLASHPOINTINSTALL_H
#define FLASHPOINTINSTALL_H

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

//-Class Structs-------------------------------------------------------------------------------------------------
public:
    struct DBTableSpecs
    {
        QString name;
        QStringList columns;
    };

    struct DBQueryBuffer
    {
        QString source;
        QSqlQuery result;
        int size;
    };

    struct Config
    {
        bool startServer;
        QString server;
    };

    struct Server
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
        friend uint qHash(const StartStop& key, uint seed) noexcept;
    };

    struct Services
    {
        //QSet<Watch> watches;
        QHash<QString, Server> servers;
        QSet<StartStop> starts;
        QSet<StartStop> stops;
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

        static inline const QString GAME_LIBRARY = "arcade";
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

        static inline const QString GAME_LIBRARY = "arcade";

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

    class JSONObject_Services
    {
    public:
        static inline const QString KEY_WATCH = "watch";
        static inline const QString KEY_SERVER = "server";
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
        Qx::GenericError parseServer(Server& serverBuffer, const QJsonValue& jvServer);
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
        static inline const QString EXTRA_ARG = R"(--extra-"%1"")";
        static inline const QString MSG_ARG = R"(--msg="%1")";

    // Class functions
    public:
        static QString parametersFromStandard(QString originalAppPath, QString originalAppParams);
    };

//-Class Variables-----------------------------------------------------------------------------------------------
public:
    // Paths
    static inline const QString LOGOS_PATH = "Data/Images/Logos";
    static inline const QString SCREENSHOTS_PATH = "Data/Images/Screenshots";
    static inline const QString EXTRAS_PATH = "Extras";
    static inline const QString MAIN_EXE_PATH = "Launcher/Flashpoint.exe";
    static inline const QString DATABASE_PATH = "Data/flashpoint.sqlite";
    static inline const QString SERVICES_JSON_PATH = "Data/services.json";
    static inline const QString CONFIG_JSON_PATH = "Launcher/config.json";
    static inline const QString VER_TXT_PATH = "version.txt";

    // Version check
    static inline const QByteArray TARGET_EXE_SHA256 = Qx::ByteArray::RAWFromStringHex("825dcf27b93214ec32aa8084ab8d6c3a6f35a7d8132643cb7561ea5674c3b325");
    static inline const QString TARGET_VER_STRING = R"(Flashpoint 8.2 Ultimate - "Approaching Planet Nine")";

    // Database
    static inline const QString DATABASE_CONNECTION_NAME = "Flashpoint Database";
    static inline const QList<DBTableSpecs> DATABASE_SPECS_LIST = {{DBTable_Game::NAME, DBTable_Game::COLUMN_LIST},
                                                                        {DBTable_Add_App::NAME, DBTable_Add_App::COLUMN_LIST},
                                                                        {DBTable_Playlist::NAME, DBTable_Playlist::COLUMN_LIST},
                                                                        {DBTable_Playlist_Game::NAME, DBTable_Playlist_Game::COLUMN_LIST}};
    static inline const QString GENERAL_QUERY_SIZE_COMMAND = "COUNT(1)";

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
    Install(QString installPath);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~Install();

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    static bool pathIsValidInstall(QString installPath, CompatLevel compatLevel);

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
    QSqlError initialGameQuery(QList<DBQueryBuffer>& resultBuffer, QSet<QString> selectedPlatforms) const;
    QSqlError initialAddAppQuery(DBQueryBuffer& resultBuffer) const;
    QSqlError initialPlaylistQuery(DBQueryBuffer& resultBuffer, QSet<QString> selectedPlaylists) const;
    QSqlError initialPlaylistGameQuery(QList<QPair<DBQueryBuffer, QUuid>>& resultBuffer, const QList<QUuid>& knownPlaylistsToQuery) const;

    // Queries - CLIFp
    QSqlError queryEntryID(DBQueryBuffer& resultBuffer, QUuid appID) const;
    QSqlError queryEntryAddApps(DBQueryBuffer& resultBuffer, QUuid appID) const;

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



#endif // FLASHPOINTINSTALL_H
