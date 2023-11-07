#ifndef TEXTRACT_H
#define TEXTRACT_H

// Qt Includes
#include <QDir>

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

class Extractor;

class QX_ERROR_TYPE(TExtractError, "TExtractError", 1255)
{
    friend class TExtract;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        InvalidSubPath,
        OpenArchive,
        MakePath,
        PathError,
        OpenArchiveFile,
        OpenDiskFile,
        WriteDiskFile,
        GeneralZip
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {InvalidSubPath, u"Invalid path within zip."_s},
        {OpenArchive, u"Failed to open archive."_s},
        {MakePath, u"Failed to create file path."_s},
        {PathError, u"Unexpected deviation in path availability."_s},
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
    class Extractor;

    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TExtract"_s;

    // Logging
    static inline const QString LOG_EVENT_EXTRACTING_ARCHIVE = u"Extracting archive %1"_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QString mPackPath;
    QString mPathInPack; // NOTE: empty string for root, only supports directories currently.
    QString mDestinationPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExtract(QObject* parent);

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
