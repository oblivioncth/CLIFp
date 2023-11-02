#ifndef CUPDATE_H
#define CUPDATE_H

// Qx Includes
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
        ConnectionError,
        InvalidUpdateData,
        InvalidReleaseVersion,
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {ConnectionError, u"Failed to query the update server."_s},
        {InvalidUpdateData, u"The update server responded with unrecognized data."_s},
        {InvalidReleaseVersion, u"The latest release has an invalid version."_s},
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    CUpdateError(Type t = NoError, const QString& s = {});

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
    };

    struct ReleaseData
    {
        QString name;
        QString tag_name;
        QList<ReleaseAsset> assets;
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
    static inline const QString QUES_UPDATE = uR"("%1" is available.\n\nUpdate?)"_s;

    // Error
    static inline const QString WRN_NO_MATCHING_BUILD_P = u"A newer version is available, but without any assets that match current build specifications."_s;
    static inline const QString WRN_NO_MATCHING_BUILD_S = u"Update manually at GitHub."_s;

    // Log
    static inline const QString LOG_EVENT_CHECKING_FOR_NEWER_VERSION = u"Checking if a newer release is available..."_s;
    static inline const QString LOG_EVENT_UPDATE_AVAILABLE = u"Update available (%1)."_s;
    static inline const QString LOG_EVENT_UPDATE_ACCEPED = u"Queuing update..."_s;
    static inline const QString LOG_EVENT_UPDATE_REJECTED = u"Update rejected"_s;

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

public:
    // Meta
    static inline const QString NAME = u"update"_s;
    static inline const QString DESCRIPTION = u"Check for and optionally install updates."_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CUpdate(Core& coreRef);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    static QDir updateCacheDir();
    static QDir updateDownloadDir();
    static QDir updateDataDir();
    static QDir updateBackupDir();
    static QString sanitizeCompiler(QString cmp);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    CUpdateError getLatestReleaseData(ReleaseData& data) const;
    QString getTargetAssetName(const QString& tagName) const;
    CUpdateError checkAndPrepareUpdate() const;
    CUpdateError installUpdate(const QString& appName) const;

protected:
    QList<const QCommandLineOption*> options() override;
    QString name() override;
    Qx::Error perform() override;

public:
    bool requiresFlashpoint() const override;
};
REGISTER_COMMAND(CUpdate::NAME, CUpdate, CUpdate::DESCRIPTION);

#endif // CUPDATE_H
