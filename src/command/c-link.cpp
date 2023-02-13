// Unit Include
#include "c-link.h"

// Project Includes

//===============================================================================================================
// CSHORTCUT
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CLink::CLink(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
const QList<const QCommandLineOption*> CLink::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
const QString CLink::name() { return NAME; }

//Public:
ErrorCode CLink::process(const QStringList& commandLine)
{
    ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return ErrorCode::NO_ERR;

    // Shortcut parameters
    QUuid shortcutId;
    QDir shortcutDir;
    QString shortcutName;

    // Get shortcut ID
    if(mParser.isSet(CL_OPTION_ID))
    {
        if((shortcutId = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_INVALID));
            return ErrorCode::ID_NOT_VALID;
        }
    }
    else if(mParser.isSet(CL_OPTION_TITLE))
    {
        if((errorStatus = mCore.getGameIdFromTitle(shortcutId, mParser.value(CL_OPTION_TITLE))))
            return errorStatus;
    }
    else
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_TITLE));
        return ErrorCode::INVALID_ARGS;
    }

    mCore.setStatus(STATUS_LINK, shortcutId.toString(QUuid::WithoutBraces));

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Get entry info (also confirms that ID is present in database)
    Fp::Db::EntryFilter entryFilter{.type = Fp::Db::EntryType::PrimaryThenAddApp, .id = shortcutId};

    QSqlError sqlError;
    Fp::Db::QueryBuffer entryInfo;

    if((sqlError = database->queryEntrys(entryInfo, entryFilter)).isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, sqlError.text()));
        return ErrorCode::SQL_ERROR;
    }
    else if(entryInfo.size < 1)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND, sqlError.text()));
        return ErrorCode::ID_NOT_FOUND;
    }

    // Get entry title
    entryInfo.result.next();

    if(entryInfo.source == Fp::Db::Table_Game::NAME)
        shortcutName = Qx::kosherizeFileName(entryInfo.result.value(Fp::Db::Table_Game::COL_TITLE).toString());
    else if(entryInfo.source == Fp::Db::Table_Add_App::NAME)
    {
        // Get parent info
        QUuid parentId = QUuid(entryInfo.result.value(Fp::Db::Table_Add_App::COL_PARENT_ID).toString());
        Fp::Db::EntryFilter parentFilter{.type = Fp::Db::EntryType::Primary, .id = parentId};

        Fp::Db::QueryBuffer parentInfo;
        if((sqlError = database->queryEntrys(parentInfo, parentFilter)).isValid())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, sqlError.text()));
            return ErrorCode::SQL_ERROR;
        }
        parentInfo.result.next();

        QString parentName = parentInfo.result.value(Fp::Db::Table_Game::COL_TITLE).toString();
        shortcutName = Qx::kosherizeFileName(parentName + " (" + entryInfo.result.value(Fp::Db::Table_Add_App::COL_NAME).toString() + ")");
    }
    else
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_SQL_MISMATCH,
                                               ERR_DIFFERENT_TITLE_SRC.arg(Fp::Db::Table_Game::NAME + "/" + Fp::Db::Table_Add_App::NAME, entryInfo.source)));
        return ErrorCode::SQL_MISMATCH;
    }

    // Get shortcut path
    if(mParser.isSet(CL_OPTION_PATH))
    {
        QFileInfo inputPathInfo(mParser.value(CL_OPTION_PATH));
        if(inputPathInfo.suffix() == shortcutExtension()) // Path is file
        {
            mCore.logEvent(NAME, LOG_EVENT_FILE_PATH);
            shortcutDir = inputPathInfo.absoluteDir();
            shortcutName = inputPathInfo.baseName();
        }
        else // Path is directory
        {
            mCore.logEvent(NAME, LOG_EVENT_DIR_PATH);
            shortcutDir = QDir(inputPathInfo.absoluteFilePath());
        }
    }
    else
    {
        mCore.logEvent(NAME, LOG_EVENT_NO_PATH);

        // Prompt user for path
        Core::SaveFileRequest sfr{
            .caption = DIAG_CAPTION,
            .dir = QDir::homePath() + "/Desktop/" + shortcutName,
            .filter = "Shortcuts (*. " + shortcutExtension() + ")"
        };
        QString selectedPath = mCore.requestSaveFilePath(sfr);

        if(selectedPath.isEmpty())
        {
            mCore.logEvent(NAME, LOG_EVENT_DIAG_CANCEL);
            return ErrorCode::NO_ERR;
        }
        else
        {
            if(!selectedPath.endsWith("." + shortcutExtension(), Qt::CaseInsensitive))
                selectedPath += "." + shortcutExtension();

            mCore.logEvent(NAME, LOG_EVENT_SEL_PATH.arg(QDir::toNativeSeparators(selectedPath)));
            QFileInfo pathInfo(selectedPath);
            shortcutDir = pathInfo.absoluteDir();
            shortcutName = pathInfo.baseName();
        }
    }

    // Create shortcut folder if needed
    if(!shortcutDir.exists())
    {
        if(!shortcutDir.mkpath(shortcutDir.absolutePath()))
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CREATE_FAILED, ERR_INVALID_PATH));
            return ErrorCode::CANT_CREATE_SHORTCUT;
        }
        mCore.logEvent(NAME, LOG_EVENT_CREATED_DIR_PATH.arg(QDir::toNativeSeparators(shortcutDir.absolutePath())));
    }

    // Create shortcut
    return createShortcut(shortcutName, shortcutDir, shortcutId);
}
