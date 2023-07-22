#ifndef TDOWNLOAD_H
#define TDOWNLOAD_H

// Qx Includes
#include <qx/network/qx-downloadmanager.h>
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

class QX_ERROR_TYPE(TDownloadError, "TDownloadError", 1252)
{
    friend class TDownload;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        ChecksumMismatch = 1,
        Incomeplete = 2
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {ChecksumMismatch, u"The title's Data Pack checksum does not match its record!"_s},
        {Incomeplete, u"The download could not be completed."_s}
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
    static inline const QString LOG_EVENT_DOWNLOADING_DATA_PACK = u"Downloading Data Pack %1"_s;
    static inline const QString LOG_EVENT_DOWNLOAD_SUCC = u"Data Pack downloaded successfully"_s;
    static inline const QString LOG_EVENT_DOWNLOAD_AUTH = u"Data Pack download unexpectedly requires authentication (%1)"_s;
    static inline const QString LOG_EVENT_STOPPING_DOWNLOADS = u"Stopping current download(s)..."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    Qx::AsyncDownloadManager mDownloadManager;
    /* NOTE: If it ever becomes required to perform multiple downloads in a run the DM instance should
     * be made a static member of TDownload, or all downloads need to be determined at the same time
     * and the task made capable of holding all of them
     */

    // Data
    QString mDestinationPath;
    QString mDestinationFilename;
    QUrl mTargetFile;
    QString mSha256;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TDownload(QObject* parent);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    QString destinationPath() const;
    QString destinationFilename() const;
    QUrl targetFile() const;
    QString sha256() const;

    void setDestinationPath(QString path);
    void setDestinationFilename(QString filename);
    void setTargetFile(QUrl targetFile);
    void setSha256(QString sha256);

    void perform() override;
    void stop() override;

//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postDownload(Qx::DownloadManagerReport downloadReport);
};

#endif // TDOWNLOAD_H
