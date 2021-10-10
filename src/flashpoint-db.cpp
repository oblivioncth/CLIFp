#include "flashpoint-db.h"
#include "qx.h"

namespace FP
{

//===============================================================================================================
// DB::TAG_CATEGORY
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator< (const DB::TagCategory& lhs, const DB::TagCategory& rhs) noexcept { return lhs.name < rhs.name; }

//===============================================================================================================
// DB
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
DB::DB(QString databaseName, const Key&) :
    mValid(false), // Instance is invalid until proven otherwise
    mDatabaseName(databaseName)
{
    QScopeGuard validityGuard([this](){ nullify(); }); // Automatically nullify on fail

    // Open database connection
    QSqlError databaseError = openThreadConnection();
    if(databaseError.isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_DATABASE, databaseError.text());
        return;
    }

    // Ensure required database tables are present
    QSet<QString> missingTables;
    if((databaseError = checkDatabaseForRequiredTables(missingTables)).isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_DATABASE, databaseError.text());
        return;
    }

    // Check if tables are missing
    if(!missingTables.isEmpty())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_MISSING_TABLE, QString(),
                         QStringList(missingTables.begin(), missingTables.end()).join("\n"));
        return;
    }

    // Ensure the database contains the required columns
    QSet<QString> missingColumns;
    if((databaseError = checkDatabaseForRequiredColumns(missingColumns)).isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_DATABASE, databaseError.text());
        return;
    }

    // Check if columns are missing
    if(!missingColumns.isEmpty())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_MISSING_TABLE, QString(),
                         QStringList(missingColumns.begin(), missingColumns.end()).join("\n"));
        return;
    }

    // Populate item members
    if((databaseError = populateAvailableItems()).isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_DATABASE, databaseError.text());
        return;
    }

    if((databaseError = populateTags()).isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_DATABASE, databaseError.text());
        return;
    }

    // Give the ok
    mValid = true;
    validityGuard.dismiss();
}

//-Desctructor------------------------------------------------------------------------------------------------
//Public:
DB::~DB()
{
    closeAllConnections();
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
void DB::nullify()
{
    mPlatformList.clear();
    mPlaylistList.clear();
    mTagMap.clear();
}

void DB::closeAllConnections()
{
    QSet<QString>::const_iterator i;
    for(i = mConnections.constBegin(); i != mConnections.constEnd(); i++)
        QSqlDatabase::database(*i, false).close();
}


QSqlDatabase DB::getThreadConnection() const
{
    QString threadedName = DATABASE_CONNECTION_NAME + QString::number((quint64)QThread::currentThread(), 16);

    if(QSqlDatabase::contains(threadedName))
        return QSqlDatabase::database(threadedName, false);
    else
    {
        QSqlDatabase fpDB = QSqlDatabase::addDatabase("QSQLITE", threadedName);
        fpDB.setConnectOptions("QSQLITE_OPEN_READONLY");
        fpDB.setDatabaseName(mDatabaseName);
        return fpDB;
    }
}

QSqlError DB::makeNonBindQuery(QueryBuffer& resultBuffer, QSqlDatabase* database, QString queryCommand, QString sizeQueryCommand) const
{
    // Create main query
    QSqlQuery mainQuery(*database);
    mainQuery.setForwardOnly(true);
    mainQuery.prepare(queryCommand);

    // Execute query and return if error occurs
    if(!mainQuery.exec())
        return mainQuery.lastError();

    // Create size query
    QSqlQuery sizeQuery(*database);
    sizeQuery.setForwardOnly(true);
    sizeQuery.prepare(sizeQueryCommand);

    // Execute query and return if error occurs
    if(!sizeQuery.exec())
        return sizeQuery.lastError();

    // Get query size
    sizeQuery.next();
    int querySize = sizeQuery.value(0).toInt();

    // Set buffer instance to result
    resultBuffer.result = mainQuery;
    resultBuffer.size = querySize;

    // Return invalid SqlError
    return QSqlError();
}

//Public:
bool DB::isValid() { return mValid; }
Qx::GenericError DB::error() { return mError; }

QSqlError DB::openThreadConnection()
{
    QSqlDatabase fpDB = getThreadConnection();

    if(fpDB.open())
    {
        mConnections.insert(fpDB.connectionName());
        return QSqlError(); // Empty error on success
    }
    else
        return fpDB.lastError(); // Open error on fail
}

void DB::closeThreadConnection()
{
    QSqlDatabase db = getThreadConnection();
    mConnections.remove(db.connectionName());
    db.close();
}

bool DB::connectionOpenInThisThread() { return getThreadConnection().isOpen(); }

QSqlError DB::checkDatabaseForRequiredTables(QSet<QString>& missingTablesReturnBuffer) const
{
    // Prep return buffer
    missingTablesReturnBuffer.clear();

    for(const TableSpecs& tableAndColumns : DATABASE_SPECS_LIST)
        missingTablesReturnBuffer.insert(tableAndColumns.name);

    // Get tables from DB
    QSqlDatabase fpDB = getThreadConnection();
    QStringList existingTables = fpDB.tables();

    // Return if DB error occured
    if(fpDB.lastError().isValid())
        return fpDB.lastError();

    for(const QString& table : existingTables)
        missingTablesReturnBuffer.remove(table);

    // Return an invalid error
    return  QSqlError();
}

QSqlError DB::checkDatabaseForRequiredColumns(QSet<QString> &missingColumsReturnBuffer) const
{

    // Ensure return buffer starts empty
    missingColumsReturnBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Ensure each table has the required columns
    QSet<QString> existingColumns;

    for(const TableSpecs& tableAndColumns : DATABASE_SPECS_LIST)
    {
        // Clear previous data
        existingColumns.clear();

        // Make column name query
        QSqlQuery columnQuery("PRAGMA table_info(" + tableAndColumns.name + ")", fpDB);

        // Return if error occurs
        if(columnQuery.lastError().isValid())
            return columnQuery.lastError();

        // Parse query
        while(columnQuery.next())
            existingColumns.insert(columnQuery.value("name").toString());

        // Check for missing columns
        for(const QString& column : tableAndColumns.columns)
            if(!existingColumns.contains(column))
                missingColumsReturnBuffer.insert(tableAndColumns.name + ": " + column);
    }


    // Return invalid SqlError
    return QSqlError();
}

QSqlError DB::populateAvailableItems()
{
    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Ensure lists are reset
    mPlatformList.clear();
    mPlaylistList.clear();

    // Make platform query
    QSqlQuery platformQuery("SELECT DISTINCT " + Table_Game::COL_PLATFORM + " FROM " + Table_Game::NAME, fpDB);

    // Return if error occurs
    if(platformQuery.lastError().isValid())
        return platformQuery.lastError();

    // Parse query
    while(platformQuery.next())
        mPlatformList.append(platformQuery.value(Table_Game::COL_PLATFORM).toString());

    // Sort list
    mPlatformList.sort();

    // Make playlist query
    QSqlQuery playlistQuery("SELECT DISTINCT " + Table_Playlist::COL_TITLE + " FROM " + Table_Playlist::NAME, fpDB);

    // Return if error occurs
    if(playlistQuery.lastError().isValid())
        return playlistQuery.lastError();

    // Parse query
    while(playlistQuery.next())
        mPlaylistList.append(playlistQuery.value(DB::Table_Playlist::COL_TITLE).toString());

    // Sort list
    mPlaylistList.sort();

    // Return invalid SqlError
    return QSqlError();
}

QSqlError DB::populateTags()
{
    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Ensure list is reset
    mTagMap.clear();

    // Temporary id map
    QHash<int, QString> primaryAliases;

    // Make tag category query
    QSqlQuery categoryQuery("SELECT `" + Table_Tag_Category::COLUMN_LIST.join("`,`") + "` FROM " + Table_Tag_Category::NAME, fpDB);

    // Return if error occurs
    if(categoryQuery.lastError().isValid())
        return categoryQuery.lastError();

    // Parse query
    while(categoryQuery.next())
    {
        TagCategory tc;
        tc.name = categoryQuery.value(Table_Tag_Category::COL_NAME).toString();
        tc.color = QColor(categoryQuery.value(Table_Tag_Category::COL_COLOR).toString());

        mTagMap[categoryQuery.value(Table_Tag_Category::COL_ID).toInt()] = tc;
    }

    // Make tag alias query
    QSqlQuery aliasQuery("SELECT `" + Table_Tag_Alias::COLUMN_LIST.join("`,`") + "` FROM " + Table_Tag_Alias::NAME, fpDB);

    // Return if error occurs
    if(aliasQuery.lastError().isValid())
        return aliasQuery.lastError();

    // Parse query
    while(aliasQuery.next())
        primaryAliases[aliasQuery.value(Table_Tag_Alias::COL_ID).toInt()] = aliasQuery.value(Table_Tag_Alias::COL_NAME).toString();

    // Make tag query
    QSqlQuery tagQuery("SELECT `" + Table_Tag::COLUMN_LIST.join("`,`") + "` FROM " + Table_Tag::NAME, fpDB);

    // Return if error occurs
    if(tagQuery.lastError().isValid())
        return tagQuery.lastError();

    // Parse query
    while(tagQuery.next())
    {
        Tag tag;
        tag.id = tagQuery.value(Table_Tag::COL_ID).toInt();
        tag.primaryAlias = primaryAliases.value(tagQuery.value(Table_Tag::COL_PRIMARY_ALIAS_ID).toInt());

        mTagMap[tagQuery.value(Table_Tag::COL_CATEGORY_ID).toInt()].tags[tag.id] = tag;
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError DB::queryGamesByPlatform(QList<QueryBuffer>& resultBuffer, QStringList platforms, InclusionOptions inclusionOptions,
                                        const QList<QUuid>& idInclusionFilter) const
{
    // Ensure return buffer is reset
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Determine game exclusion filter from tag exclusions if applicable
    QSet<QUuid> idExclusionFilter;
    if(!inclusionOptions.excludedTagIds.isEmpty())
    {
        // Make game tag sets query
        QString tagIdCSV = Qx::String::join(inclusionOptions.excludedTagIds, [](int tagId){return QString::number(tagId);}, "','");
        QSqlQuery tagQuery("SELECT `" + Table_Game_Tags_Tag::COL_GAME_ID + "` FROM " + Table_Game_Tags_Tag::NAME +
                           " WHERE " + Table_Game_Tags_Tag::COL_TAG_ID + " IN('" + tagIdCSV + "')", fpDB);

        QSqlError tagQueryError = tagQuery.lastError();
        if(tagQueryError.isValid())
            return tagQueryError;

        // Populate exclusion filter
        while(tagQuery.next())
            idExclusionFilter.insert(tagQuery.value(Table_Game_Tags_Tag::COL_GAME_ID).toUuid());
    }

    for(const QString& platform : platforms) // Naturally returns empty list if no platforms are selected
    {
        // Create platform query string
        QString placeholder = ":platform";
        QString baseQueryCommand = "SELECT %1 FROM " + Table_Game::NAME + " WHERE " +
                                   Table_Game::COL_PLATFORM + " = " + placeholder + " AND ";

        // Handle filtering
        QString filteredQueryCommand = baseQueryCommand.append(inclusionOptions.includeAnimations ? GAME_AND_ANIM_FILTER : GAME_ONLY_FILTER);

        if(!idExclusionFilter.isEmpty())
        {
            QString gameIdCSV = Qx::String::join(idExclusionFilter, [](QUuid id){return id.toString(QUuid::WithoutBraces);}, "','");
            filteredQueryCommand += " AND " + Table_Game::COL_ID + " NOT IN('" + gameIdCSV + "')";
        }

        if(!idInclusionFilter.isEmpty())
        {
            QString gameIdCSV = Qx::String::join(idInclusionFilter, [](QUuid id){return id.toString(QUuid::WithoutBraces);}, "','");
            filteredQueryCommand += " AND " + Table_Game::COL_ID + " IN('" + gameIdCSV + "')";
        }

        // Create final command strings
        QString mainQueryCommand = filteredQueryCommand.arg("`" + Table_Game::COLUMN_LIST.join("`,`") + "`");
        QString sizeQueryCommand = filteredQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

        // Create main query and bind current platform
        QSqlQuery initialQuery(fpDB);
        initialQuery.setForwardOnly(true);
        initialQuery.prepare(mainQueryCommand);
        initialQuery.bindValue(placeholder, platform);

        // Execute query and return if error occurs
        if(!initialQuery.exec())
            return initialQuery.lastError();

        // Create size query and bind current platform
        QSqlQuery sizeQuery(fpDB);
        sizeQuery.prepare(sizeQueryCommand);
        sizeQuery.bindValue(placeholder, platform);

        // Execute query and return if error occurs
        if(!sizeQuery.exec())
            return sizeQuery.lastError();

        // Get query size
        sizeQuery.next();
        int querySize = sizeQuery.value(0).toInt();

        // Add result to buffer if there were any hits
        if(querySize > 0)
            resultBuffer.append({platform, initialQuery, querySize});
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError DB::queryAllAddApps(QueryBuffer& resultBuffer) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Make query
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Add_App::NAME;
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Add_App::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    resultBuffer.source = Table_Add_App::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryPlaylistsByName(QueryBuffer& resultBuffer, QStringList playlists) const
{
    // Return blank result if no playlists are selected
    if(playlists.isEmpty())
    {
        resultBuffer.source = QString();
        resultBuffer.result = QSqlQuery();
        resultBuffer.size = 0;

        return QSqlError();
    }
    else
    {
        // Ensure return buffer is effectively null
        resultBuffer = QueryBuffer();

        // Get database
        QSqlDatabase fpDB = getThreadConnection();

        // Create selected playlists query string
        QString placeHolders = QString("?,").repeated(playlists.size());
        placeHolders.chop(1); // Remove trailing ?
        QString baseQueryCommand = "SELECT %1 FROM " + Table_Playlist::NAME + " WHERE " +
                Table_Playlist::COL_TITLE + " IN (" + placeHolders + ") AND " +
                Table_Playlist::COL_LIBRARY + " = '" + Table_Playlist::ENTRY_GAME_LIBRARY + "'";
        QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Playlist::COLUMN_LIST.join("`,`") + "`");
        QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

        // Create main query and bind selected playlists
        QSqlQuery mainQuery(fpDB);
        mainQuery.setForwardOnly(true);
        mainQuery.prepare(mainQueryCommand);
        for(const QString& playlist : playlists)
            mainQuery.addBindValue(playlist);

        // Execute query and return if error occurs
        if(!mainQuery.exec())
            return mainQuery.lastError();

        // Create size query and bind selected playlists
        QSqlQuery sizeQuery(fpDB);
        sizeQuery.setForwardOnly(true);
        sizeQuery.prepare(sizeQueryCommand);
        for(const QString& playlist : playlists)
            sizeQuery.addBindValue(playlist);

        // Execute query and return if error occurs
        if(!sizeQuery.exec())
            return sizeQuery.lastError();

        // Get query size
        sizeQuery.next();
        int querySize = sizeQuery.value(0).toInt();

        // Set buffer instance to result
        resultBuffer.source = Table_Playlist::NAME;
        resultBuffer.result = mainQuery;
        resultBuffer.size = querySize;

        // Return invalid SqlError
        return QSqlError();
    }
}

QSqlError DB::queryPlaylistGamesByPlaylist(QList<QueryBuffer>& resultBuffer, const QList<QUuid>& playlistIDs) const
{
    // Ensure return buffer is empty
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    for(QUuid playlistID : playlistIDs) // Naturally returns empty list if no playlists are selected
    {
        // Query all games for the current playlist
        QString baseQueryCommand = "SELECT %1 FROM " + Table_Playlist_Game::NAME + " WHERE " +
                Table_Playlist_Game::COL_PLAYLIST_ID + " = '" + playlistID.toString(QUuid::WithoutBraces) + "'";
        QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Playlist_Game::COLUMN_LIST.join("`,`") + "`");
        QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

        // Make query
        QSqlError queryError;
        QueryBuffer queryResult;
        queryResult.source = playlistID.toString();

        if((queryError = makeNonBindQuery(queryResult, &fpDB, mainQueryCommand, sizeQueryCommand)).isValid())
            return queryError;

        // Add result to buffer if there were any hits
        if(queryResult.size > 0)
            resultBuffer.append(queryResult);
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError DB::queryPlaylistGameIDs(QueryBuffer& resultBuffer, const QList<QUuid>& playlistIDs) const
{
    // Ensure return buffer is empty
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Create playlist ID query string
    QString idCSV = Qx::String::join(playlistIDs, [](QUuid id){return id.toString(QUuid::WithoutBraces);}, "','");

    // Query all game IDs that fall under given the playlists
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Playlist_Game::NAME + " WHERE " +
            Table_Playlist_Game::COL_PLAYLIST_ID + " IN('" + idCSV + "')";
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Playlist_Game::COL_GAME_ID + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = Table_Playlist_Game::NAME;

    if((queryError = makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand)).isValid())
        return queryError;

    // Return invalid SqlError
    return QSqlError();

}

QSqlError DB::queryAllEntryTags(QueryBuffer& resultBuffer) const
{
    // Ensure return buffer is empty
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Query all tags tied to game IDs
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Game_Tags_Tag::NAME;
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Game_Tags_Tag::COL_GAME_ID + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = Table_Playlist_Game::NAME;

    // Return invalid SqlError
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryEntryByID(QueryBuffer& resultBuffer, QUuid appID) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Check for entry as a game first
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Game::NAME + " WHERE " +
            Table_Game::COL_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Game::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = Table_Game::NAME;

    if((queryError = makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand)).isValid())
        return queryError;

    // Return result if one or more results were found (reciever handles situation in latter case)
    if(resultBuffer.size >= 1)
        return QSqlError();

    // Check for entry as an additional app second
    baseQueryCommand = "SELECT %1 FROM " + Table_Add_App::NAME + " WHERE " +
        Table_Add_App::COL_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    mainQueryCommand = baseQueryCommand.arg("`" + Table_Add_App::COLUMN_LIST.join("`,`") + "`");
    sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query and return result regardless of outcome
    resultBuffer.source = Table_Add_App::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryEntriesByTitle(QueryBuffer& resultBuffer, QString title) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Escape title
    title.replace(R"(')", R"('')");

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Check for entry as a game first
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Game::NAME + " WHERE " +
            Table_Game::COL_TITLE + " == '" + title + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Game::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = Table_Game::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryEntryDataByID(QueryBuffer& resultBuffer, QUuid appID) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Setup ID query
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Game_Data::NAME + " WHERE " +
            Table_Game_Data::COL_GAME_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Game_Data::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = Table_Game_Data::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryEntryAddApps(QueryBuffer& resultBuffer, QUuid appID, bool playableOnly) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Make query
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Add_App::NAME + " WHERE " +
            Table_Add_App::COL_PARENT_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    if(playableOnly)
        baseQueryCommand += " AND " + Table_Add_App::COL_APP_PATH + " NOT IN ('" + Table_Add_App::ENTRY_EXTRAS +
                            "','" + Table_Add_App::ENTRY_MESSAGE + "') AND " + Table_Add_App::COL_AUTORUN +
                            " != 1";
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Add_App::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    resultBuffer.source = Table_Add_App::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryDataPackSource(QueryBuffer& resultBuffer) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Setup ID query
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Source::NAME;
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Source::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = Table_Source::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryEntrySourceData(QueryBuffer& resultBuffer, QString appSha256Hex) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Setup ID query
    QString baseQueryCommand = "SELECT %1 FROM " + Table_Source_Data::NAME + " WHERE " +
            Table_Source_Data::COL_SHA256 + " == '" + appSha256Hex + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Source_Data::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = Table_Source_Data::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::queryAllGameIDs(QueryBuffer& resultBuffer, LibraryFilter includeFilter) const
{
    // Ensure return buffer is effectively null
    resultBuffer = QueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Make query
    QString baseQueryCommand = "SELECT %2 FROM " + Table_Game::NAME + " WHERE " +
                               Table_Game::COL_STATUS + " != '" + Table_Game::ENTRY_NOT_WORK + "'%1";
    baseQueryCommand = baseQueryCommand.arg(includeFilter == LibraryFilter::Game ? " AND " + GAME_ONLY_FILTER : (includeFilter == LibraryFilter::Anim ? " AND " + ANIM_ONLY_FILTER : ""));
    QString mainQueryCommand = baseQueryCommand.arg("`" + Table_Game::COL_ID + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    resultBuffer.source = Table_Game::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError DB::entryUsesDataPack(bool& resultBuffer, QUuid gameId) const
{
    // Default return buffer to false
    resultBuffer = false;

    // Get database
    QSqlDatabase fpDB = getThreadConnection();

    // Make query
    QString packCheckQueryCommand = "SELECT " + GENERAL_QUERY_SIZE_COMMAND + " FROM " + Table_Game_Data::NAME + " WHERE " +
                                   Table_Game_Data::COL_GAME_ID + " == '" + gameId.toString(QUuid::WithoutBraces) + "'";

    QSqlQuery packCheckQuery(fpDB);
    packCheckQuery.setForwardOnly(true);
    packCheckQuery.prepare(packCheckQueryCommand);

    // Execute query and return if error occurs
    if(!packCheckQuery.exec())
        return packCheckQuery.lastError();

    // Set buffer based on result
    packCheckQuery.next();
    resultBuffer = packCheckQuery.value(0).toInt() > 0;

    // Return invalid SqlError
    return QSqlError();
}

QStringList DB::platformList() const { return mPlatformList; }
QStringList DB::playlistList() const { return mPlaylistList; }
QMap<int, DB::TagCategory> DB::getTags() const { return mTagMap; }

}
