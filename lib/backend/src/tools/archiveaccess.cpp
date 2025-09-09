// Unit Include
#include "archiveaccess.h"

// Qx Includes
#include <qx/io/qx-common-io.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "kernel/core.h"
#include "utility.h"

//===============================================================================================================
// MounterError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
ArchiveAccessError::ArchiveAccessError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool ArchiveAccessError::isValid() const { return mType != NoError; }
QString ArchiveAccessError::specific() const { return mSpecific; }
ArchiveAccessError::Type ArchiveAccessError::type() const { return mType; }

//Private:
Qx::Severity ArchiveAccessError::deriveSeverity() const { return Qx::Warning; }
quint32 ArchiveAccessError::deriveValue() const { return mType; }
QString ArchiveAccessError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString ArchiveAccessError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// ArchiveAccess
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
ArchiveAccess::ArchiveAccess(Core& core, Type type) :
    Directorate(core.director()),
    mType(type),
    mArchivesDir(core.fpInstall().archiveDataDirectory())
{
    logEvent(MSG_INIT.arg(ENUM_NAME(mType)));

    // Initialize parts
    QStringList partPaths;
    if(auto archDirOp = Qx::dirContentList(partPaths, mArchivesDir, { ZIP_FILTER_TEMPLATE.arg(ENUM_NAME(type)) }); archDirOp.isFailure())
    {
        logError(archDirOp);
        return;
    }

    if(partPaths.isEmpty())
    {
        logError(ArchiveAccessError(ArchiveAccessError::NoParts, ENUM_NAME(type)));
        return;
    }
    logEvent(MSG_PART_COUNT.arg(mParts.size()));

    for(const auto& pp : std::as_const(partPaths))
        mParts.emplace_back(pp);
}

//-Instance Functions-------------------------------------------------------------
//Private:
bool ArchiveAccess::readyPart(QuaZip& part)
{
    if(part.isOpen())
    {
        logEvent(MSG_PART_OPEN);
        return true;
    }

    logEvent(MSG_PREPARING_PART.arg(part.getZipName()));
    if(!part.open(QuaZip::mdUnzip))
    {
        logError(ArchiveAccessError(ArchiveAccessError::NoParts, part.getZipName()));
        return false;
    }

    return true;
}

//Public:
QString ArchiveAccess::name() const { return NAME; }

QByteArray ArchiveAccess::getFileContents(const QString& inZipPath)
{
    /* Maybe return an ArchiveAccessError, for cases where something unexpected happened
     * (can't open zip, or file not found), and treat those cases as an error instead
     * of a warning.
     */
    logEvent(MSG_FILE_SEARCH.arg(ENUM_NAME(mType), inZipPath));

    // Simply search all parts (what the stock launcher does, though I'm sure it could be more optimal)
    for(auto& part : mParts)
    {
        if(!readyPart(part))
            continue;

        if(part.setCurrentFile(inZipPath))
        {
            QuaZipFile file(&part); // auto-closes on destruct
            if(!file.open(QIODevice::ReadOnly))
            {
                logError(ArchiveAccessError(ArchiveAccessError::CantOpenFile, part.getZipName()));
                break;
            }

            return file.readAll();
        }
    }

    logError(ArchiveAccessError(ArchiveAccessError::FileNotFound));
    return {};
}
