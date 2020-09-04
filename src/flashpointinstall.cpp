#include "flashpointinstall.h"
#include "qx-io.h"
#include "qx-windows.h"

namespace FP
{

//===============================================================================================================
// INSTALL::OFLIb
//===============================================================================================================

//-Class Functions--------------------------------------------------------------------------------------------
//Public:
QString Install::CLIFp::parametersFromStandard(QString originalAppPath, QString originalAppParams)
{
    if(originalAppPath == DBTable_Add_App::ENTRY_MESSAGE)
        return MSG_ARG.arg(originalAppParams);
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
    mMainEXEFile = std::make_unique<QFile>(installPath + "/" + MAIN_EXE_PATH);
    mCLIFpEXEFile = std::make_unique<QFile>(installPath + "/" + CLIFp::EXE_NAME);
    mDatabaseFile = std::make_unique<QFile>(installPath + "/" + DATABASE_PATH);
    mServicesJSONFile = std::make_unique<QFile>(installPath + "/" + SERVICES_JSON_PATH);
    mConfigJSONFile = std::make_unique<QFile>(installPath + "/" + CONFIG_JSON_PATH);
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
    QFileInfo mainEXE(installPath + "/" + MAIN_EXE_PATH);
    QFileInfo database(installPath + "/" + DATABASE_PATH);
    QFileInfo services(installPath + "/" + SERVICES_JSON_PATH);
    QFileInfo config(installPath + "/" + CONFIG_JSON_PATH);
    QFileInfo version(installPath + "/" + VER_TXT_PATH);

    return logosFolder.exists() && logosFolder.isDir() &&
           screenshotsFolder.exists() && screenshotsFolder.isDir() &&
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

Qx::GenericError Install::parseConfig(Config& config)
{
    // Ensure return config is null
    config.startServer = false;
    config.server = QString();

    // Load original JSON file
    QByteArray configData;
    Qx::IOOpReport configLoadReport = Qx::readAllBytesFromFile(configData, *mConfigJSONFile);

    if(!configLoadReport.wasSuccessful())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mConfigJSONFile->fileName()), configLoadReport.getOutcomeInfo());

    // Parse original JSON data
    QJsonParseError parseError;
    QJsonDocument configDocument = QJsonDocument::fromJson(configData, &parseError);

    if(configDocument.isNull())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mConfigJSONFile->fileName()), parseError.errorString());

    // Ensure top level container is object
    if(!configDocument.isObject())
        return Qx::GenericError(QString(), ERR_PARSING_JSON_DOC.arg(mConfigJSONFile->fileName()), ERR_JSON_UNEXP_FOMRAT);

    // Get values
    QJsonValue jsonStartServer = configDocument.object().value(JSONObject_Config::KEY_START_SERVER);

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
    resultBuffer.source = QString();
    resultBuffer.result = QSqlQuery();
    resultBuffer.size = -1;

    // Get database
    QSqlDatabase fpDB = getThreadedDatabaseConnection();

    // Make query
    QString baseQueryCommand = "SELECT %1 FROM " + DBTable_Add_App::NAME + " WHERE " +
            DBTable_Add_App::COL_APP_PATH + " != '" + DBTable_Add_App::ENTRY_EXTRAS + "'";
    QString mainQueryCommand = baseQueryCommand.arg("`" + DBTable_Add_App::COLUMN_LIST.join("`,`") + "`");
    QString sizeQueryCommand = baseQueryCommand.arg(GENERAL_QUERY_SIZE_COMMAND);

    // Create main query
    QSqlQuery mainQuery(fpDB);
    mainQuery.setForwardOnly(true);
    mainQuery.prepare(mainQueryCommand);

    // Execute query and return if error occurs
    if(!mainQuery.exec())
        return mainQuery.lastError();

    // Create size query
    QSqlQuery sizeQuery(fpDB);
    sizeQuery.setForwardOnly(true);
    sizeQuery.prepare(sizeQueryCommand);

    // Execute query and return if error occurs
    if(!sizeQuery.exec())
        return sizeQuery.lastError();

    // Get query size
    sizeQuery.next();
    int querySize = sizeQuery.value(0).toInt();

    // Set buffer instance to result
    resultBuffer.source = DBTable_Add_App::NAME;
    resultBuffer.result = mainQuery;
    resultBuffer.size = querySize;

    // Return invalid SqlError
    return QSqlError();
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
        resultBuffer.source = QString();
        resultBuffer.result = QSqlQuery();
        resultBuffer.size = -1;

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

        // Create main query
        QSqlQuery mainQuery(fpDB);
        mainQuery.setForwardOnly(true);
        mainQuery.prepare(mainQueryCommand);

        // Execute query and return if error occurs
        if(!mainQuery.exec())
            return mainQuery.lastError();

        // Create size query
        QSqlQuery sizeQuery(fpDB);
        sizeQuery.setForwardOnly(true);
        sizeQuery.prepare(sizeQueryCommand);

        // Execute query and return if error occurs
        if(!sizeQuery.exec())
            return sizeQuery.lastError();

        // Get query size
        sizeQuery.next();
        int querySize = sizeQuery.value(0).toInt();

        // Add result to buffer
        resultBuffer.append(qMakePair(DBQueryBuffer{playlistID.toString(), mainQuery, querySize}, playlistID));
    }

    // Return invalid SqlError
    return QSqlError();
}

QString Install::getPath() const { return mRootDirectory.absolutePath(); }
QStringList Install::getPlatformList() const { return mPlatformList; }
QStringList Install::getPlaylistList() const { return mPlaylistList; }
QDir Install::getLogosDirectory() const { return mLogosDirectory; }
QDir Install::getScrenshootsDirectory() const { return mScreenshotsDirectory; }
QString Install::getCLIFpPath() const { return mCLIFpEXEFile->fileName(); }

}
