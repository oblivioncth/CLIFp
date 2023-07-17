// Unit Include
#include "c-link.h"

//===============================================================================================================
// CLinkError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CLinkError::CLinkError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CLinkError::isValid() const { return mType != NoError; }
QString CLinkError::specific() const { return mSpecific; }
CLinkError::Type CLinkError::type() const { return mType; }

//Private:
Qx::Severity CLinkError::deriveSeverity() const { return Qx::Critical; }
quint32 CLinkError::deriveValue() const { return mType; }
QString CLinkError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CLinkError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CLink
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CLink::CLink(Core& coreRef) : TitleCommand(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
QList<const QCommandLineOption*> CLink::options() { return CL_OPTIONS_SPECIFIC + TitleCommand::options(); }
QSet<const QCommandLineOption*> CLink::requiredOptions() { return CL_OPTIONS_REQUIRED + TitleCommand::requiredOptions(); }
QString CLink::name() { return NAME; }

Qx::Error CLink::perform()
{
    // Shortcut parameters
    QUuid shortcutId;
    QDir shortcutDir;
    QString shortcutName;

    // Get shortcut ID
    if(Qx::Error ide = getTitleId(shortcutId); ide.isValid())
        return ide;

    mCore.setStatus(STATUS_LINK, shortcutId.toString(QUuid::WithoutBraces));

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Get entry (also confirms that ID is present in database)
    std::variant<Fp::Game, Fp::AddApp> entry_v;
    Fp::DbError dbError = database->getEntry(entry_v, shortcutId);
    if(dbError.isValid())
    {
        mCore.postError(NAME, dbError);
        return dbError;
    }

    if(std::holds_alternative<Fp::Game>(entry_v))
    {
        Fp::Game game = std::get<Fp::Game>(entry_v);
        shortcutName = Qx::kosherizeFileName(game.title());
    }
    else if(std::holds_alternative<Fp::AddApp>(entry_v))
    {
        Fp::AddApp addApp = std::get<Fp::AddApp>(entry_v);

        // Get parent info
        Fp::DbError dbError = database->getEntry(entry_v, addApp.parentId());
        if(dbError.isValid())
        {
            mCore.postError(NAME, dbError);
            return dbError;
        }
        Q_ASSERT(std::holds_alternative<Fp::Game>(entry_v));

        Fp::Game parent = std::get<Fp::Game>(entry_v);
        shortcutName = Qx::kosherizeFileName(parent.title() + " (" + addApp.name() + ")");
    }
    else
        qCritical("Invalid variant state for std::variant<Fp::Game, Fp::AddApp>.");

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
            return CLinkError();
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
            CLinkError err(CLinkError::InvalidPath);
            mCore.postError(NAME, err);
            return err;
        }
        mCore.logEvent(NAME, LOG_EVENT_CREATED_DIR_PATH.arg(QDir::toNativeSeparators(shortcutDir.absolutePath())));
    }

    // Create shortcut
    return createShortcut(shortcutName, shortcutDir, shortcutId);
}
