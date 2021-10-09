#ifndef FLASHPOINT_INSTALL_H
#define FLASHPOINT_INSTALL_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QtSql>
#include "qx.h"

#include "flashpoint-json.h"
#include "flashpoint-macro.h"
#include "flashpoint-db.h"

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
    static inline const QList<DB::TableSpecs> DATABASE_SPECS_LIST = {{DB::Table_Game::NAME, DB::Table_Game::COLUMN_LIST},
                                                                        {DB::Table_Add_App::NAME, DB::Table_Add_App::COLUMN_LIST},
                                                                        {DB::Table_Playlist::NAME, DB::Table_Playlist::COLUMN_LIST},
                                                                        {DB::Table_Playlist_Game::NAME, DB::Table_Playlist_Game::COLUMN_LIST}};
    static inline const QString GENERAL_QUERY_SIZE_COMMAND = "COUNT(1)";

    static inline const QString GAME_ONLY_FILTER = DB::Table_Game::COL_LIBRARY + " = '" + DB::Table_Game::ENTRY_GAME_LIBRARY + "'";
    static inline const QString ANIM_ONLY_FILTER = DB::Table_Game::COL_LIBRARY + " = '" + DB::Table_Game::ENTRY_ANIM_LIBRARY + "'";
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
    QSqlError makeNonBindQuery(DB::QueryBuffer& resultBuffer, QSqlDatabase* database, QString queryCommand, QString sizeQueryCommand) const;

public:
    // General information
    bool isValid() const;
    QString errorString() const;
    QString versionString() const;
    QString launcherChecksum() const;

    // Connection
    QSqlError openThreadedDatabaseConnection();
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
    QSqlError queryGamesByPlatform(QList<DB::QueryBuffer>& resultBuffer, QStringList platforms, InclusionOptions inclusionOptions,
                                   const QList<QUuid>& idInclusionFilter = {}) const;
    QSqlError queryAllAddApps(DB::QueryBuffer& resultBuffer) const;
    QSqlError queryPlaylistsByName(DB::QueryBuffer& resultBuffer, QStringList playlists) const;
    QSqlError queryPlaylistGamesByPlaylist(QList<DB::QueryBuffer>& resultBuffer, const QList<QUuid>& playlistIDs) const;
    QSqlError queryPlaylistGameIDs(DB::QueryBuffer& resultBuffer, const QList<QUuid>& playlistIDs) const;
    QSqlError queryAllEntryTags(DB::QueryBuffer& resultBuffer) const;

    // Queries - CLIFp
    QSqlError queryEntryByID(DB::QueryBuffer& resultBuffer, QUuid appID) const;
    QSqlError queryEntriesByTitle(DB::QueryBuffer& resultBuffer, QString title) const;
    QSqlError queryEntryDataByID(DB::QueryBuffer& resultBuffer, QUuid appID) const;
    QSqlError queryEntryAddApps(DB::QueryBuffer& resultBuffer, QUuid appID, bool playableOnly = false) const;
    QSqlError queryDataPackSource(DB::QueryBuffer& resultBuffer) const;
    QSqlError queryEntrySourceData(DB::QueryBuffer& resultBuffer, QString appSha256Hex) const;
    QSqlError queryAllGameIDs(DB::QueryBuffer& resultBuffer, LibraryFilter filter) const;

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
