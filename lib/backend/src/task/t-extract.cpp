// Unit Include
#include "t-extract.h"

// QuaZip Includes
#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>

// TODO: Should probably be treated as a long task and support stopping

//===============================================================================================================
// TExtractError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TExtractError::TExtractError() :
    mType(NoError)
{}

TExtractError::TExtractError(const QString& archName, Type t, const QString& s) :
    mType(t),
    mSpecific(s),
    mArchName(archName)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TExtractError::isValid() const { return mType != NoError; }
QString TExtractError::specific() const { return mSpecific; }
TExtractError::Type TExtractError::type() const { return mType; }
QString TExtractError::archName() const { return mArchName; }

//Private:
Qx::Severity TExtractError::deriveSeverity() const { return Qx::Critical; }
quint32 TExtractError::deriveValue() const { return mType; }
QString TExtractError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TExtractError::deriveSecondary() const { return mArchName; }
QString TExtractError::deriveDetails() const { return mSpecific; }

class TExtract::Extractor
{
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    static inline const QString ERR_CODE_TEMPLATE = u"Code: 0x%1"_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QuaZip mZip;
    QuaZipFile mCurrentZipFile;
    QuaZipDir mCurrentZipDir; // NOTE: empty string for root, only supports directories currently.
    QFile mCurrentDiskFile;
    QDir mCurrentDiskDir;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    Extractor(const QString& zipPath, const QString& zipDirPath, const QDir& destinationDir) :
        mZip(zipPath),
        mCurrentZipFile(&mZip),
        mCurrentZipDir(&mZip, sanitizeZipDirPath(zipDirPath)),
        mCurrentDiskDir(destinationDir)
    {}

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    ~Extractor() { mZip.close(); }

//-Class Functions---------------------------------------------------------------------------------------------------------------
private:
    static QString sanitizeZipDirPath(const QString& zdp)
    {
        if(zdp.isEmpty() || zdp == '/')
            return u""_s;

        QString clean = zdp;
        if(clean.front() == '/')
            clean = clean.sliced(1);
        if(clean.back() == '/')
            clean.chop(1);

        return clean;
    }

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    QString zipErrorString() const { return ERR_CODE_TEMPLATE.arg(mZip.getZipError(), 2, 16, QChar('0')); }
    TExtractError makeError(TExtractError::Type t, const QString& s = {}) const { return TExtractError(mZip.getZipName(), t, s); }
    TExtractError makeZipError(TExtractError::Type t) const { return TExtractError(mZip.getZipName(), t, zipErrorString()); }

    bool diskCdDown(const QString& subFolder)
    {
        if(!mCurrentDiskDir.exists(subFolder) && !mCurrentDiskDir.mkdir(subFolder))
            return false;

        return mCurrentDiskDir.cd(subFolder);
    }

    TExtractError processDir(const QString& name = {})
    {
        // Move to dir, unless processing root
        if(!name.isEmpty())
        {
            if(!mCurrentZipDir.cd(name))
                return makeError(TExtractError::PathError);
            if(!diskCdDown(name))
                return makeError(TExtractError::MakePath);
        }

        // Process files
        const QStringList files = mCurrentZipDir.entryList(QDir::Files);
        for(const QString& f : files)
            processFile(f);

        // Process sub-folders
        const QStringList subFolders = mCurrentZipDir.entryList(QDir::Dirs);
        for(const QString& sf : subFolders)
            processDir(sf);

        // Return to parent dir, unless root
        if(!name.isEmpty())
        {
            if(!mCurrentZipDir.cdUp() || !mCurrentDiskDir.cdUp())
                return makeError(TExtractError::PathError);
        }

        return TExtractError();
    }

    TExtractError processFile(const QString name)
    {
        // Change to archive file
        if(!mZip.setCurrentFile(mCurrentZipDir.filePath(name)))
            return makeError(TExtractError::PathError);

        // Open archive file and read data
        if(!mCurrentZipFile.open(QIODevice::ReadOnly))
            return makeZipError(TExtractError::OpenArchiveFile);

        QByteArray fileData = mCurrentZipFile.readAll();
        mCurrentZipFile.close();

        // Change to disk file
        mCurrentDiskFile.setFileName(mCurrentDiskDir.absoluteFilePath(name));

        // Open disk file and write data
        if(!mCurrentDiskFile.open(QIODevice::WriteOnly))
            return makeError(TExtractError::OpenDiskFile, mCurrentDiskFile.errorString());

        if(mCurrentDiskFile.write(fileData) != fileData.size())
            return makeError(TExtractError::WriteDiskFile, mCurrentDiskFile.fileName());

        mCurrentDiskFile.close();

        return TExtractError();
    }

public:
    TExtractError extract()
    {
        // Open archive
        if(!mZip.open(QuaZip::mdUnzip))
            return makeZipError(TExtractError::OpenArchive);

        // Ensure zip sub-folder is valid
        if(!mCurrentZipDir.exists())
            return makeError(TExtractError::InvalidSubPath);

        // Ensure root destination folder is present
        if(!mCurrentDiskDir.mkpath(u"."_s))
            return makeError(TExtractError::MakePath);

        // Recurse
        TExtractError err = processDir();

         // Check for general zip error
        if(!err.isValid() && mZip.getZipError() != UNZ_OK)
            err = makeZipError(TExtractError::GeneralZip);

        return err;
    }
};

//===============================================================================================================
// TExtract
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TExtract::TExtract(Core& core) :
    Task(core)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TExtract::name() const { return NAME; }
QStringList TExtract::members() const
{
    QStringList ml = Task::members();
    ml.append(u".packPath() = \""_s + mPackPath + u"\""_s);
    ml.append(u".pathInPack() = \""_s + mPathInPack + u"\""_s);
    ml.append(u".destinationPath() = \""_s + mDestinationPath + u"\""_s);
    return ml;
}

QString TExtract::packPath() const { return mPackPath; }
QString TExtract::pathInPack() const { return mPathInPack; }
QString TExtract::destinationPath() const { return mDestinationPath; }

void TExtract::setPackPath(QString path) { mPackPath = path; }
void TExtract::setPathInPack(QString path) { mPathInPack = path; }
void TExtract::setDestinationPath(QString path) { mDestinationPath = path; }

void TExtract::perform()
{
    // Log string
    QFileInfo packFileInfo(mPackPath);
    logEvent(LOG_EVENT_EXTRACTING_ARCHIVE.arg(packFileInfo.fileName()));

    // Extract pack
    Extractor extractor(mPackPath, mPathInPack, mDestinationPath);
    TExtractError ee = extractor.extract();
    if(ee.isValid())
        postDirective<DError>(ee);

    emit complete(ee);
}
