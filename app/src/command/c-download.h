#ifndef CDOWNLOAD_H
#define CDOWNLOAD_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/command.h"

class QX_ERROR_TYPE(CDownloadError, "CDownloadError", 1217)
{
    friend class CDownload;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        InvalidPlaylist
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {InvalidPlaylist, u""_s}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    CDownloadError(Type t = NoError, const QString& s = {});

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

class CDownload : public Command
{
    //-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_DOWNLOAD = u"Downloading data packs"_s;

    // Logging
    static inline const QString LOG_EVENT_PLAYLIST_MATCH = u"Playlist matches ID: %1"_s;
    static inline const QString LOG_EVENT_NON_DATAPACK = u"Game %1 does not use a data pack."_s;
    static inline const QString LOG_EVENT_NO_OP = u"No datapacks to download."_s;

    // Command line option strings
    static inline const QString CL_OPT_PLAYLIST_S_NAME = u"p"_s;
    static inline const QString CL_OPT_PLAYLIST_L_NAME = u"playlist"_s;
    static inline const QString CL_OPT_PLAYLIST_DESC = u"Name of the playlist to download games for."_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_PLAYLIST{{CL_OPT_PLAYLIST_S_NAME, CL_OPT_PLAYLIST_L_NAME}, CL_OPT_PLAYLIST_DESC, u"playlist"_s}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_PLAYLIST};
    static inline const QSet<const QCommandLineOption*> CL_OPTIONS_REQUIRED{&CL_OPTION_PLAYLIST};

public:
    // Meta
    static inline const QString NAME = u"download"_s;
    static inline const QString DESCRIPTION = u"Download game data packs in bulk"_s;

    //-Constructor----------------------------------------------------------------------------------------------------------
public:
    CDownload(Core& coreRef);

    //-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    QList<const QCommandLineOption*> options() override;
    QSet<const QCommandLineOption*> requiredOptions() override;
    QString name() override;
    Qx::Error perform() override;
};
REGISTER_COMMAND(CDownload::NAME, CDownload, CDownload::DESCRIPTION);

#endif // CDOWNLOAD_H
