#include "flashpoint-install.h"
#include "qx-io.h"
#include "qx-windows.h"

namespace FP
{
//===============================================================================================================
// INSTALL::StartStop
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator== (const Install::StartStop& lhs, const Install::StartStop& rhs) noexcept
{
    return lhs.path == rhs.path && lhs.filename == rhs.filename && lhs.arguments == rhs.arguments;
}

//-Hashing------------------------------------------------------------------------------------------------------
uint qHash(const Install::StartStop& key, uint seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.path);
    seed = hash(seed, key.filename);
    seed = hash(seed, key.arguments);

    return seed;
}

//===============================================================================================================
// INSTALL::TagCategory
//===============================================================================================================

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator< (const Install::TagCategory& lhs, const Install::TagCategory& rhs) noexcept { return lhs.name < rhs.name; }

//===============================================================================================================
// INSTALL::SettingsReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Install::SettingsReader::SettingsReader(Settings* targetSettings, std::shared_ptr<QFile> sourceJsonFile) :
    mSourceJsonFile(sourceJsonFile)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError Install::SettingsReader::readInto()
{
    // Load original JSON file
    QByteArray settingsData;
    Qx::IOOpReport settingsLoadReport = Qx::readAllBytesFromFile(settingsData, *mSourceJsonFile);

    if(!settingsLoadReport.wasSuccessful())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), settingsLoadReport.getOutcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument settingsDocument = QJsonDocument::fromJson(settingsData, &parseError);

    if(settingsDocument.isNull())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), parseError.errorString());
    else
    {
        // Ensure top level container is object
        if(!settingsDocument.isObject())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        return parseDocument(settingsDocument);
    }
}

//===============================================================================================================
// INSTALL::ConfigReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Install::ConfigReader::ConfigReader(Config* targetConfig, std::shared_ptr<QFile> sourceJsonFile) :
    SettingsReader(targetConfig, sourceJsonFile)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Install::ConfigReader::parseDocument(const QJsonDocument &configDoc)
{
    // Get derivation specific target
    Config* targetConfig = static_cast<Config*>(mTargetSettings);

    // Get values
    Qx::GenericError valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(targetConfig->flashpointPath, configDoc.object(), JsonObject_Config::KEY_FLASHPOINT_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetConfig->startServer, configDoc.object(), JsonObject_Config::KEY_START_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(targetConfig->server, configDoc.object(), JsonObject_Config::KEY_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Return invalid error on success
    return Qx::GenericError();

}

//===============================================================================================================
// INSTALL::PreferencesReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Install::PreferencesReader::PreferencesReader(Preferences* targetPreferences, std::shared_ptr<QFile> sourceJsonFile) :
    SettingsReader(targetPreferences, sourceJsonFile)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Install::PreferencesReader::parseDocument(const QJsonDocument &prefDoc)
{
    // Get derivation specific target
    Preferences* targetPreferences = static_cast<Preferences*>(mTargetSettings);

    // Get values
    Qx::GenericError valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->imageFolderPath, prefDoc.object(), JsonObject_Preferences::KEY_IMAGE_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->jsonFolderPath, prefDoc.object(), JsonObject_Preferences::KEY_JSON_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(targetPreferences->dataPacksFolderPath, prefDoc.object(), JsonObject_Preferences::KEY_DATA_PACKS_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Return invalid error on success
    return Qx::GenericError();
}

//===============================================================================================================
// INSTALL::ServicesReader
//===============================================================================================================

//-Constructor--------------------------------------------------------------------------------------------------------
//Public:
Install::ServicesReader::ServicesReader(Services* targetServices, std::shared_ptr<QFile> sourceJsonFile, const Install& hostInstall) :
    SettingsReader(targetServices, sourceJsonFile),
    mHostInstall(hostInstall)
{}

//-Instance Functions-------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Install::ServicesReader::parseDocument(const QJsonDocument &servicesDoc)
{
    // Get derivation specific target
    Services* targetServices = static_cast<Services*>(mTargetSettings);

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get watches
    // TODO: include logs

    // Get servers
    QJsonArray jaServers;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaServers, servicesDoc.object(), JsonObject_Services::KEY_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse servers
    for(const QJsonValue& jvServer : qAsConst(jaServers))
    {
        ServerDaemon serverBuffer;
        if((valueError = parseServerDaemon(serverBuffer, jvServer)).isValid())
            return valueError;

        targetServices->servers.insert(serverBuffer.name, serverBuffer);
    }

    // Get daemons
    QJsonArray jaDaemons;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaDaemons, servicesDoc.object(), JsonObject_Services::KEY_DAEMON)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse daemons
    for(const QJsonValue& jvDaemon : qAsConst(jaDaemons))
    {
        ServerDaemon daemonBuffer;
        if((valueError = parseServerDaemon(daemonBuffer, jvDaemon)).isValid())
            return valueError;

        targetServices->daemons.insert(daemonBuffer.name, daemonBuffer);
    }

    // Get starts
    QJsonArray jaStarts;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStarts, servicesDoc.object(), JsonObject_Services::KEY_START)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse starts
    for(const QJsonValue& jvStart : qAsConst(jaStarts))
    {
        StartStop startStopBuffer;
        if((valueError = parseStartStop(startStopBuffer, jvStart)).isValid())
            return valueError;

        targetServices->starts.insert(startStopBuffer);
    }

    // Get stops
    QJsonArray jaStops;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStops, servicesDoc.object(), JsonObject_Services::KEY_STOP)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse starts
    for(const QJsonValue& jvStop : qAsConst(jaStops))
    {
        StartStop startStopBuffer;
        if((valueError = parseStartStop(startStopBuffer, jvStop)).isValid())
            return valueError;

        targetServices->stops.insert(startStopBuffer);
    }

    // Return invalid error on success
    return Qx::GenericError();
}

Qx::GenericError Install::ServicesReader::parseServerDaemon(ServerDaemon& serverBuffer, const QJsonValue& jvServer)
{
    // Ensure array element is Object
    if(!jvServer.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get server Object
    QJsonObject joServer = jvServer.toObject();

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get direct values
    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.name, joServer, JsonObject_Server::KEY_NAME)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.path, joServer, JsonObject_Server::KEY_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.filename, joServer, JsonObject_Server::KEY_FILENAME)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.kill, joServer, JsonObject_Server::KEY_KILL)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Get arguments
    QJsonArray jaArgs;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaArgs, joServer, JsonObject_Server::KEY_ARGUMENTS)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    for(const QJsonValue& jvArg : qAsConst(jaArgs))
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        serverBuffer.arguments.append(jvArg.toString());
    }

    // Ensure filename has extension (assuming exe)
    if(QFileInfo(serverBuffer.filename).suffix().isEmpty())
        serverBuffer.filename += ".exe";

    // Resolve macros for relavent variables
    serverBuffer.path = mHostInstall.resolveFlashpointMacros(serverBuffer.path);
    for(QString& arg : serverBuffer.arguments)
        arg = mHostInstall.resolveFlashpointMacros(arg);

    // Return invalid error on success
    return Qx::GenericError();
}

Qx::GenericError Install::ServicesReader::parseStartStop(StartStop& startStopBuffer, const QJsonValue& jvStartStop)
{
    // Ensure return buffer is null
    startStopBuffer = StartStop();

    // Ensure array element is Object
    if(!jvStartStop.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get server Object
    QJsonObject joStartStop = jvStartStop.toObject();

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get direct values
    if((valueError = Qx::Json::checkedKeyRetrieval(startStopBuffer.path, joStartStop , JsonObject_StartStop::KEY_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(startStopBuffer.filename, joStartStop, JsonObject_StartStop::KEY_FILENAME)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Get arguments
    QJsonArray jaArgs;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaArgs, joStartStop, JsonObject_StartStop::KEY_ARGUMENTS)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    for(const QJsonValue& jvArg : qAsConst(jaArgs))
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mSourceJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        startStopBuffer.arguments.append(jvArg.toString());
    }

    // Ensure filename has extension (assuming exe)
    if(QFileInfo(startStopBuffer.filename).suffix().isEmpty())
        startStopBuffer.filename += ".exe";

    // Resolve macros for relavent variables
    startStopBuffer.path = mHostInstall.resolveFlashpointMacros(startStopBuffer.path);
    for(QString& arg : startStopBuffer.arguments)
        arg = mHostInstall.resolveFlashpointMacros(arg);

    // Return invalid error on success
    return Qx::GenericError();
}

//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath) :
    mValid(false) // Install is invalid until proven otherwise
{
    QScopeGuard validityGuard([this](){ nullify(); }); // Automatically nullify on fail

    // VALIDITY CHECKES/////////////////////////////
    //TODO: Try to disolve this function by simply attempting these steps in the constructor and check afterwards if the instance is valid via
    // something like isNull(). There may be a catch to this as if recalled correctly this was considered before and abandonded because of a conflict
    // with the approach, but otherwise this method would be much more preferable and less redundant

    // Initialize static files and directories
    mRootDirectory = QDir(installPath);
    mLauncherFile = std::make_unique<QFile>(installPath + "/" + LAUNCHER_PATH);
    mDatabaseFile = std::make_unique<QFile>(installPath + "/" + DATABASE_PATH);
    mConfigJsonFile = std::make_shared<QFile>(installPath + "/" + CONFIG_JSON_PATH);
    mPreferencesJsonFile = std::make_shared<QFile>(installPath + "/" + PREFERENCES_JSON_PATH);
    mDataPackMounterFile = std::make_shared<QFile>(installPath + "/" + DATA_PACK_MOUNTER_PATH);
    mVersionFile = std::make_unique<QFile>(installPath + "/" + VER_TXT_PATH);
    mExtrasDirectory = QDir(installPath + "/" + EXTRAS_PATH);

    //-Check install validity--------------------------------------------

    // Check for file existance
    const QList<const QFile* const> filesToCheck{
        mDatabaseFile.get(),
        mConfigJsonFile.get(),
        mPreferencesJsonFile.get(),
        mLauncherFile.get(),
        mVersionFile.get(),
        mDataPackMounterFile.get()
    };

    for(const QFile* const file : filesToCheck)
    {
        QFileInfo fileInfo(*file);
        if(!fileInfo.exists() || !fileInfo.isFile())
        {
            mErrorString = FILE_DNE.arg(fileInfo.filePath());
            return;
        }
    }

    // Get settings
    Qx::GenericError readReport;

    ConfigReader configReader(&mConfig, mConfigJsonFile);
    if((readReport = configReader.readInto()).isValid())
    {
        mErrorString = readReport.primaryInfo() + " [" + readReport.secondaryInfo() + "]";
        return;
    }

    PreferencesReader prefReader(&mPreferences, mPreferencesJsonFile);
    if((readReport = prefReader.readInto()).isValid())
    {
        mErrorString = readReport.primaryInfo() + " [" + readReport.secondaryInfo() + "]";
        return;
    }
    mServicesJsonFile = std::make_shared<QFile>(installPath + "/" + mPreferences.jsonFolderPath + "/" + SERVICES_JSON_NAME);
    mLogosDirectory = QDir(installPath + "/" + mPreferences.imageFolderPath + '/' + LOGOS_FOLDER_NAME);
    mScreenshotsDirectory = QDir(installPath + "/" + mPreferences.imageFolderPath + '/' + SCREENSHOTS_FOLDER_NAME);

    ServicesReader servicesReader(&mServices, mServicesJsonFile, *this);
    if((readReport = servicesReader.readInto()).isValid())
    {
        mErrorString = readReport.primaryInfo() + " [" + readReport.secondaryInfo() + "]";
        return;
    }

    // Give the OK
    mValid = true;
    validityGuard.dismiss();
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
Install::~Install()
{
    closeThreadedDatabaseConnection();
}

//-Class Functions------------------------------------------------------------------------------------------------
//Public:
Qx::GenericError FP::Install::appInvolvesSecurePlayer(bool& involvesBuffer, QFileInfo appInfo)
{
    // Reset buffer
    involvesBuffer = false;

    if(appInfo.fileName().contains(SECURE_PLAYER_INFO.baseName()))
    {
        involvesBuffer = true;
        return Qx::GenericError();
    }
    else if(appInfo.suffix().compare("bat", Qt::CaseInsensitive) == 0)
    {
        // Check if bat uses secure player
        QFile batFile(appInfo.absoluteFilePath());
        Qx::IOOpReport readReport = Qx::fileContainsString(involvesBuffer, batFile, SECURE_PLAYER_INFO.baseName());

        // Check for read errors
        if(!readReport.wasSuccessful())
            return Qx::GenericError(Qx::GenericError::Critical, readReport.getOutcome(), readReport.getOutcomeInfo());
        else
            return Qx::GenericError();
    }
    else
        return Qx::GenericError();
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
void Install::nullify()
{
    // Files and directories
    mRootDirectory = QDir();
    mLogosDirectory = QDir();
    mScreenshotsDirectory = QDir();
    mExtrasDirectory = QDir();
    mLauncherFile.reset();
    mDatabaseFile.reset();
    mConfigJsonFile.reset();
    mPreferencesJsonFile.reset();
    mServicesJsonFile.reset();
    mDataPackMounterFile.reset();
    mVersionFile.reset();

    // Settings
    Config mConfig = {};
    Preferences mPreferences = {};
    Services mServices = {};
}

QSqlDatabase Install::getThreadedDatabaseConnection() const
{
    QString threadedName = DATABASE_CONNECTION_NAME + QString::number((quint64)QThread::currentThread(), 16);

    if(QSqlDatabase::contains(threadedName))
        return QSqlDatabase::database(threadedName, false);
    else
    {
        QSqlDatabase fpDB = QSqlDatabase::addDatabase("QSQLITE", threadedName);
        fpDB.setConnectOptions("QSQLITE_OPEN_READONLY");
        fpDB.setDatabaseName(mDatabaseFile->fileName());
        return fpDB;
    }
}

QSqlError Install::makeNonBindQuery(DBQueryBuffer& resultBuffer, QSqlDatabase* database, QString queryCommand, QString sizeQueryCommand) const
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
bool Install::isValid() const { return mValid; }
QString Install::errorString() const { return mErrorString; }

QString Install::resolveFlashpointMacros(QString macroString) const
{
    // Resolve all known macros
    macroString.replace(MACRO_FP_PATH, getPath());

    return macroString;
}

QString Install::versionString() const
{
    // Check version file
    QString readVersion = QString();
    if(mVersionFile->exists())
        Qx::readTextFromFile(readVersion, *mVersionFile, Qx::TextPos::START).wasSuccessful();

    return readVersion;
}

QString Install::launcherChecksum() const
{
    QString launcherHash;
    Qx::calculateFileChecksum(launcherHash, *mLauncherFile, QCryptographicHash::Sha256);

    return launcherHash;
}

QSqlError Install::openThreadDatabaseConnection()
{
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    if(fpDB.open())
        return QSqlError(); // Empty error on success
    else
        return fpDB.lastError(); // Open error on fail
}

void Install::closeThreadedDatabaseConnection() { getThreadedDatabaseConnection().close(); }
bool Install::databaseConnectionOpenInThisThread() { return getThreadedDatabaseConnection().isOpen(); }

Install::Config Install::getConfig() const { return mConfig; }
Install::Preferences Install::getPreferences() const { return mPreferences; }
Install::Services Install::getServices() const { return mServices; }

QSqlError Install::checkDatabaseForRequiredTables(QSet<QString>& missingTablesReturnBuffer) const
{
    // Prep return buffer
    missingTablesReturnBuffer.clear();

    for(const DBTableSpecs& tableAndColumns : DATABASE_SPECS_LIST)
        missingTablesReturnBuffer.insert(tableAndColumns.name);

    // Get tables from DB
    QSqlDatabase fpDB = getThreadedDatabaseConnection();
    QStringList existingTables = fpDB.tables();

    // Return if DB error occured
    if(fpDB.lastError().isValid())
        return fpDB.lastError();

    for(const QString& table : existingTables)
        missingTablesReturnBuffer.remove(table);

    // Return an invalid error
    return  QSqlError();
}

QSqlError Install::checkDatabaseForRequiredColumns(QSet<QString> &missingColumsReturnBuffer) const
{

    // Ensure return buffer starts empty
    missingColumsReturnBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Ensure each table has the required columns
    QSet<QString> existingColumns;

    for(const DBTableSpecs& tableAndColumns : DATABASE_SPECS_LIST)
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

QSqlError Install::populateAvailableItems()
{
    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Ensure lists are reset
    mPlatformList.clear();
    mPlaylistList.clear();

    // Make platform query
    QSqlQuery platformQuery("SELECT DISTINCT " + DBTable_Game::COL_PLATFORM + " FROM " + DBTable_Game::NAME, fpDB);

    // Return if error occurs
    if(platformQuery.lastError().isValid())
        return platformQuery.lastError();

    // Parse query
    while(platformQuery.next())
        mPlatformList.append(platformQuery.value(DBTable_Game::COL_PLATFORM).toString());

    // Sort list
    mPlatformList.sort();

    // Make playlist query
    QSqlQuery playlistQuery("SELECT DISTINCT " + DBTable_Playlist::COL_TITLE + " FROM " + DBTable_Playlist::NAME, fpDB);

    // Return if error occurs
    if(playlistQuery.lastError().isValid())
        return playlistQuery.lastError();

    // Parse query
    while(playlistQuery.next())
        mPlaylistList.append(playlistQuery.value(DBTable_Playlist::COL_TITLE).toString());

    // Sort list
    mPlaylistList.sort();

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::populateTags()
{
    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Ensure list is reset
    mTagMap.clear();

    // Temporary id map
    QHash<int, QString> primaryAliases;

    // Make tag category query
    QSqlQuery categoryQuery("SELECT `" + DBTable_Tag_Category::COLUMN_LIST.join("`,`") + "` FROM " + DBTable_Tag_Category::NAME, fpDB);

    // Return if error occurs
    if(categoryQuery.lastError().isValid())
        return categoryQuery.lastError();

    // Parse query
    while(categoryQuery.next())
    {
        TagCategory tc;
        tc.name = categoryQuery.value(DBTable_Tag_Category::COL_NAME).toString();
        tc.color = QColor(categoryQuery.value(DBTable_Tag_Category::COL_COLOR).toString());

        mTagMap[categoryQuery.value(DBTable_Tag_Category::COL_ID).toInt()] = tc;
    }

    // Make tag alias query
    QSqlQuery aliasQuery("SELECT `" + DBTable_Tag_Alias::COLUMN_LIST.join("`,`") + "` FROM " + DBTable_Tag_Alias::NAME, fpDB);

    // Return if error occurs
    if(aliasQuery.lastError().isValid())
        return aliasQuery.lastError();

    // Parse query
    while(aliasQuery.next())
        primaryAliases[aliasQuery.value(DBTable_Tag_Alias::COL_ID).toInt()] = aliasQuery.value(DBTable_Tag_Alias::COL_NAME).toString();

    // Make tag query
    QSqlQuery tagQuery("SELECT `" + DBTable_Tag::COLUMN_LIST.join("`,`") + "` FROM " + DBTable_Tag::NAME, fpDB);

    // Return if error occurs
    if(tagQuery.lastError().isValid())
        return tagQuery.lastError();

    // Parse query
    while(tagQuery.next())
    {
        Tag tag;
        tag.id = tagQuery.value(DBTable_Tag::COL_ID).toInt();
        tag.primaryAlias = primaryAliases.value(tagQuery.value(DBTable_Tag::COL_PRIMARY_ALIAS_ID).toInt());

        mTagMap[tagQuery.value(DBTable_Tag::COL_CATEGORY_ID).toInt()].tags[tag.id] = tag;
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::queryGamesByPlatform(QList<DBQueryBuffer>& resultBuffer, QStringList platforms, InclusionOptions inclusionOptions,
                                        const QList<QUuid>& idInclusionFilter) const
{
    // Ensure return buffer is reset
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Determine game exclusion filter from tag exclusions if applicable
    QSet<QUuid> idExclusionFilter;
    if(!inclusionOptions.excludedTagIds.isEmpty())
    {
        // Make game tag sets query
        QString tagIdCSV = Qx::String::join(inclusionOptions.excludedTagIds, [](int tagId){return QString::number(tagId);}, "','");
        QSqlQuery tagQuery("SELECT `" + DBTable_Game_Tags_Tag::COL_GAME_ID + "` FROM " + DBTable_Game_Tags_Tag::NAME +
                           " WHERE " + DBTable_Game_Tags_Tag::COL_TAG_ID + " IN('" + tagIdCSV + "')", fpDB);

        QSqlError tagQueryError = tagQuery.lastError();
        if(tagQueryError.isValid())
            return tagQueryError;

        // Populate exclusion filter
        while(tagQuery.next())
            idExclusionFilter.insert(tagQuery.value(DBTable_Game_Tags_Tag::COL_GAME_ID).toUuid());
    }

    for(const QString& platform : platforms) // Naturally returns empty list if no platforms are selected
    {
        // Create platform query string
        QString placeholder = ":platform";
        QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game::NAME + " WHERE " +
                                   DBTable_Game::COL_PLATFORM + " = " + placeholder + " AND ";

        // Handle filtering
        QString filteredQueryCommand = baseQueryCommand.append(inclusionOptions.includeAnimations ? GAME_AND_ANIM_FILTER : GAME_ONLY_FILTER);

        if(!idExclusionFilter.isEmpty())
        {
            QString gameIdCSV = Qx::String::join(idExclusionFilter, [](QUuid id){return id.toString(QUuid::WithoutBraces);}, "','");
            filteredQueryCommand += " AND " + DBTable_Game::COL_ID + " NOT IN('" + gameIdCSV + "')";
        }

        if(!idInclusionFilter.isEmpty())
        {
            QString gameIdCSV = Qx::String::join(idInclusionFilter, [](QUuid id){return id.toString(QUuid::WithoutBraces);}, "','");
            filteredQueryCommand += " AND " + DBTable_Game::COL_ID + " IN('" + gameIdCSV + "')";
        }

        // Create final command strings
        QString mainQueryCommand = filteredQueryCommand.arg("`" + DBTable_Game::COLUMN_LIST.join("`,`") + "`");
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

QSqlError Install::queryAllAddApps(DBQueryBuffer& resultBuffer) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Make query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Add_App::NAME;
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Add_App::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    resultBuffer.source = DBTable_Add_App::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryPlaylistsByName(DBQueryBuffer& resultBuffer, QStringList playlists) const
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
        resultBuffer = DBQueryBuffer();

        // Get database
        QSqlDatabase fpDB = getThreadedDatabaseConnection();

        // Create selected playlists query string
        QString placeHolders = QString("?,").repeated(playlists.size());
        placeHolders.chop(1); // Remove trailing ?
        QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Playlist::NAME + " WHERE " +
                DBTable_Playlist::COL_TITLE + " IN (" + placeHolders + ") AND " +
                DBTable_Playlist::COL_LIBRARY + " = '" + DBTable_Playlist::ENTRY_GAME_LIBRARY + "'";
        QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Playlist::COLUMN_LIST.join("`,`") + "`");
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
        resultBuffer.source = DBTable_Playlist::NAME;
        resultBuffer.result = mainQuery;
        resultBuffer.size = querySize;

        // Return invalid SqlError
        return QSqlError();
    }
}

QSqlError Install::queryPlaylistGamesByPlaylist(QList<DBQueryBuffer>& resultBuffer, const QList<QUuid>& playlistIDs) const
{
    // Ensure return buffer is empty
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    for(QUuid playlistID : playlistIDs) // Naturally returns empty list if no playlists are selected
    {
        // Query all games for the current playlist
        QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Playlist_Game::NAME + " WHERE " +
                DBTable_Playlist_Game::COL_PLAYLIST_ID + " = '" + playlistID.toString(QUuid::WithoutBraces) + "'";
        QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Playlist_Game::COLUMN_LIST.join("`,`") + "`");
        QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

        // Make query
        QSqlError queryError;
        DBQueryBuffer queryResult;
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

QSqlError Install::queryPlaylistGameIDs(DBQueryBuffer& resultBuffer, const QList<QUuid>& playlistIDs) const
{
    // Ensure return buffer is empty
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Create playlist ID query string
    QString idCSV = Qx::String::join(playlistIDs, [](QUuid id){return id.toString(QUuid::WithoutBraces);}, "','");

    // Query all game IDs that fall under given the playlists
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Playlist_Game::NAME + " WHERE " +
            DBTable_Playlist_Game::COL_PLAYLIST_ID + " IN('" + idCSV + "')";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Playlist_Game::COL_GAME_ID + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = DBTable_Playlist_Game::NAME;

    if((queryError = makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand)).isValid())
        return queryError;

    // Return invalid SqlError
    return QSqlError();

}

QSqlError Install::queryAllEntryTags(DBQueryBuffer& resultBuffer) const
{
    // Ensure return buffer is empty
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Query all tags tied to game IDs
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game_Tags_Tag::NAME;
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Game_Tags_Tag::COL_GAME_ID + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = DBTable_Playlist_Game::NAME;

    // Return invalid SqlError
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryEntryByID(DBQueryBuffer& resultBuffer, QUuid appID) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Check for entry as a game first
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game::NAME + " WHERE " +
            DBTable_Game::COL_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Game::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = DBTable_Game::NAME;

    if((queryError = makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand)).isValid())
        return queryError;

    // Return result if one or more results were found (reciever handles situation in latter case)
    if(resultBuffer.size >= 1)
        return QSqlError();

    // Check for entry as an additional app second
    baseQueryCommand = "SELECT %1 FROM " + DBTable_Add_App::NAME + " WHERE " +
        DBTable_Add_App::COL_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Add_App::COLUMN_LIST.join("`,`") + "`");
    sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query and return result regardless of outcome
    resultBuffer.source = DBTable_Add_App::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryEntriesByTitle(DBQueryBuffer& resultBuffer, QString title) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Escape title
    title.replace(R"(')", R"('')");

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Check for entry as a game first
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game::NAME + " WHERE " +
            DBTable_Game::COL_TITLE + " == '" + title + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Game::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = DBTable_Game::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryEntryDataByID(DBQueryBuffer& resultBuffer, QUuid appID) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Setup ID query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game_Data::NAME + " WHERE " +
            DBTable_Game_Data::COL_GAME_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Game_Data::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = DBTable_Game_Data::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryEntryAddApps(DBQueryBuffer& resultBuffer, QUuid appID, bool playableOnly) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Make query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Add_App::NAME + " WHERE " +
            DBTable_Add_App::COL_PARENT_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    if(playableOnly)
        baseQueryCommand += " AND " + DBTable_Add_App::COL_APP_PATH + " NOT IN ('" + DBTable_Add_App::ENTRY_EXTRAS +
                            "','" + DBTable_Add_App::ENTRY_MESSAGE + "') AND " + DBTable_Add_App::COL_AUTORUN +
                            " != 1";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Add_App::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    resultBuffer.source = DBTable_Add_App::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryDataPackSource(DBQueryBuffer& resultBuffer) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Setup ID query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Source::NAME;
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Source::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = DBTable_Source::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryEntrySourceData(DBQueryBuffer& resultBuffer, QString appSha256Hex) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Setup ID query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Source_Data::NAME + " WHERE " +
            DBTable_Source_Data::COL_SHA256 + " == '" + appSha256Hex + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Source_Data::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query
    QSqlError queryError;
    resultBuffer.source = DBTable_Source_Data::NAME;

    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryAllGameIDs(DBQueryBuffer& resultBuffer, LibraryFilter includeFilter) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Make query
    QString baseQueryCommand = "SELECT %2 FROM " + DBTable_Game::NAME + " WHERE " +
                               DBTable_Game::COL_STATUS + " != '" + DBTable_Game::ENTRY_NOT_WORK + "'%1";
    baseQueryCommand = baseQueryCommand.arg(includeFilter == LibraryFilter::Game ? " AND " + GAME_ONLY_FILTER : (includeFilter == LibraryFilter::Anim ? " AND " + ANIM_ONLY_FILTER : ""));
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Game::COL_ID + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    resultBuffer.source = DBTable_Game::NAME;
    return makeNonBindQuery(resultBuffer, &fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::entryUsesDataPack(bool& resultBuffer, QUuid gameId) const
{
    // Default return buffer to false
    resultBuffer = false;

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Make query
    QString packCheckQueryCommand = "SELECT " + GENERAL_QUERY_SIZE_COMMAND + " FROM " + DBTable_Game_Data::NAME + " WHERE " +
                                   DBTable_Game_Data::COL_GAME_ID + " == '" + gameId.toString(QUuid::WithoutBraces) + "'";

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

QString Install::getPath() const { return mRootDirectory.absolutePath(); }
QStringList Install::getPlatformList() const { return mPlatformList; }
QStringList Install::getPlaylistList() const { return mPlaylistList; }
QDir Install::getLogosDirectory() const { return mLogosDirectory; }
QDir Install::getScrenshootsDirectory() const { return mScreenshotsDirectory; }
QDir Install::getExtrasDirectory() const { return mExtrasDirectory; }
QString Install::getDataPackMounterPath() const { return mDataPackMounterFile->fileName(); }
QMap<int, Install::TagCategory> Install::getTags() const { return mTagMap; }

}
