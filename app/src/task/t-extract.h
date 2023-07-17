#ifndef TEXTRACT_H
#define TEXTRACT_H

// Qt Includes
#include <QDir>

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

class QX_ERROR_TYPE(TExtractError, "TExtractError", 1255)
{
    friend class TExtract;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        OpenArchive = 1,
        MakePath = 2,
        OpenArchiveFile = 3,
        OpenDiskFile = 4,
        WriteDiskFile = 5,
        GeneralZip = 6
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {OpenArchive, QSL("Failed to open archive.")},
        {MakePath, QSL("Failed to create file path.")},
        {OpenArchiveFile, QSL("Failed to open archive file.")},
        {OpenDiskFile, QSL("Failed to open disk file.")},
        {WriteDiskFile, QSL("Failed to write disk file.")},
        {GeneralZip, QSL("General zip error.")}
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;
    QString mArchName;

    //-Constructor-------------------------------------------------------------
private:
    TExtractError();
    TExtractError(const QString& archName, Type t, const QString& s = {});

    //-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;
    QString archName() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
    QString deriveDetails() const override;
};

class TExtract : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QSL("TExtract");

    // Logging
    static inline const QString LOG_EVENT_EXTRACTING_DATA_PACK = QSL("Extracting Data Pack %1");

    // Error
    static inline const QString ERR_CODE_TEMPLATE = QSL("Code: 0x%1");

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QString mPackPath;
    QString mPathInPack; // NOTE: '/' for root, only supports directories currently.
    QString mDestinationPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExtract(QObject* parent = nullptr);

//-Class Functions---------------------------------------------------------------------------------------------------------------
private:
    static TExtractError extractZipSubFolderContentToDir(QString zipFilePath, QString subFolder, QDir dir);

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
