#ifndef TDOWNLOAD_H
#define TDOWNLOAD_H

// Qx Includes
#include <qx/network/qx-downloadmanager.h>

// Project Includes
#include "task/task.h"

class TDownload : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TDownload");

    // Logging
    static inline const QString LOG_EVENT_DOWNLOADING_DATA_PACK = "Downloading Data Pack %1";
    static inline const QString LOG_EVENT_DOWNLOAD_SUCC = "Data Pack downloaded successfully";
    static inline const QString LOG_EVENT_DOWNLOAD_AUTH = "Data Pack download unexpectedly requires authentication (%1)";
    static inline const QString LOG_EVENT_STOPPING_DOWNLOADS = "Stopping current download(s)...";

    // Errors
    static inline const QString ERR_PACK_SUM_MISMATCH = "The title's Data Pack checksum does not match its record!";

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
    TDownload(QObject* parent = nullptr);

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
