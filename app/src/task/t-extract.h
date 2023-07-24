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
        {NoError, u""_s},
        {OpenArchive, u"Failed to open archive."_s},
        {MakePath, u"Failed to create file path."_s},
        {OpenArchiveFile, u"Failed to open archive file."_s},
        {OpenDiskFile, u"Failed to open disk file."_s},
        {WriteDiskFile, u"Failed to write disk file."_s},
        {GeneralZip, u"General zip error."_s}
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
    static inline const QString NAME = u"TExtract"_s;

    // Logging
    static inline const QString LOG_EVENT_EXTRACTING_DATA_PACK = u"Extracting Data Pack %1"_s;

    // Error
    static inline const QString ERR_CODE_TEMPLATE = u"Code: 0x%1"_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QString mPackPath;
    QString mPathInPack; // NOTE: '/' for root, only supports directories currently.
    QString mDestinationPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExtract(QObject* parent);

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
