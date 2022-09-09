#ifndef TEXTRACT_H
#define TEXTRACT_H

// Project Includes
#include "task/task.h"

// Qt Includes
#include <QDir>

class TExtract : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TExtract");

    // Logging
    static inline const QString LOG_EVENT_EXTRACTING_DATA_PACK = "Extracting Data Pack %1";

    // Extract
    static inline const QString ERR_PACK_EXTRACT = "Could not extract data pack %1.";
    static inline const QString ERR_PACK_EXTRACT_OPEN = "Failed to open the archive.";
    static inline const QString ERR_PACK_EXTRACT_MAKE_PATH = "Failed to create file path.";
    static inline const QString ERR_PACK_EXTRACT_OPEN_ARCH_FILE = "Failed to open archive file (code 0x%1).";
    static inline const QString ERR_PACK_EXTRACT_OPEN_DISK_FILE = "Failed to open disk file \"%1\".";
    static inline const QString ERR_PACK_EXTRACT_WRITE_DISK_FILE = "Failed to write disk file \"%1\".";
    static inline const QString ERR_PACK_EXTRACT_GENERAL_ZIP = "General zip error (code 0x%1).";

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QString mPackPath;
    QString mPathInPack; // NOTE: '/' for root, only supports directories currently.
    QString mDestinationPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExtract();

//-Class Functions---------------------------------------------------------------------------------------------------------------
private:
    static Qx::GenericError extractZipSubFolderContentToDir(QString zipFilePath, QString subFolder, QDir dir);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    QString packPath() const;
    QString pathInPack() const;
    QString destinationPath() const;

    void setPackPath(QString path);
    void setPathInPack(QString path);
    void setDestinationPath(QString path);

    void perform() override;
};

#endif // TEXTRACT_H
