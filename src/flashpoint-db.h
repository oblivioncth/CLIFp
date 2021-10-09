#ifndef FLASHPOINT_DB_H
#define FLASHPOINT_DB_H

#include <QStringList>
#include <QtSql>

namespace FP
{

class DB
{
//-Inner Classes-------------------------------------------------------------------------------------------------
public:
    class Table_Game
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

    class Table_Game_Data
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

    class Table_Add_App
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

    class Table_Playlist
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

    class Table_Playlist_Game
    {
    public:
        static inline const QString NAME = "playlist_game";

        static inline const QString COL_ID = "id";
        static inline const QString COL_PLAYLIST_ID = "playlistId";
        static inline const QString COL_ORDER = "order";
        static inline const QString COL_GAME_ID = "gameId";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_PLAYLIST_ID, COL_ORDER, COL_GAME_ID};
    };

    class Table_Source
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

    class Table_Source_Data
    {
    public:
        static inline const QString NAME = "source_data";

        static inline const QString COL_ID = "id";
        static inline const QString COL_SOURCE_ID = "sourceId";
        static inline const QString COL_SHA256 = "sha256";
        static inline const QString COL_URL_PATH = "urlPath";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_SOURCE_ID, COL_SHA256, COL_URL_PATH};
    };

    class Table_Game_Tags_Tag
    {
    public:
        static inline const QString NAME = "game_tags_tag";

        static inline const QString COL_GAME_ID = "gameId";
        static inline const QString COL_TAG_ID = "tagId";

        static inline const QStringList COLUMN_LIST = {COL_GAME_ID, COL_TAG_ID};
    };

    class Table_Tag
    {
    public:
        static inline const QString NAME = "tag";

        static inline const QString COL_ID = "id";
        static inline const QString COL_PRIMARY_ALIAS_ID = "primaryAliasId";
        static inline const QString COL_CATEGORY_ID = "categoryId";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_PRIMARY_ALIAS_ID, COL_CATEGORY_ID};
    };

    class Table_Tag_Alias
    {
    public:
        static inline const QString NAME = "tag_alias";

        static inline const QString COL_ID = "id";
        static inline const QString COL_TAG_ID = "tagId";
        static inline const QString COL_NAME = "name";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_TAG_ID, COL_NAME};
    };

    class Table_Tag_Category
    {
    public:
        static inline const QString NAME = "tag_category";

        static inline const QString COL_ID = "id";
        static inline const QString COL_NAME = "name";
        static inline const QString COL_COLOR = "color";

        static inline const QStringList COLUMN_LIST = {COL_ID, COL_NAME, COL_COLOR};

    };

//-Structs-----------------------------------------------------------------------------------------------------
public:
    struct TableSpecs
    {
        QString name;
        QStringList columns;
    };

    struct QueryBuffer
    {
        QString source;
        QSqlQuery result;
        int size = 0;
    };
};

}



#endif // FLASHPOINT_DB_H
