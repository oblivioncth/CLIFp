// Unit Include
#include "t-extract.h"

// QuaZip Includes
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

// TODO: Should probably be treated as a long task and support stopping

//===============================================================================================================
// TExtract
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TExtract::TExtract() {}

//-Class Functions---------------------------------------------------------------------------------------------------------------
//Private:
Qx::GenericError TExtract::extractZipSubFolderContentToDir(QString zipFilePath, QString subFolder, QDir dir)
{
    // Error template
    QFileInfo zipInfo(zipFilePath);
    Qx::GenericError error(Qx::GenericError::Critical, ERR_PACK_EXTRACT.arg(zipInfo.fileName()));

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
        return error.setSecondaryInfo(ERR_PACK_EXTRACT_OPEN);
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
                if(!currentDirOnDisk.mkpath("."))
                    return error.setSecondaryInfo(ERR_PACK_EXTRACT_MAKE_PATH);
            }

            // Open file in archive and read its data
            if(!currentArchiveFile.open(QIODevice::ReadOnly))
            {
                int zipError = zipFile.getZipError();
                return error.setSecondaryInfo(ERR_PACK_EXTRACT_OPEN_ARCH_FILE.arg(zipError, 2, 16, QChar('0')));
            }

            QByteArray fileData = currentArchiveFile.readAll();
            currentArchiveFile.close();

            // Open disk file and write data to it
            QFile fileOnDisk(pathOnDisk.absoluteFilePath());
            if(!fileOnDisk.open(QIODevice::WriteOnly))
                return error.setSecondaryInfo(ERR_PACK_EXTRACT_OPEN_DISK_FILE.arg(fileOnDisk.fileName()));

            if(fileOnDisk.write(fileData) != fileData.size())
                return error.setSecondaryInfo(ERR_PACK_EXTRACT_WRITE_DISK_FILE.arg(fileOnDisk.fileName()));

            fileOnDisk.close();
        }
    }

    // Check if processing ended due to an error
    int zipError = zipFile.getZipError();
    if(zipError != UNZ_OK)
        return error.setSecondaryInfo(ERR_PACK_EXTRACT_GENERAL_ZIP.arg(zipError, 2, 16, QChar('0')));

    // Return success
    return Qx::GenericError();
}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TExtract::name() const { return NAME; }
QStringList TExtract::members() const
{
    QStringList ml = Task::members();
    ml.append(".packPath() = \"" + mPackPath + "\"");
    ml.append(".pathInPack() = \"" + mPathInPack + "\"");
    ml.append(".destinationPath() = \"" + mDestinationPath + "\"");
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
    // Error tracking
    ErrorCode errorStatus = ErrorCode::NO_ERR;

    // Log string
    QFileInfo packFileInfo(mPackPath);
    emit eventOccurred(NAME, LOG_EVENT_EXTRACTING_DATA_PACK.arg(packFileInfo.fileName()));

    // Extract pack
    Qx::GenericError extractError = extractZipSubFolderContentToDir(mPackPath,
                                                                    mPathInPack,
                                                                    mDestinationPath);

    if(extractError.isValid())
    {
        emit errorOccurred(NAME, extractError);
        errorStatus = ErrorCode::PACK_EXTRACT_FAIL;
    }

    emit complete(errorStatus);
}
