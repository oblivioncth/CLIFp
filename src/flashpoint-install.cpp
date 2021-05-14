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
// INSTALL::JsonConfigReader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::JsonConfigReader::JsonConfigReader(Config* targetConfig, std::shared_ptr<QFile> targetJsonFile)
    : mTargetConfig(targetConfig), mTargetJsonFile(targetJsonFile) {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Install::JsonConfigReader::parseConfigDocument(const QJsonDocument& configDoc)
{
    // Ensure top level container is object
    if(!configDoc.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get values
    Qx::GenericError valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetConfig->flashpointPath, configDoc.object(), JsonObject_Config::KEY_FLASHPOINT_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetConfig->startServer, configDoc.object(), JsonObject_Config::KEY_START_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetConfig->server, configDoc.object(), JsonObject_Config::KEY_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Return invalid error on success
    return Qx::GenericError();

}

//Public:
Qx::GenericError Install::JsonConfigReader::readInto()
{
    // Load original JSON file
    QByteArray configData;
    Qx::IOOpReport configLoadReport = Qx::readAllBytesFromFile(configData, *mTargetJsonFile);

    if(!configLoadReport.wasSuccessful())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), configLoadReport.getOutcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument configDocument = QJsonDocument::fromJson(configData, &parseError);

    if(configDocument.isNull())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), parseError.errorString());
    else
        return parseConfigDocument(configDocument);
}

//===============================================================================================================
// INSTALL::JsonPreferencesReader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::JsonPreferencesReader::JsonPreferencesReader(Preferences* targetPreferences, std::shared_ptr<QFile> targetJsonFile)
    : mTargetPreferences(targetPreferences), mTargetJsonFile(targetJsonFile) {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError Install::JsonPreferencesReader::parsePreferencesDocument(const QJsonDocument& configDoc)
{
    // Ensure top level container is object
    if(!configDoc.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Get values
    Qx::GenericError valueError;

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetPreferences->imageFolderPath, configDoc.object(), JsonObject_Preferences::KEY_IMAGE_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetPreferences->jsonFolderPath, configDoc.object(), JsonObject_Preferences::KEY_JSON_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);

    if((valueError = Qx::Json::checkedKeyRetrieval(mTargetPreferences->dataPacksFolderPath, configDoc.object(), JsonObject_Preferences::KEY_DATA_PACKS_FOLDER_PATH)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Return invalid error on success
    return Qx::GenericError();

}

//Public:
Qx::GenericError Install::JsonPreferencesReader::readInto()
{
    // Load original JSON file
    QByteArray preferencesData;
    Qx::IOOpReport preferencesLoadReport = Qx::readAllBytesFromFile(preferencesData, *mTargetJsonFile);

    if(!preferencesLoadReport.wasSuccessful())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), preferencesLoadReport.getOutcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument preferencesDocument = QJsonDocument::fromJson(preferencesData, &parseError);

    if(preferencesDocument.isNull())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), parseError.errorString());
    else
        return parsePreferencesDocument(preferencesDocument);
}

//===============================================================================================================
// INSTALL::JsonServicesReader
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::JsonServicesReader::JsonServicesReader(const QString hostInstallPath, Services* targetServices, std::shared_ptr<QFile> targetJsonFile)
    : mHostInstallPath(hostInstallPath), mTargetServices(targetServices), mTargetJsonFile(targetJsonFile) {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Private:
QString Install::JsonServicesReader::resolveFlashpointMacros(QString macroString)
{
    // Resolve all known macros
    macroString.replace(MACRO_FP_PATH, mHostInstallPath);

    return macroString;
}

Qx::GenericError Install::JsonServicesReader::parseServicesDocument(const QJsonDocument& servicesDoc)
{
    // Ensure top level container is object
    if(!servicesDoc.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

    // Value error checking buffer
    Qx::GenericError valueError;

    // Get watches
    // TODO: include logs

    // Get servers
    QJsonArray jaServers;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaServers, servicesDoc.object(), JsonObject_Services::KEY_SERVER)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse servers
    for(const QJsonValue& jvServer : jaServers)
    {
        ServerDaemon serverBuffer;
        if((valueError = parseServerDaemon(serverBuffer, jvServer)).isValid())
            return valueError;

        mTargetServices->servers.insert(serverBuffer.name, serverBuffer);
    }

    // Get daemons
    QJsonArray jaDaemons;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaDaemons, servicesDoc.object(), JsonObject_Services::KEY_DAEMON)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

    // Parse daemons
    for(const QJsonValue& jvDaemon : jaDaemons)
    {
        ServerDaemon daemonBuffer;
        if((valueError = parseServerDaemon(daemonBuffer, jvDaemon)).isValid())
            return valueError;

        mTargetServices->daemons.insert(daemonBuffer.name, daemonBuffer);
    }

    // Get starts
    QJsonArray jaStarts;
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStarts, servicesDoc.object(), JsonObject_Services::KEY_START)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

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
    if((valueError = Qx::Json::checkedKeyRetrieval(jaStops, servicesDoc.object(), JsonObject_Services::KEY_STOP)).isValid())
        return valueError.setErrorLevel(Qx::GenericError::Critical);;

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

Qx::GenericError Install::JsonServicesReader::parseServerDaemon(ServerDaemon& serverBuffer, const QJsonValue& jvServer)
{
    // Ensure array element is Object
    if(!jvServer.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

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

    for(const QJsonValue& jvArg : jaArgs)
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        serverBuffer.arguments.append(jvArg.toString());
    }

    // Ensure filename has extension (assuming exe)
    if(QFileInfo(serverBuffer.filename).suffix().isEmpty())
        serverBuffer.filename += ".exe";

    // Resolve macros for relavent variables
    serverBuffer.path = resolveFlashpointMacros(serverBuffer.path);
    for(QString& arg : serverBuffer.arguments)
        arg = resolveFlashpointMacros(arg);

    // Return invalid error on success
    return Qx::GenericError();
}

Qx::GenericError Install::JsonServicesReader::parseStartStop(StartStop& startStopBuffer, const QJsonValue& jvStartStop)
{
    // Ensure return buffer is null
    startStopBuffer = StartStop();

    // Ensure array element is Object
    if(!jvStartStop.isObject())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

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

    for(const QJsonValue& jvArg : jaArgs)
    {
        // Ensure array element is String
        if(!jvArg.isString())
            return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), ERR_JSON_UNEXP_FORMAT);

        startStopBuffer.arguments.append(jvArg.toString());
    }

    // Ensure filename has extension (assuming exe)
    if(QFileInfo(startStopBuffer.filename).suffix().isEmpty())
        startStopBuffer.filename += ".exe";

    // Resolve macros for relavent variables
    startStopBuffer.path = resolveFlashpointMacros(startStopBuffer.path);
    for(QString& arg : startStopBuffer.arguments)
        arg = resolveFlashpointMacros(arg);

    // Return invalid error on success
    return Qx::GenericError();
}

//Public:
Qx::GenericError Install::JsonServicesReader::readInto()
{
    // Load original JSON file
    QByteArray servicesData;
    Qx::IOOpReport servicesLoadReport = Qx::readAllBytesFromFile(servicesData, *mTargetJsonFile);

    if(!servicesLoadReport.wasSuccessful())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), servicesLoadReport.getOutcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument servicesDocument = QJsonDocument::fromJson(servicesData, &parseError);

    if(servicesDocument.isNull())
        return Qx::GenericError(Qx::GenericError::Critical, ERR_PARSING_JSON_DOC.arg(mTargetJsonFile->fileName()), parseError.errorString());
    else
        return parseServicesDocument(servicesDocument);
}

//===============================================================================================================
// INSTALL::CLIFp
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QString Install::CLIFp::parametersFromStandard(QString originalAppPath, QString originalAppParams)
{
    if(originalAppPath == DBTable_Add_App::ENTRY_MESSAGE)
        return MSG_ARG.arg(originalAppParams) + " -q";
    else if(originalAppPath == DBTable_Add_App::ENTRY_EXTRAS)
        return EXTRA_ARG.arg(originalAppParams) + " -q";
    else
        return APP_ARG.arg(originalAppPath) + " " + PARAM_ARG.arg(originalAppParams) + " -q";
}

//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath, QString clifpSubPath)
{
    // Ensure instance will be at least minimally compatible
    if(!checkInstallValidity(installPath, CompatLevel::Execution).installValid)
        assert("Cannot create a Install instance with an invalid installPath. Check first with Install::checkInstallValidity(QString, CompatLevel).");

    // Initialize static files and directories
    mRootDirectory = QDir(installPath);
    mLauncherFile = std::make_unique<QFile>(installPath + "/" + LAUNCHER_PATH);
    mDatabaseFile = std::make_unique<QFile>(installPath + "/" + DATABASE_PATH);
    mConfigJsonFile = std::make_shared<QFile>(installPath + "/" + CONFIG_JSON_PATH);
    mPreferencesJsonFile = std::make_shared<QFile>(installPath + "/" + PREFERENCES_JSON_PATH);
    mDataPackMounterFile = std::make_shared<QFile>(installPath + "/" + DATA_PACK_MOUNTER_PATH);
    mVersionFile = std::make_unique<QFile>(installPath + "/" + VER_TXT_PATH);
    mExtrasDirectory = QDir(installPath + "/" + EXTRAS_PATH);

    // Initialize flexible files and directories
    mCLIFpFile = std::make_unique<QFile>(installPath + "/" + (clifpSubPath.isNull() ? "" : clifpSubPath + "/") + CLIFp::EXE_NAME); // Defaults to root

    // Get preferences
    Preferences installPreferences;
    getPreferences(installPreferences); // Assume that prefernces can be read since this is checked in checkInstallValidity()

    // Initialize config based files and directories
    mServicesJsonFile = std::make_shared<QFile>(installPath + "/" + installPreferences.jsonFolderPath + "/" + SERVICES_JSON_NAME);
    mLogosDirectory = QDir(installPath + "/" + installPreferences.imageFolderPath + '/' + LOGOS_FOLDER_NAME);
    mScreenshotsDirectory = QDir(installPath + "/" + installPreferences.imageFolderPath + '/' + SCREENSHOTS_FOLDER_NAME);

}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
Install::~Install()
{
    closeThreadedDatabaseConnection();
}

//-Class Functions------------------------------------------------------------------------------------------------
//Public:
Install::ValidityReport Install::checkInstallValidity(QString installPath, CompatLevel compatLevel)
{
    //TODO: Try to disolve this function by simply attempting these steps in the constructor and check afterwards if the instance is valid via
    // something like isNull(). There may be a catch to this as if recalled correctly this was considered before and abandonded because of a conflict
    // with the approach, but otherwise this method would be much more preferable and less redundant

    QFileInfo mainEXE(installPath + "/" + LAUNCHER_PATH);
    QFileInfo database(installPath + "/" + DATABASE_PATH);
    QFileInfo config(installPath + "/" + CONFIG_JSON_PATH);
    QFileInfo preferences(installPath + "/" + PREFERENCES_JSON_PATH);
    QFileInfo version(installPath + "/" + VER_TXT_PATH);

    // Check common requirements
    if(!database.exists() || !database.isFile())
        return ValidityReport{false, FILE_DNE.arg(database.filePath())};
    if(!config.exists() || !config.isFile())
        return ValidityReport{false, FILE_DNE.arg(config.filePath())};

    Config testConfig; // Check that config can be read
    std::shared_ptr<QFile> configFile = std::make_shared<QFile>(config.filePath());
    JsonConfigReader jsConfigReader(&testConfig, configFile);
    Qx::GenericError configReadReport = jsConfigReader.readInto();

    if(configReadReport.isValid())
        return ValidityReport{false, configReadReport.primaryInfo() + " [" + configReadReport.secondaryInfo() + "]"};

    Preferences testPreferences; // Check that preferences can be read
    std::shared_ptr<QFile> preferencesFile = std::make_shared<QFile>(preferences.filePath());
    JsonPreferencesReader jsPrefReader(&testPreferences, preferencesFile);
    Qx::GenericError prefReadReport = jsPrefReader.readInto();

    if(prefReadReport.isValid())
        return ValidityReport{false, prefReadReport.primaryInfo() + " [" + prefReadReport.secondaryInfo() + "]"};

    Services testServices; // Test that services can be read
    std::shared_ptr<QFile> servicesFile = std::make_shared<QFile>(installPath + "/" + testPreferences.jsonFolderPath + "/" + SERVICES_JSON_NAME);
    JsonServicesReader jsServicesReader(installPath, &testServices, servicesFile);
    Qx::GenericError servicesReadReport = jsServicesReader.readInto();

    if(servicesReadReport.isValid())
        return ValidityReport{false, servicesReadReport.primaryInfo() + " [" + servicesReadReport.secondaryInfo() + "]"};

    // Check full compat level requirements
    if(compatLevel == CompatLevel::Full)
    {
        if(!mainEXE.exists() || !mainEXE.isFile())
            return ValidityReport{false, FILE_DNE.arg(config.filePath())};
        if(!version.exists() || !version.isFile())
            return ValidityReport{false, FILE_DNE.arg(version.filePath())};
    }

    // Return valid pass
    return ValidityReport{true, QString()};
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
bool Install::matchesTargetVersion() const
{    
    // Check exe checksum
    mLauncherFile->open(QFile::ReadOnly);
    QByteArray mainEXEFileData = mLauncherFile->readAll();
    mLauncherFile->close();

    if(Qx::Integrity::generateChecksum(mainEXEFileData, QCryptographicHash::Sha256) != TARGET_EXE_SHA256)
        return false;

    // Check version file
    QString readVersion;
    if(!mVersionFile->exists())
        return false;

    if(!Qx::readTextFromFile(readVersion, *mVersionFile, Qx::TextPos::START).wasSuccessful())
        return false;

    if(readVersion != TARGET_ULT_VER_STRING && readVersion != TARGET_INF_VER_STRING)
        return false;

    // Return true if all passes
    return true;
}

bool Install::hasCLIFp() const
{
    QFileInfo presentInfo(*mCLIFpFile);
    return presentInfo.exists() && presentInfo.isFile();
}

Qx::MMRB Install::currentCLIFpVersion() const
{
    if(!hasCLIFp())
        return Qx::MMRB();
    else
        return Qx::getFileDetails(mCLIFpFile->fileName()).getFileVersion();
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

Qx::GenericError Install::getConfig(Config& configBuffer) const
{
    // Ensure return services is null
    configBuffer = Config();

    // Create reader instance
    JsonConfigReader jsReader(&configBuffer, mConfigJsonFile);

    // Read services file
    return jsReader.readInto();
}

Qx::GenericError Install::getPreferences(Preferences& preferencesBuffer) const
{
    // Ensure return services is null
    preferencesBuffer = Preferences();

    // Create reader instance
    JsonPreferencesReader jsReader(&preferencesBuffer, mPreferencesJsonFile);

    // Read services file
    return jsReader.readInto();
}

Qx::GenericError Install::getServices(Services &servicesBuffer) const
{
    // Ensure return services is null
    servicesBuffer = Services();

    // Create reader instance
    JsonServicesReader jsReader(getPath(), &servicesBuffer, mServicesJsonFile);

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
    if(QFile::exists(mCLIFpFile->fileName()) && QFileInfo(mCLIFpFile->fileName()).isFile())
    {
        if(!mCLIFpFile->remove())
        {
            errorMessage = mCLIFpFile->errorString();
            return false;
        }
    }

    // Deploy new
    QFile internalCLIFp(":/res/file/" + FP::Install::CLIFp::EXE_NAME);
    if(!internalCLIFp.copy(mCLIFpFile->fileName()))
    {
        errorMessage = internalCLIFp.errorString();
        return false;
    }

    // Remove default read-only state
    mCLIFpFile->setPermissions(QFile::ReadOther | QFile::WriteOther);

    // Return true on
    return true;

}

QSqlError Install::queryGamesByPlatform(QList<DBQueryBuffer>& resultBuffer, QStringList platforms, InclusionOptions inclusionOptions,
                                        const QList<QUuid>& idFilter) const
{
    // Ensure return buffer is reset
    resultBuffer.clear();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    for(const QString& platform : platforms) // Naturally returns empty list if no platforms are selected
    {
        // Create platform query string
        QString placeholder = ":platform";
        QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game::NAME + " WHERE " +
                DBTable_Game::COL_PLATFORM + " = " + placeholder + " AND ";

        // Handle filtering
        QString filteredQueryCommand = baseQueryCommand.append(inclusionOptions.includeAnimations ? GAME_AND_ANIM_FILTER : GAME_ONLY_FILTER);

        if(!inclusionOptions.includeExtreme)
            filteredQueryCommand += " AND " + DBTable_Game::COL_EXTREME + " = '0'";

        if(!idFilter.isEmpty())
        {
            QString idCSV = Qx::String::join(idFilter, "','", [](QUuid id){return id.toString(QUuid::WithoutBraces);});
            filteredQueryCommand += " AND " + DBTable_Game::COL_ID + " IN('" + idCSV + "')";
        }

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
    QString idCSV = Qx::String::join(playlistIDs, "','", [](QUuid id){return id.toString(QUuid::WithoutBraces);});

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

QSqlError Install::queryEntryDataByID(DBQueryBuffer& resultBuffer, QUuid appID) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Setup ID query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Game_Data::NAME + " WHERE " +
            DBTable_Game_Data::COL_ID + " == '" + appID.toString(QUuid::WithoutBraces) + "'";
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

QSqlError Install::queryEntrySource(DBQueryBuffer& resultBuffer) const
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

QSqlError Install::queryAllGameIDs(DBQueryBuffer& resultBuffer, LibraryFilter filter) const
{
    // Ensure return buffer is effectively null
    resultBuffer = DBQueryBuffer();

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Make query
    QString baseQueryCommand = "SELECT %2 FROM " + DBTable_Game::NAME + " WHERE " +
                               DBTable_Game::COL_STATUS + " != '" + DBTable_Game::ENTRY_NOT_WORK + "'%1";
    baseQueryCommand = baseQueryCommand.arg(filter == LibraryFilter::Game ? " AND " + GAME_ONLY_FILTER : (filter == LibraryFilter::Anim ? " AND " + ANIM_ONLY_FILTER : ""));
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
QString Install::getCLIFpPath() const { return mCLIFpFile->fileName(); }
QString Install::getDataPackMounterPath() const { return mDataPackMounterFile->fileName(); }

}
