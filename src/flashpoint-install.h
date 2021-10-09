#ifndef FLASHPOINT_INSTALL_H
#define FLASHPOINT_INSTALL_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QtSql>
#include "qx.h"

#include "flashpoint-json.h"
#include "flashpoint-macro.h"

namespace FP
{

class Install
{
//-Class Enums---------------------------------------------------------------------------------------------------
public:
    enum class LibraryFilter{ Game, Anim, Either };

//-Class Structs-------------------------------------------------------------------------------------------------
public:
    struct InclusionOptions
    {
        QSet<int> excludedTagIds;
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

    struct Tag
    {
        int id;
        QString primaryAlias;
    };

    struct TagCategory
    {
        QString name;
        QColor color;
        QMap<int, Tag> tags;

        friend bool operator< (const TagCategory& lhs, const TagCategory& rhs) noexcept;
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

    class DBTable_Game_Data
    {
    public:
        static inline const QString NAME = "game_data";

        static inline const QString COL_ID = "id";
        static inline const QString COL_GAME_ID = "gameId";
        static inline const QString COL_TITLE = "title";
        static inline const QString COL_DATE_ADDED = "dateAdded";
        static inline const QString COL_SHA256 = "sha256";
        static inline const QString COL_CRC32 = "crc32";
        static inline const QString COL_PRES_ON_DISK = "presentOnDisk";
        static inline const QString COL_PATH = "path";
        static inline const QString COL_SIZE = "size";
        static inline const QString COL_PARAM = "parameters";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_GAME_ID, COL_TITLE, COL_DATE_ADDED, COL_SHA256, COL_CRC32, COL_PRES_ON_DISK, COL_PATH, COL_SIZE, COL_PARAM};
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

    class DBTable_Source
    {
    public:
        static inline const QString NAME = "source";

        static inline const QString COL_ID = "id";
        static inline const QString COL_NAME = "name";
        static inline const QString COL_DATE_ADDED = "dateAdded";
        static inline const QString COL_LAST_UPDATED = "lastUpdated";
        static inline const QString COL_SRC_FILE_URL = "sourceFileUrl";
        static inline const QString COL_BASE_URL = "baseUrl";
        static inline const QString COL_COUNT = "count";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_NAME, COL_DATE_ADDED, COL_LAST_UPDATED, COL_SRC_FILE_URL, COL_BASE_URL, COL_COUNT};
    };

    class DBTable_Source_Data
    {
    public:
        static inline const QString NAME = "source_data";

        static inline const QString COL_ID = "id";
        static inline const QString COL_SOURCE_ID = "sourceId";
        static inline const QString COL_SHA256 = "sha256";
        static inline const QString COL_URL_PATH = "urlPath";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_SOURCE_ID, COL_SHA256, COL_URL_PATH};
    };

    class DBTable_Game_Tags_Tag
    {
    public:
        static inline const QString NAME = "game_tags_tag";

        static inline const QString COL_GAME_ID = "gameId";
        static inline const QString COL_TAG_ID = "tagId";

        static inline const QStringList COLUMN_LIST = {COL_GAME_ID, COL_TAG_ID};
    };

    class DBTable_Tag
    {
    public:
        static inline const QString NAME = "tag";

        static inline const QString COL_ID = "id";
        static inline const QString COL_PRIMARY_ALIAS_ID = "primaryAliasId";
        static inline const QString COL_CATEGORY_ID = "categoryId";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_PRIMARY_ALIAS_ID, COL_CATEGORY_ID};
    };

    class DBTable_Tag_Alias
    {
    public:
        static inline const QString NAME = "tag_alias";

        static inline const QString COL_ID = "id";
        static inline const QString COL_TAG_ID = "tagId";
        static inline const QString COL_NAME = "name";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_TAG_ID, COL_NAME};
    };

    class DBTable_Tag_Category
    {
    public:
        static inline const QString NAME = "tag_category";

        static inline const QString COL_ID = "id";
        static inline const QString COL_NAME = "name";
        static inline const QString COL_COLOR = "color";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_NAME, COL_COLOR};

    };

//-Class Variables-----------------------------------------------------------------------------------------------
private:
    // Validity check fail reasons
    static inline const QString FILE_DNE = "A required file does not exist: %1";

public:
    // Static paths
    static inline const QString LAUNCHER_PATH = "Launcher/Flashpoint.exe";
    static inline const QString DATABASE_PATH = "Data/flashpoint.sqlite";
    static inline const QString CONFIG_JSON_PATH = "Launcher/config.json";
    static inline const QString PREFERENCES_JSON_PATH = "preferences.json";
    static inline const QString DATA_PACK_MOUNTER_PATH = "FPSoftware/fpmount/fpmount.exe";
    static inline const QString VER_TXT_PATH = "version.txt";

    // File Info
    static inline const QFileInfo SECURE_PLAYER_INFO = QFileInfo("FlashpointSecurePlayer.exe");

    // Dynamic path file names
    static inline const QString SERVICES_JSON_NAME = "services.json";

    // Static Folders
    static inline const QString EXTRAS_PATH = "Extras";

    // Dynamic path folder names
    static inline const QString LOGOS_FOLDER_NAME = "Logos";
    static inline const QString SCREENSHOTS_FOLDER_NAME = "Screenshots";

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

    // Settings
    static inline const QString MACRO_FP_PATH = "<fpPath>";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Validity
    bool mValid;
    QString mErrorString;

    // Files and directories
    QDir mRootDirectory;
    QDir mLogosDirectory;
    QDir mScreenshotsDirectory;
    QDir mExtrasDirectory;
    std::unique_ptr<QFile> mLauncherFile;
    std::unique_ptr<QFile> mDatabaseFile;
    std::shared_ptr<QFile> mConfigJsonFile;
    std::shared_ptr<QFile> mPreferencesJsonFile;
    std::shared_ptr<QFile> mServicesJsonFile;
    std::shared_ptr<QFile> mDataPackMounterFile;
    std::unique_ptr<QFile> mVersionFile;

    // Settings
    Json::Config mConfig;
    Json::Preferences mPreferences;
    Json::Services mServices;

    // Database information
    QStringList mPlatformList;
    QStringList mPlaylistList;
    QMap<int, TagCategory> mTagMap; // Order matters for display in tag selector

    // Utilities
    MacroResolver* mMacroResolver;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Install(QString installPath);

//-Desctructor-------------------------------------------------------------------------------------------------
public:
    ~Install();

//-Class Functions------------------------------------------------------------------------------------------------------
public:
    static Qx::GenericError appInvolvesSecurePlayer(bool& involvesBuffer, QFileInfo appInfo);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    void nullify();
    QSqlDatabase getThreadedDatabaseConnection() const;
    QSqlError makeNonBindQuery(DBQueryBuffer& resultBuffer, QSqlDatabase* database, QString queryCommand, QString sizeQueryCommand) const;

public:
    // General information
    bool isValid() const;
    QString errorString() const;
    QString versionString() const;
    QString launcherChecksum() const;

    // Connection
    QSqlError openThreadDatabaseConnection();
    void closeThreadedDatabaseConnection();
    bool databaseConnectionOpenInThisThread();

    // Support Application Checks
    Json::Config getConfig() const;
    Json::Preferences getPreferences() const;
    Json::Services getServices() const;

    // Requirement Checking
    QSqlError checkDatabaseForRequiredTables(QSet<QString>& missingTablesBuffer) const;
    QSqlError checkDatabaseForRequiredColumns(QSet<QString>& missingColumsBuffer) const;

    // Commands
    QSqlError populateAvailableItems();
    QSqlError populateTags();

    // Queries - OFLIb
    QSqlError queryGamesByPlatform(QList<DBQueryBuffer>& resultBuffer, QStringList platforms, InclusionOptions inclusionOptions,
                                   const QList<QUuid>& idInclusionFilter = {}) const;
    QSqlError queryAllAddApps(DBQueryBuffer& resultBuffer) const;
    QSqlError queryPlaylistsByName(DBQueryBuffer& resultBuffer, QStringList playlists) const;
    QSqlError queryPlaylistGamesByPlaylist(QList<DBQueryBuffer>& resultBuffer, const QList<QUuid>& playlistIDs) const;
    QSqlError queryPlaylistGameIDs(DBQueryBuffer& resultBuffer, const QList<QUuid>& playlistIDs) const;
    QSqlError queryAllEntryTags(DBQueryBuffer& resultBuffer) const;

    // Queries - CLIFp
    QSqlError queryEntryByID(DBQueryBuffer& resultBuffer, QUuid appID) const;
    QSqlError queryEntriesByTitle(DBQueryBuffer& resultBuffer, QString title) const;
    QSqlError queryEntryDataByID(DBQueryBuffer& resultBuffer, QUuid appID) const;
    QSqlError queryEntryAddApps(DBQueryBuffer& resultBuffer, QUuid appID, bool playableOnly = false) const;
    QSqlError queryDataPackSource(DBQueryBuffer& resultBuffer) const;
    QSqlError queryEntrySourceData(DBQueryBuffer& resultBuffer, QString appSha256Hex) const;
    QSqlError queryAllGameIDs(DBQueryBuffer& resultBuffer, LibraryFilter filter) const;

    // Checks
    QSqlError entryUsesDataPack(bool& resultBuffer, QUuid gameId) const;

    // Data access
    QString getPath() const;
    QStringList getPlatformList() const;
    QStringList getPlaylistList() const;
    QDir getLogosDirectory() const;
    QDir getScrenshootsDirectory() const;
    QDir getExtrasDirectory() const;
    QString getDataPackMounterPath() const;
    QMap<int, TagCategory> getTags() const;
    const MacroResolver* getMacroResolver() const;
};

}



#endif // FLASHPOINT_INSTALL_H
