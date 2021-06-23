#include "c-shortcut.h"
#include "c-play.h"

#include "qfiledialog.h"
#include "qx-windows.h"
#include "version.h"

//===============================================================================================================
// CSHORTCUT
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CShortcut::CShortcut(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
const QList<const QCommandLineOption*> CShortcut::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
const QString CShortcut::name() { return NAME; }

//Public:
ErrorCode CShortcut::process(const QStringList& commandLine)
{
    ErrorCode errorStatus;

    // Parse and check for valid arguments
    if((errorStatus = parse(commandLine)))
        return errorStatus;

    // Handle standard options
    if(checkStandardOptions())
        return Core::ErrorCodes::NO_ERR;

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
            return Core::ErrorCodes::ID_NOT_VALID;
        }
    }
    else
    {
        mCore.logError(NAME, Qx::GenericError(Qx::GenericError::Error, Core::LOG_ERR_INVALID_PARAM, ERR_NO_ID));
        return Core::ErrorCodes::INVALID_ARGS;
    }

    // Open database
    if((errorStatus = mCore.openAndVerifyProperDatabase()))
        return errorStatus;

    // Get entry info (also confirms that ID is present in database)
    QSqlError sqlError;
    FP::Install::DBQueryBuffer entryInfo;

    if((sqlError = mCore.getFlashpointInstall().queryEntryByID(entryInfo, shortcutID)).isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_UNEXPECTED_SQL, sqlError.text()));
        return Core::ErrorCodes::SQL_ERROR;
    }
    else if(entryInfo.size < 1)
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, Core::ERR_ID_NOT_FOUND, sqlError.text()));
        return Core::ErrorCodes::ID_NOT_FOUND;
    }

    // Get entry title
    entryInfo.result.next();
    shortcutName = Qx::kosherizeFileName(entryInfo.result.value(FP::Install::DBTable_Game::COL_TITLE).toString());

    // Get shortcut path
    if(mParser.isSet(CL_OPTION_PATH))
    {
        QString inputPath = mParser.value(CL_OPTION_PATH);
        if(inputPath.back() == '\\' || inputPath.back() == '/')
            shortcutDir = QDir(inputPath);
        else
        {
            QFileInfo pathInfo(inputPath);
            shortcutDir = pathInfo.absoluteDir();
            shortcutName = pathInfo.suffix() == LNK_EXT ? pathInfo.baseName() : pathInfo.fileName();
        }
    }
    else
    {
        QString selectedPath = QFileDialog::getSaveFileName(nullptr, DIAG_CAPTION, QDir::homePath(), "Shortcuts (*. " + LNK_EXT + ")");
        if(selectedPath.isEmpty())
        {
            mCore.logEvent(NAME, LOG_EVENT_DIAG_CANCEL);
            return Core::ErrorCodes::NO_ERR;
        }
        else
        {
            QFileInfo pathInfo(selectedPath);
            shortcutDir = pathInfo.absoluteDir();
            shortcutName = pathInfo.baseName();
        }
    }

    // Create shortcut folder
    if(!shortcutDir.mkpath(shortcutDir.absolutePath()))
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CREATE_FAILED, ERR_INVALID_PATH));
        return ErrorCodes::INVALID_SHORTCUT_PARAM;
    }

    // Create shortcut properties
    Qx::ShortcutProperties sp;
    sp.target = CLIFP_DIR_PATH + "/" + VER_ORIGINALFILENAME_STR;
    sp.targetArgs = CPlay::NAME + " -" + CPlay::CL_OPT_ID_S_NAME + " " + shortcutID.toString(QUuid::WithoutBraces);
    sp.comment = shortcutName;

    // Create shortcut
    Qx::GenericError shortcutError = Qx::createShortcut(shortcutDir.absolutePath() + "/" + shortcutName + LNK_EXT, sp);

    // Check for creation failure
    if(shortcutError.isValid())
    {
        mCore.postError(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CREATE_FAILED, shortcutError.primaryInfo()));
        return ErrorCodes::INVALID_SHORTCUT_PARAM;
    }

    // Return success
    return Core::ErrorCodes::NO_ERR;
}
