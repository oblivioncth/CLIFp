#include "flashpointinstall.h"
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
// INSTALL::JSONServicesReader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::JSONServicesReader::JSONServicesReader(Services* targetServices, std::shared_ptr<QFile> targetJSONFile)
    : mTargetServices(targetServices), mTargetJSONFile(targetJSONFile) {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Install::JSONServicesReader::parseServicesDocument(const QJsonDocument& servicesDoc)
{
    // Ensure top level container is object
    if(!servicesDoc.isObject())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get watches
    // TODO: include logs

    // Get servers
    QJsonArray jaServers;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaServers, servicesDoc.object(), JSONObject_Services::KEY_SERVER)).isValid())
        return valueError;

    // Parse servers
    for(const QJsonValue& jvServer : jaServers)
    {
        Server serverBuffer;
        if((valueError = parseServer(serverBuffer, jvServer)).isValid())
            return valueError;

        mTargetServices->servers.insert(serverBuffer.name, serverBuffer);
    }

    // Get starts
    QJsonArray jaStarts;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStarts, servicesDoc.object(), JSONObject_Services::KEY_START)).isValid())
        return valueError;

    // Parse starts
    for(const QJsonValue& jvStart : jaStarts)
    {
        StartStop startStopBuffer;
        if((valueError = parseStartStop(startStopBuffer, jvStart)).isValid())
            return valueError;

        mTargetServices->starts.insert(startStopBuffer);
    }

    // Get stops
    QJsonArray jaStops;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStops, servicesDoc.object(), JSONObject_Services::KEY_STOP)).isValid())
        return valueError;

    // Parse starts
    for(const QJsonValue& jvStop : jaStops)
    {
        StartStop startStopBuffer;
        if((valueError = parseStartStop(startStopBuffer, jvStop)).isValid())
            return valueError;

        mTargetServices->stops.insert(startStopBuffer);
    }

    // Return invalid error on success
    return Qx::GenericError();
}

Qx::GenericError Install::JSONServicesReader::parseServer(Server& serverBuffer, const QJsonValue& jvServer)
{
    // Ensure array element is Object
    if(!jvServer.isObject())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get server Object
    QJsonObject joServer = jvServer.toObject();

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get direct values
    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.name, joServer, JSONObject_Server::KEY_NAME)).isValid())
        return valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.path, joServer, JSONObject_Server::KEY_PATH)).isValid())
        return valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.filename, joServer, JSONObject_Server::KEY_FILENAME)).isValid())
        return valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(serverBuffer.kill, joServer, JSONObject_Server::KEY_KILL)).isValid())
        return valueError;

    // Get arguments
    QJsonArray jaArgs;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaArgs, joServer, JSONObject_Server::KEY_ARGUMENTS)).isValid())
        return valueError;

    for(const QJsonValue& jvArg : jaArgs)
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        serverBuffer.arguments.append(jvArg.toString());
    }

    // Return invalid error on success
    return Qx::GenericError();
}

Qx::GenericError Install::JSONServicesReader::parseStartStop(StartStop& startStopBuffer, const QJsonValue& jvStartStop)
{
    // Ensure return buffer is null
    startStopBuffer = StartStop();

    // Ensure array element is Object
    if(!jvStartStop.isObject())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get server Object
    QJsonObject joStartStop = jvStartStop.toObject();

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get direct values
    if((valueError = Qx::Json::checkedKeyRetrieval(startStopBuffer.path, joStartStop , JSONObject_StartStop::KEY_PATH)).isValid())
        return valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(startStopBuffer.filename, joStartStop, JSONObject_StartStop::KEY_FILENAME)).isValid())
        return valueError;

    // Get arguments
    QJsonArray jaArgs;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaArgs, joStartStop, JSONObject_StartStop::KEY_ARGUMENTS)).isValid())
        return valueError;

    for(const QJsonValue& jvArg : jaArgs)
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        startStopBuffer.arguments.append(jvArg.toString());
    }

    // Return invalid error on success
    return Qx::GenericError();
}

//Public:
Qx::GenericError Install::JSONServicesReader::readInto()
{
    // Load original JSON file
    QByteArray servicesData;
    Qx::IOOpReport servicesLoadReport = Qx::readAllBytesFromFile(servicesData, *mTargetJSONFile);

    if(!servicesLoadReport.wasSuccessful())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), servicesLoadReport.getOutcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument servicesDocument = QJsonDocument::fromJson(servicesData, &parseError);

    if(servicesDocument.isNull())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), parseError.errorString());
    else
        return parseServicesDocument(servicesDocument);
}

//===============================================================================================================
// INSTALL::JSONServicesReader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::JSONConfigReader::JSONConfigReader(Config* targetConfig, std::shared_ptr<QFile> targetJSONFile)
    : mTargetConfig(targetConfig), mTargetJSONFile(targetJSONFile) {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Install::JSONConfigReader::parseConfigDocument(const QJsonDocument& configDoc)
{
    // Ensure top level container is object
    if(!configDoc.isObject())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get values
    Qx::GenericError valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetConfig->startServer, configDoc.object(), JSONObject_Config::KEY_START_SERVER)).isValid())
        return valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetConfig->server, configDoc.object(), JSONObject_Config::KEY_SERVER)).isValid())
        return valueError;

    // Return invalid error on success
    return Qx::GenericError();

}

//Public:
Qx::GenericError Install::JSONConfigReader::readInto()
{
    // Load original JSON file
    QByteArray configData;
    Qx::IOOpReport configLoadReport = Qx::readAllBytesFromFile(configData, *mTargetJSONFile);

    if(!configLoadReport.wasSuccessful())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), configLoadReport.getOutcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument configDocument = QJsonDocument::fromJson(configData, &parseError);

    if(configDocument.isNull())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mTargetJSONFile->fileName()), parseError.errorString());
    else
        return parseConfigDocument(configDocument);
}

//===============================================================================================================
// INSTALL::CLIFp
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QString Install::CLIFp::parametersFromStandard(QString originalAppPath, QString originalAppParams)
{
    if(originalAppPath == DBTable_Add_App::ENTRY_MESSAGE)
        return MSG_ARG.arg(originalAppParams);
    else if(originalAppPath == DBTable_Add_App::ENTRY_EXTRAS)
        return EXTRA_ARG.arg(originalAppParams);
    else
        return APP_ARG.arg(originalAppPath) + " " + PARAM_ARG.arg(originalAppParams);
}

//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath)
{
    // Ensure instance will be valid
    if(!pathIsValidInstall(installPath))
        assert("Cannot create a Install instance with an invalid installPath. Check first with Install::pathIsValidInstall(QString).");

    // Initialize files and directories;
    mRootDirectory = QDir(installPath);
    mLogosDirectory = QDir(installPath + "/" + LOGOS_PATH);
    mScreenshotsDirectory = QDir(installPath + "/" + SCREENSHOTS_PATH);
    mExtrasDirectory = QDir(installPath + "/" + EXTRAS_PATH);
    mMainEXEFile = std::make_unique<QFile>(installPath + "/" + MAIN_EXE_PATH);
    mCLIFpEXEFile = std::make_unique<QFile>(installPath + "/" + CLIFp::EXE_NAME);
    mDatabaseFile = std::make_unique<QFile>(installPath + "/" + DATABASE_PATH);
    mServicesJSONFile = std::make_shared<QFile>(installPath + "/" + SERVICES_JSON_PATH);
    mConfigJSONFile = std::make_shared<QFile>(installPath + "/" + CONFIG_JSON_PATH);
    mVersionTXTFile = std::make_unique<QFile>(installPath + "/" + VER_TXT_PATH);
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
Install::~Install()
{
    closeThreadedDatabaseConnection();
}

//-Class Functions------------------------------------------------------------------------------------------------
//Public:
bool Install::pathIsValidInstall(QString installPath)
{
    QFileInfo logosFolder(installPath + "/" + LOGOS_PATH);
    QFileInfo screenshotsFolder(installPath + "/" + SCREENSHOTS_PATH);
    QFileInfo extrasFolder(installPath + "/" + EXTRAS_PATH);
    QFileInfo mainEXE(installPath + "/" + MAIN_EXE_PATH);
    QFileInfo database(installPath + "/" + DATABASE_PATH);
    QFileInfo services(installPath + "/" + SERVICES_JSON_PATH);
    QFileInfo config(installPath + "/" + CONFIG_JSON_PATH);
    QFileInfo version(installPath + "/" + VER_TXT_PATH);

    return logosFolder.exists() && logosFolder.isDir() &&
           screenshotsFolder.exists() && screenshotsFolder.isDir() &&
           extrasFolder.exists() && extrasFolder.isDir() &&
           mainEXE.exists() && mainEXE.isExecutable() &&
           database.exists() && database.isFile() &&
           services.exists() && services.isFile() &&
           config.exists() && config.isFile() &&
           version.exists() && version.isFile();
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
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

QSqlError Install::makeNonBindQuery(DBQueryBuffer& resultBuffer, const QSqlDatabase& database, QString queryCommand, QString sizeQueryCommand) const
{
    // Create main query
    QSqlQuery mainQuery(database);
    mainQuery.setForwardOnly(true);
    mainQuery.prepare(queryCommand);

    // Execute query and return if error occurs
    if(!mainQuery.exec())
        return mainQuery.lastError();

    // Create size query
    QSqlQuery sizeQuery(database);
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
bool Install::matchesTargetVersion() const
{    
    // Check exe checksum
    mMainEXEFile->open(QFile::ReadOnly);
    QByteArray mainEXEFileData = mMainEXEFile->readAll();
    mMainEXEFile->close();

    if(Qx::Integrity::generateChecksum(mainEXEFileData, QCryptographicHash::Sha256) != TARGET_EXE_SHA256)
        return false;

    // Check version file
    QString readVersion;
    if(!mVersionTXTFile->exists())
        return false;

    if(!Qx::readTextFromFile(readVersion, *mVersionTXTFile, Qx::TextPos::START).wasSuccessful())
        return false;

    if(readVersion != TARGET_VER_STRING)
        return false;

    // Return true if all passes
    return true;
}

bool Install::hasCLIFp() const
{
    QFileInfo presentInfo(*mCLIFpEXEFile);
    return presentInfo.exists() && presentInfo.isFile();
}

Qx::MMRB Install::currentCLIFpVersion() const
{
    if(!hasCLIFp())
        return Qx::MMRB();
    else
        return Qx::getFileDetails(mCLIFpEXEFile->fileName()).getFileVersion();
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

Qx::GenericError Install::getConfig(Config& configBuffer)
{
    // Ensure return services is null
    configBuffer = Config();

    // Create reader instance
    JSONConfigReader jsReader(&configBuffer, mConfigJSONFile);

    // Read services file
    return jsReader.readInto();
}

Qx::GenericError Install::getServices(Services &servicesBuffer)
{
    // Ensure return services is null
    servicesBuffer = Services();

    // Create reader instance
    JSONServicesReader jsReader(&servicesBuffer, mServicesJSONFile);

    // Read services file
    return jsReader.readInto();
}

QSqlError Install::checkDatabaseForRequiredTables(QSet<QString>& missingTablesReturnBuffer) const
{
    // Prep return buffer
    missingTablesReturnBuffer.clear();

    for(DBTableSpecs tableAndColumns : DATABASE_SPECS_LIST)
        missingTablesReturnBuffer.insert(tableAndColumns.name);

    // Get tables from DB
    QSqlDatabase fpDB = getThreadedDatabaseConnection();
    QStringList existingTables = fpDB.tables();

    // Return if DB error occured
    if(fpDB.lastError().isValid())
        return fpDB.lastError();

    for(QString table : existingTables)
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

    for(DBTableSpecs tableAndColumns : DATABASE_SPECS_LIST)
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
        for(QString column : tableAndColumns.columns)
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

    // Make platform query
    QSqlQuery platformQuery("SELECT DISTINCT " + DBTable_Game::COL_PLATFORM + " FROM " + DBTable_Game::NAME, fpDB);

    // Return if error occurs
    if(platformQuery.lastError().isValid())
        return platformQuery.lastError();

    // Ensure lists are reset
    mPlatformList.clear();
    mPlaylistList.clear();

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

bool Install::deployCLIFp(QString& errorMessage)
{
    // Ensure error message is null
    errorMessage = QString();

    // Delete existing if present
    if(QFile::exists(mCLIFpEXEFile->fileName()) && QFileInfo(mCLIFpEXEFile->fileName()).isFile())
    {
        if(!mCLIFpEXEFile->remove())
        {
            errorMessage = mCLIFpEXEFile->errorString();
            return false;
        }
    }

    // Deploy new
    QFile internalCLIFp(":/res/file/" + FP::Install::CLIFp::EXE_NAME);
    if(!internalCLIFp.copy(mCLIFpEXEFile->fileName()))
    {
        errorMessage = internalCLIFp.errorString();
        return false;
    }

    // Remove default read-only state
    mCLIFpEXEFile->setPermissions(QFile::ReadOther | QFile::WriteOther);

    // Return true on
    return true;

}

QSqlError Install::initialGameQuery(QList<DBQueryBuffer>& resultBuffer, QSet<QString> selectedPlatforms) const
{
    // Ensure return buffer is reset
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    for(const QString& platform : selectedPlatforms) // Naturally returns empty list if no platforms are selected
    {
        // Create platform query string
        QString placeholder = ":platform";
        QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game::NAME + " WHERE " +
                DBTable_Game::COL_PLATFORM + " = " + placeholder + " AND " +
                DBTable_Game::COL_LIBRARY + " = '" + DBTable_Game::GAME_LIBRARY + "'";

        QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Game::COLUMN_LIST.join("`,`") + "`");
        QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

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

        // Add result to buffer
        resultBuffer.append({platform, initialQuery, querySize});
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::initialAddAppQuery(DBQueryBuffer& resultBuffer) const
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
    return makeNonBindQuery(resultBuffer, fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::initialPlaylistQuery(DBQueryBuffer& resultBuffer, QSet<QString> selectedPlaylists) const
{
    // Return blank result if no playlists are selected
    if(selectedPlaylists.isEmpty())
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
        QString placeHolders = QString("?,").repeated(selectedPlaylists.size());
        placeHolders.chop(1); // Remove trailing ?
        QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Playlist::NAME + " WHERE " +
                DBTable_Playlist::COL_TITLE + " IN (" + placeHolders + ") AND " +
                DBTable_Playlist::COL_LIBRARY + " = '" + DBTable_Playlist::GAME_LIBRARY + "'";
        QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Playlist::COLUMN_LIST.join("`,`") + "`");
        QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

        // Create main query and bind selected playlists
        QSqlQuery mainQuery(fpDB);
        mainQuery.setForwardOnly(true);
        mainQuery.prepare(mainQueryCommand);
        for(const QString& playlist : selectedPlaylists)
            mainQuery.addBindValue(playlist);

        // Execute query and return if error occurs
        if(!mainQuery.exec())
            return mainQuery.lastError();

        // Create size query
        QSqlQuery sizeQuery(fpDB);
        sizeQuery.setForwardOnly(true);
        sizeQuery.prepare(sizeQueryCommand);

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

QSqlError Install::initialPlaylistGameQuery(QList<QPair<DBQueryBuffer, QUuid>>& resultBuffer, const QList<QUuid>& knownPlaylistsToQuery) const
{
    // Ensure return buffer is empty
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    for(QUuid playlistID : knownPlaylistsToQuery) // Naturally returns empty list if no playlists are selected
    {
        // Query all games for the current playlist
        QString baseQueryCommand = "SELECT `" + DBTable_Playlist_Game::COLUMN_LIST.join("`,`") + "` FROM " + DBTable_Playlist_Game::NAME + " WHERE " +
                DBTable_Playlist_Game::COL_PLAYLIST_ID + " = '" + playlistID.toString(QUuid::WithoutBraces) + "'";
        QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Playlist_Game::COLUMN_LIST.join("`,`") + "`");
        QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

        // Make query
        QSqlError queryError;
        DBQueryBuffer queryResult;
        queryResult.source = playlistID.toString();

        if((queryError = makeNonBindQuery(queryResult, fpDB, mainQueryCommand, sizeQueryCommand)).isValid())
            return queryError;

        // Add result to buffer
        resultBuffer.append(qMakePair(queryResult, playlistID));
    }

    // Return invalid SqlError
    return QSqlError();
}

QSqlError Install::queryEntryID(DBQueryBuffer& resultBuffer, QUuid appID) const
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

    if((queryError = makeNonBindQuery(resultBuffer, fpDB, mainQueryCommand, sizeQueryCommand)).isValid())
        return queryError;

    // Return result if one or more result were found (reciever handles situation in latter case)
    if(resultBuffer.size >= 1)
        return QSqlError();

    // Check for entry as an additional app second
    baseQueryCommand = "SELECT %1 FROM " + DBTable_Add_App::NAME + " WHERE " +
        DBTable_Add_App::COL_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
    mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Add_App::COLUMN_LIST.join("`,`") + "`");
    sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Make query and return result regardless of outcome
    return makeNonBindQuery(resultBuffer, fpDB, mainQueryCommand, sizeQueryCommand);
}

QSqlError Install::queryEntryAddApps(DBQueryBuffer& resultBuffer, QUuid appID) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Make query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Add_App::NAME + " WHERE " +
            DBTable_Add_App::COL_PARENT_ID + " != '" + appID.toString(QUuid::WithoutBraces) + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Add_App::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    resultBuffer.source = DBTable_Add_App::NAME;
    return makeNonBindQuery(resultBuffer, fpDB, mainQueryCommand, sizeQueryCommand);
}

QString Install::getPath() const { return mRootDirectory.absolutePath(); }
QStringList Install::getPlatformList() const { return mPlatformList; }
QStringList Install::getPlaylistList() const { return mPlaylistList; }
QDir Install::getLogosDirectory() const { return mLogosDirectory; }
QDir Install::getScrenshootsDirectory() const { return mScreenshotsDirectory; }
QDir Install::getExtrasDirectory() const { return mExtrasDirectory; }
QString Install::getCLIFpPath() const { return mCLIFpEXEFile->fileName(); }

}
