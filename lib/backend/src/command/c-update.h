#ifndef CUPDATE_H
#define CUPDATE_H

// Qx Includes
#include <qx/core/qx-json.h>
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class QX_ERROR_TYPE(CUpdateError, "CUpdateError", 1218)
{
    friend class CUpdate;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        AlreadyOpen,
        ConnectionError,
        InvalidUpdateData,
        InvalidReleaseVersion,
        OldProcessNotFinished,
        InvalidPath,
        TransferFail,
        CacheClearFail
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {AlreadyOpen, u"Cannot update when another instance of CLIFp is running."_s},
        {ConnectionError, u"Failed to query the update server."_s},
        {InvalidUpdateData, u"The update server responded with unrecognized data."_s},
        {InvalidReleaseVersion, u"The latest release has an invalid version."_s},
        {OldProcessNotFinished, u"The old version is still running."_s},
        {InvalidPath, u"An update path is invalid."_s},
        {TransferFail, u"File transfer operation failed."_s},
        {CacheClearFail, u"Failed to clear the update cache."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;
    Qx::Severity mSeverity;

//-Constructor-------------------------------------------------------------
private:
    CUpdateError(Type t = NoError, const QString& s = {}, Qx::Severity sv = Qx::Critical);

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class CUpdate : public Command
{
//-Class Structs------------------------------------------------------------------------------------------------------
private:
    struct ReleaseAsset
    {
        QString name;
        QString browser_download_url;

        QX_JSON_STRUCT(
            name,
            browser_download_url
        );
    };

    struct ReleaseData
    {
        QString name;
        QString tag_name;
        QList<ReleaseAsset> assets;

        QX_JSON_STRUCT(
            name,
            tag_name,
            assets
        );
    };

    struct FileTransfer
    {
        QString source;
        QString dest;
    };

    struct TransferSpecs
    {
        QDir updateRoot;
        QDir installRoot;
        QDir backupRoot;
        QString appName;
        QString binName;
    };

    struct UpdateTransfers
    {
        QList<FileTransfer> install;
        QList<FileTransfer> backup;
    };

//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS = u"Updating"_s;
    static inline const QString STATUS_CHECKING = u"Checking..."_s;
    static inline const QString STATUS_DOWNLOADING = u"Downloading..."_s;
    static inline const QString STATUS_INSTALLING = u"Installing..."_s;

    // Message
    static inline const QString MSG_NO_UPDATES = u"No updates available."_s;
    static inline const QString QUES_UPDATE = u"\"%1\" is available.\n\nUpdate?"_s;
    static inline const QString MSG_UPDATE_COMPLETE = u"Update installed successfully."_s;

    // Error
    static inline const QString WRN_NO_MATCHING_BUILD_P = u"A newer version is available, but without any assets that match current build specifications."_s;
    static inline const QString WRN_NO_MATCHING_BUILD_S = u"Update manually at GitHub."_s;

    // Log - Prepare
    static inline const QString LOG_EVENT_CHECKING_FOR_NEWER_VERSION = u"Checking if a newer release is available..."_s;
    static inline const QString LOG_EVENT_UPDATE_AVAILABLE = u"Update available (%1)."_s;
    static inline const QString LOG_EVENT_UPDATE_ACCEPED = u"Queuing update..."_s;
    static inline const QString LOG_EVENT_UPDATE_REJECTED = u"Update rejected"_s;

    // Log - Install
    static inline const QString LOG_EVENT_WAITING_ON_OLD_CLOSE = u"Waiting for bootstrap process to close (%1ms reamining)..."_s;
    static inline const QString LOG_EVENT_INSTALLING_UPDATE = u"Installing update..."_s;
    static inline const QString LOG_EVENT_FILE_TRANSFER = u"Transferring \"%1\" to \"%2\""_s;
    static inline const QString LOG_EVENT_BACKUP_FILES = u"Backing up original files..."_s;
    static inline const QString LOG_EVENT_RESTORE_FILES = u"Restoring original files..."_s;
    static inline const QString LOG_EVENT_INSTALL_FILES = u"Installing new files..."_s;

    // Command line option strings
    static inline const QString CL_OPT_INSTALL_L_NAME = u"install"_s;
    static inline const QString CL_OPT_INSTALL_DESC = u""_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_INSTALL{{CL_OPT_INSTALL_L_NAME}, CL_OPT_INSTALL_DESC, u"install"_s}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_INSTALL};

    // General
    static inline const QString RELEASE_ASSET_MAIN_TEMPLATE = u"CLIFp_%1_%2_%3_x64.%4.zip"_s;
    static inline const QString RELEASE_ASSET_LINUX_CMP_TEMPLATE = u"%1++-%2"_s;
    static inline const QString UPDATE_STAGE_NAME = u"CLIFp Updater"_s;

    // Cache
    static inline constinit bool smPersistCache = false;

public:
    // Meta
    static inline const QString NAME = u"update"_s;
    static inline const QString DESCRIPTION = u"Check for and optionally install updates."_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CUpdate(Core& coreRef, const QStringList& commandLine);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    // Path
    static QDir updateCacheDir();
    static QDir updateDownloadDir();
    static QDir updateDataDir();
    static QDir updateBackupDir();

    // Adjustment
    static QString sanitizeCompiler(QString cmp);
    static QString substitutePathNames(const QString& path, QStringView binName, QStringView appName);

    // Work
    static Qx::IoOpReport determineNewFiles(QStringList& files, const QDir& sourceRoot);
    static UpdateTransfers determineTransfers(const QStringList& files, const TransferSpecs& specs);

public:
    static bool isUpdateCacheClearable();
    static CUpdateError clearUpdateCache();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    CUpdateError getLatestReleaseData(ReleaseData& data) const;
    QString getTargetAssetName(const QString& tagName) const;
    CUpdateError handleTransfers(const UpdateTransfers& transfers) const;
    CUpdateError checkAndPrepareUpdate() const;
    Qx::Error installUpdate(const QFileInfo& existingAppInfo) const;

protected:
    QList<const QCommandLineOption*> options() const override;
    QString name() const override;
    Qx::Error perform() override;

public:
    bool requiresFlashpoint() const override;
    bool autoBlockNewInstances() const override;
};

#endif // CUPDATE_H
