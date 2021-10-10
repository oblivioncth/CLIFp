#ifndef FLASHPOINT_INSTALL_H
#define FLASHPOINT_INSTALL_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QtSql>
#include "qx.h"

#include "fp-json.h"
#include "fp-macro.h"
#include "fp-db.h"

namespace FP
{

class Install
{
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

    // Settings
    static inline const QString MACRO_FP_PATH = "<fpPath>";

    // Error
    static inline const QString ERR_INVALID = "Invalid Flashpoint Install:";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    // Validity
    bool mValid;
    Qx::GenericError mError;

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

    // Database
    DB* mDatabase;

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

public:
    // Validity
    bool isValid() const;
    Qx::GenericError error() const;

    // General information
    QString versionString() const;
    QString launcherChecksum() const;

    // Database
    DB* database(QSqlError* error = nullptr);

    // Support Application Checks
    Json::Config config() const;
    Json::Preferences preferences() const;
    Json::Services services() const;

    // Data access
    QString fullPath() const;
    QDir logosDirectory() const;
    QDir screenshootsDirectory() const;
    QDir extrasDirectory() const;
    QString datapackMounterPath() const;
    const MacroResolver* macroResolver() const;
};

}



#endif // FLASHPOINT_INSTALL_H
