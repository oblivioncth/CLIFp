// Unit Include
#include "c-download.h"

// Project Includes
#include "task/t-download.h"
#include "task/t-generic.h"

//===============================================================================================================
// CDownloadError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CDownloadError::CDownloadError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CDownloadError::isValid() const { return mType != NoError; }
QString CDownloadError::specific() const { return mSpecific; }
CDownloadError::Type CDownloadError::type() const { return mType; }

//Private:
Qx::Severity CDownloadError::deriveSeverity() const { return Qx::Critical; }
quint32 CDownloadError::deriveValue() const { return mType; }
QString CDownloadError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CDownloadError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CDownload
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CDownload::CDownload(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
QList<const QCommandLineOption*> CDownload::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }
QSet<const QCommandLineOption*> CDownload::requiredOptions() { return CL_OPTIONS_REQUIRED + Command::requiredOptions(); }
QString CDownload::name() { return NAME; }

Qx::Error CDownload::perform()
{
    QString playlistName = mParser.value(CL_OPTION_PLAYLIST).trimmed();
    mCore.setStatus(STATUS_DOWNLOAD, playlistName);

    Fp::Db* db = mCore.fpInstall().database();
    Fp::PlaylistManager* pm = mCore.fpInstall().playlistManager();
    if(Qx::Error pError = pm->populate(); pError.isValid())
        return pError;

    // Find playlist
    QList<Fp::Playlist> playlists = pm->playlists();
    auto pItr = std::find_if(playlists.cbegin(), playlists.cend(), [&playlistName](auto p){
        return p.title() == playlistName || p.title().trimmed() == playlistName; // Some playlists have spaces for sorting purposes
    });

    if(pItr == playlists.cend())
    {
        CDownloadError err(CDownloadError::InvalidPlaylist, playlistName);
        mCore.postError(NAME, err);
        return err;
    }
    mCore.logEvent(NAME, LOG_EVENT_PLAYLIST_MATCH.arg(pItr->id().toString(QUuid::WithoutBraces)));

    // Queue downloads for each game
    TDownload* downloadTask = new TDownload(&mCore);
    downloadTask->setStage(Task::Stage::Primary);
    downloadTask->setDescription(u"playlist data packs"_s);
    QList<int> dataIds;

    const Fp::Toolkit* tk = mCore.fpInstall().toolkit();
    for(const auto& pg : pItr->playlistGames())
    {
        // Get data
        Fp::GameData gameData;
        if(Fp::DbError gdErr = db->getGameData(gameData, pg.gameId()); gdErr.isValid())
        {
            mCore.postError(NAME, gdErr);
            return gdErr;
        }

        if(gameData.isNull())
        {
            mCore.logEvent(NAME, LOG_EVENT_NON_DATAPACK.arg(pg.gameId().toString(QUuid::WithoutBraces)));
            continue;
        }

        if(tk->datapackIsPresent(gameData))
            continue;

        // Queue download
        downloadTask->addFile({.target = tk->datapackUrl(gameData), .dest = tk->datapackPath(gameData), .checksum = gameData.sha256()});

        // Note data id
        dataIds.append(gameData.id());
    }

    if(downloadTask->isEmpty())
    {
        mCore.logEvent(NAME, LOG_EVENT_NO_OP);
        return CDownloadError();
    }

    // Enqueue download task
    mCore.enqueueSingleTask(downloadTask);

    // Enqueue onDiskState update task
    Core* corePtr = &mCore; // Safe, will outlive task
    TGeneric* onDiskUpdateTask = new TGeneric(corePtr);
    onDiskUpdateTask->setStage(Task::Stage::Primary);
    onDiskUpdateTask->setDescription(u"Update GameData onDisk state."_s);
    onDiskUpdateTask->setAction([dataIds, corePtr]{
        return corePtr->fpInstall().database()->updateGameDataOnDiskState(dataIds, true);
    });
    mCore.enqueueSingleTask(onDiskUpdateTask);

    // Return success
    return CDownloadError();
}
