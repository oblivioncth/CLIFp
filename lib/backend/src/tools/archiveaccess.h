#ifndef ARCHIVEACCESS_H
#define ARCHIVEACCESS_H

// Standard Library Includes
#include <list>

// Qx Includes
#include <qx/core/qx-error.h>

// QuaZip Includes
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

// Project Includes
#include "kernel/directorate.h"

/* TODO: Arguably this should be part of libfp, though that would require
 * making Quazip/zlib a dependency for it as well. I'd like to have only
 * one or the other require zip libs so if we want to move this to libfp,
 * then we also need to move the extract function, which is a little tricky
 * since right now it's async as a Task. We'd need to make it async in libfp
 * I guess and just have TExtract forward the call and signal connections.
 */

class Core;

class QX_ERROR_TYPE(ArchiveAccessError, "ArchiveAccessError", 1235)
{
    friend class ArchiveAccess;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        NoParts,
        CantOpenPart,
        FileNotFound,
        CantOpenFile
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {NoParts, u"There were no zip parts for the given archive type."_s},
        {CantOpenPart, u"Failed to open part."_s},
        {FileNotFound, u"No parts contained the target file."_s},
        {CantOpenFile, u"Failed to open inner file."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mProcessName;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    ArchiveAccessError(Type t = NoError, const QString& specific = {});

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    QString specific() const;
    Type type() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class ArchiveAccess : public Directorate
{
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"ArchiveAccess"_s;

    // Filter
    static inline const QString ZIP_FILTER_TEMPLATE = u"%1_*.zip"_s;

    // Messages
    static inline const QString MSG_INIT = u"Initializing access to data archive for %1..."_s;
    static inline const QString MSG_PART_COUNT = u"Archive consists of %1 parts."_s;
    static inline const QString MSG_PREPARING_PART = u"Preparing part %1."_s;
    static inline const QString MSG_PART_OPEN = u"Part %1 is already open."_s;
    static inline const QString MSG_FILE_SEARCH = u"Searching %1 parts for %2."_s;

//-Class Enums------------------------------------------------------------------------------------------------
public:
    enum Type { GameData };

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    Type mType;
    QDir mArchivesDir;
    std::list<QuaZip> mParts; // QuaZip cannot be moved, so we cannot use QList/vector :/

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    ArchiveAccess(Core& core, Type type);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    bool readyPart(QuaZip& part);

public:
    QString name() const override;
    QByteArray getFileContents(const QString& inZipPath); // Don't use leading slashes.

};

#endif // ARCHIVEACCESS_H
