// Unit Include
#include "c-link.h"

// Qt Includes
#include <QFileDialog>

// Qx Includes
#include <qx/windows/qx-common-windows.h>

// Project Includes
#include "c-play.h"
#include "project_vars.h"
#include "utility.h"

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
    QUuid shortcutID;
    QDir shortcutDir;
    QString shortcutName;

    // Get shortcut ID
    if(mParser.isSet(CL_OPTION_ID))
    {
        if((shortcutID = QUuid(mParser.value(CL_OPTION_ID))).isNull())
        {
            mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_INVALID));
            return ErrorCode::ID_NOT_VALID;
        }
    }
    else if(mParser.isSet(CL_OPTION_TITLE))
    {
        if((errorStatus = mCore.getGameIDFromTitle(shortcutID, mParser.value(CL_OPTION_TITLE))))
            return errorStatus;
    }
    else
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_TITLE));
        return ErrorCode::INVALID_ARGS;
    }

    mCore.setStatus(STATUS_LINK, shortcutID.toString(QUuid::WithoutBraces));

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Get entry info (also confirms that ID is present in database)
    QSqlError sqlError;
    Fp::Db::QueryBuffer entryInfo;

    if((sqlError = database->queryEntryById(entryInfo, shortcutID)).isValid())
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
        Fp::Db::QueryBuffer parentInfo;
        if((sqlError = database->queryEntryById(parentInfo,
            QUuid(entryInfo.result.value(Fp::Db::Table_Add_App::COL_PARENT_ID).toString()))).isValid())
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
        if(inputPathInfo.suffix() == LNK_EXT) // Path is file
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
        QString selectedPath = QFileDialog::getSaveFileName(nullptr, DIAG_CAPTION, QDir::homePath() + "/Desktop/" + shortcutName, "Shortcuts (*. " + LNK_EXT + ")");
        if(selectedPath.isEmpty())
        {
            mCore.logEvent(NAME, LOG_EVENT_DIAG_CANCEL);
            return ErrorCode::NO_ERR;
        }
        else
        {
            if(!selectedPath.endsWith("." + LNK_EXT, Qt::CaseInsensitive))
                selectedPath += "." + LNK_EXT;

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
            return ErrorCode::INVALID_SHORTCUT_PARAM;
        }
        mCore.logEvent(NAME, LOG_EVENT_CREATED_DIR_PATH.arg(QDir::toNativeSeparators(shortcutDir.absolutePath())));
    }

    // Create shortcut properties
    Qx::ShortcutProperties sp;
    sp.target = CLIFP_DIR_PATH + "/" + PROJECT_EXECUTABLE_NAME;
    sp.targetArgs = CPlay::NAME + " -" + CPlay::CL_OPT_ID_S_NAME + " " + shortcutID.toString(QUuid::WithoutBraces);
    sp.comment = shortcutName;

    // Create shortcut
    QString fullShortcutPath = shortcutDir.absolutePath() + "/" + shortcutName + "." + LNK_EXT;
    Qx::GenericError shortcutError = Qx::createShortcut(fullShortcutPath, sp);

    // Check for creation failure
    if(shortcutError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CREATE_FAILED, shortcutError.primaryInfo()));
        return ErrorCode::INVALID_SHORTCUT_PARAM;
    }
    else
        mCore.logEvent(NAME, LOG_EVENT_CREATED_SHORTCUT.arg(shortcutID.toString(QUuid::WithoutBraces), QDir::toNativeSeparators(fullShortcutPath)));

    // Return success
    return ErrorCode::NO_ERR;
}
