// Unit Include
#include "t-extract.h"

// QuaZip Includes
#include <quazip/quazip.h>
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

//===============================================================================================================
// TExtract
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TExtract::TExtract(QObject* parent) :
    Task(parent)
{}

//-Class Functions---------------------------------------------------------------------------------------------------------------
//Private:
TExtractError TExtract::extractZipSubFolderContentToDir(QString zipFilePath, QString subFolder, QDir dir)
{
    // Error template
    QString zipName = QFileInfo(zipFilePath).fileName();

    // Zip file
    QuaZip zipFile(zipFilePath);

    // Form subfolder string to match zip content scheme
    if(subFolder.isEmpty())
        subFolder = '/';

    if(subFolder != '/')
    {
        // Remove leading '/' if present
        if(subFolder.front() == '/')
            subFolder = subFolder.mid(1);

        // Add trailing '/' if missing
        if(subFolder.back() != '/')
            subFolder.append('/');
    }

    // Open archive, ensure it's closed when done
    if(!zipFile.open(QuaZip::mdUnzip))
        return TExtractError(zipName, TExtractError::OpenArchive);
    QScopeGuard closeGuard([&](){ zipFile.close(); });

    // Persistent data
    QuaZipFile currentArchiveFile(&zipFile);
    QDir currentDirOnDisk(dir);

    // Extract all files in sub-folder
    for(bool atEnd = !zipFile.goToFirstFile(); !atEnd; atEnd = !zipFile.goToNextFile())
    {
        QString fileName = zipFile.getCurrentFileName();

        // Only consider files in specified sub-folder
        if(fileName.startsWith(subFolder))
        {
            // Determine path on disk
            QString pathWithinFolder = fileName.mid(subFolder.size());
            QFileInfo pathOnDisk(dir.absoluteFilePath(pathWithinFolder));

            // Update current directory and make path if necessary
            if(pathOnDisk.absolutePath() != currentDirOnDisk.absolutePath())
            {
                currentDirOnDisk = pathOnDisk.absoluteDir();
                if(!currentDirOnDisk.mkpath(u"."_s))
                    return TExtractError(zipName, TExtractError::MakePath);
            }

            // Open file in archive and read its data
            if(!currentArchiveFile.open(QIODevice::ReadOnly))
            {
                int zipError = zipFile.getZipError();
                return TExtractError(zipName, TExtractError::OpenArchiveFile, ERR_CODE_TEMPLATE.arg(zipError, 2, 16, QChar('0')));
            }

            QByteArray fileData = currentArchiveFile.readAll();
            currentArchiveFile.close();

            // Open disk file and write data to it
            QFile fileOnDisk(pathOnDisk.absoluteFilePath());
            if(!fileOnDisk.open(QIODevice::WriteOnly))
                return TExtractError(zipName, TExtractError::OpenDiskFile, fileOnDisk.fileName());

            if(fileOnDisk.write(fileData) != fileData.size())
                return TExtractError(zipName, TExtractError::WriteDiskFile, fileOnDisk.fileName());

            fileOnDisk.close();
        }
    }

    // Check if processing ended due to an error
    int zipError = zipFile.getZipError();
    if(zipError != UNZ_OK)
        return TExtractError(zipName, TExtractError::GeneralZip, ERR_CODE_TEMPLATE.arg(zipError, 2, 16, QChar('0')));

    // Return success
    return TExtractError();
}

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
    emit eventOccurred(NAME, LOG_EVENT_EXTRACTING_ARCHIVE.arg(packFileInfo.fileName()));

    // Extract pack
    TExtractError ee = extractZipSubFolderContentToDir(mPackPath,
                                                       mPathInPack,
                                                       mDestinationPath);

    if(ee.isValid())
        emit errorOccurred(NAME, ee);

    emit complete(ee);
}
