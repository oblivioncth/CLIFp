#ifndef TDOWNLOAD_H
#define TDOWNLOAD_H

// Qx Includes
#include <qx/network/qx-downloadmanager.h>
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

// FP Forward Declarations
namespace Fp
{
    class Toolkit;
    class GameData;
}

class QX_ERROR_TYPE(TDownloadError, "TDownloadError", 1252)
{
    friend class TDownload;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        Incomeplete,
        OfflineEdition
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {Incomeplete, u"The download(s) could not be completed."_s},
        {OfflineEdition, u"A datapack download was prompted in an offline edition of Flashpoint."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    TDownloadError(Type t = NoError, const QString& s = {});

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

class TDownload : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TDownload"_s;

    // Logging
    static inline const QString LOG_EVENT_DOWNLOAD = u"Downloading %1"_s;
    static inline const QString LOG_EVENT_DOWNLOAD_SUCC = u"File(s) downloaded successfully"_s;
    static inline const QString LOG_EVENT_DOWNLOAD_AUTH = u"File download unexpectedly requires authentication (%1)"_s;
    static inline const QString LOG_EVENT_STOPPING_DOWNLOADS = u"Stopping current download(s)..."_s;

    // Members
    static inline const QString FILE_NO_CHECKSUM = u"NO SUM"_s;
    static inline const QString FILE_DOWNLOAD_TEMPLATE = uR"("%1" -> "%2" (%3))"_s;
    static inline const QString FILE_DOWNLOAD_ELIDE = u"+%1 more..."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    Qx::AsyncDownloadManager mDownloadManager;

    // Data
    QList<Qx::DownloadTask> mFiles;
    QString mDescription;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TDownload(QObject* parent);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    bool isEmpty() const;
    qsizetype fileCount() const;
    QList<Qx::DownloadTask> files() const;
    QString description() const;

    void addFile(const Qx::DownloadTask file);
    TDownloadError addDatapack(const Fp::Toolkit* tk, const Fp::GameData* gameData);
    void setDescription(const QString& desc);

    void perform() override;
    void stop() override;

//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postDownload(Qx::DownloadManagerReport downloadReport);
};

#endif // TDOWNLOAD_H
