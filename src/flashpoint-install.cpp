#include "flashpoint-install.h"
#include "qx-io.h"
#include "qx-windows.h"

namespace FP
{
//===============================================================================================================
// INSTALL
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Install::Install(QString installPath) :
    mValid(false) // Install is invalid until proven otherwise
{
    QScopeGuard validityGuard([this](){ nullify(); }); // Automatically nullify on fail

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
    const QList<const QFile*> filesToCheck{
        mDatabaseFile.get(),
        mConfigJsonFile.get(),
        mPreferencesJsonFile.get(),
        mLauncherFile.get(),
        mVersionFile.get(),
        mDataPackMounterFile.get()
    };

    for(const QFile* file : filesToCheck)
    {
        QFileInfo fileInfo(*file);
        if(!fileInfo.exists() || !fileInfo.isFile())
        {
            mError = Qx::GenericError(Qx::GenericError::Critical, ERR_INVALID, FILE_DNE.arg(fileInfo.filePath()));
            return;
        }
    }

    // Get settings
    Qx::GenericError readReport;

    Json::ConfigReader configReader(&mConfig, mConfigJsonFile);
    if((readReport = configReader.readInto()).isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_INVALID, readReport.primaryInfo() + " [" + readReport.secondaryInfo() + "]");
        return;
    }

    Json::PreferencesReader prefReader(&mPreferences, mPreferencesJsonFile);
    if((readReport = prefReader.readInto()).isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_INVALID, readReport.primaryInfo() + " [" + readReport.secondaryInfo() + "]");
        return;
    }
    mServicesJsonFile = std::make_shared<QFile>(installPath + "/" + mPreferences.jsonFolderPath + "/" + SERVICES_JSON_NAME);
    mLogosDirectory = QDir(installPath + "/" + mPreferences.imageFolderPath + '/' + LOGOS_FOLDER_NAME);
    mScreenshotsDirectory = QDir(installPath + "/" + mPreferences.imageFolderPath + '/' + SCREENSHOTS_FOLDER_NAME);

    Json::ServicesReader servicesReader(&mServices, mServicesJsonFile, mMacroResolver);
    if((readReport = servicesReader.readInto()).isValid())
    {
        mError = Qx::GenericError(Qx::GenericError::Critical, ERR_INVALID, readReport.primaryInfo() + " [" + readReport.secondaryInfo() + "]");
        return;
    }

    // Add database
    mDatabase = new DB(mDatabaseFile->fileName(), {});

    if(!mDatabase->isValid())
    {
        mError = mDatabase->error();
        return;
    }

    // Create macro resolver
    mMacroResolver = new MacroResolver(mRootDirectory.absolutePath(), {});

    // Give the OK
    mValid = true;
    validityGuard.dismiss();
}

//-Destructor------------------------------------------------------------------------------------------------
//Public:
Install::~Install()
{
    mDatabase->closeThreadConnection();
    delete mMacroResolver;
    delete mDatabase;
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
    delete mMacroResolver;
    delete mDatabase;

    // Settings
    Json::Config mConfig = {};
    Json::Preferences mPreferences = {};
    Json::Services mServices = {};
}

//Public:
bool Install::isValid() const { return mValid; }
Qx::GenericError Install::error() const { return mError; }

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

DB* Install::database(QSqlError* error)
{
    /*
     * Automatically manages opening the database if the thread is different. An error here in unlikely,
     * given that open capabilities are testing in the DB constructor, but nonetheless they are possible,
     * so a return state is optionally set here. It is only possible for an error to occur if database
     * isn't already open in the calling thread since an issue can only occur from opening a connection
    */

    QSqlError sqlError;

    if(!mDatabase->connectionOpenInThisThread())
        sqlError = mDatabase->openThreadConnection();

    if(sqlError.isValid())
    {
        if(error)
            *error = sqlError;
        return nullptr;
    }
    else
    {
        if(error)
            *error = QSqlError();
        return mDatabase;
    }
}

Json::Config Install::config() const { return mConfig; }
Json::Preferences Install::preferences() const { return mPreferences; }
Json::Services Install::services() const { return mServices; }

QString Install::fullPath() const { return mRootDirectory.absolutePath(); }
QDir Install::logosDirectory() const { return mLogosDirectory; }
QDir Install::screenshootsDirectory() const { return mScreenshotsDirectory; }
QDir Install::extrasDirectory() const { return mExtrasDirectory; }
QString Install::datapackMounterPath() const { return mDataPackMounterFile->fileName(); }
const MacroResolver* Install::macroResolver() const { return mMacroResolver; }

}
