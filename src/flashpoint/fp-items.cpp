#include "fp-items.h"
#include "qx.h"

namespace FP
{

//===============================================================================================================
// GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
Game::Game() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid Game::getID() const { return mID; }
QString Game::getTitle() const { return mTitle; }
QString Game::getSeries() const { return mSeries; }
QString Game::getDeveloper() const { return mDeveloper; }
QString Game::getPublisher() const { return mPublisher; }
QDateTime Game::getDateAdded() const { return mDateAdded; }
QDateTime Game::getDateModified() const { return mDateModified; }
QString Game::getPlatform() const { return mPlatform; }
QString Game::getPlayMode() const { return mPlayMode; }
bool Game::isBroken() const { return mBroken; }
QString Game::getStatus() const { return mStatus; }
QString Game::getNotes() const{ return mNotes; }
QString Game::getSource() const { return mSource; }
QString Game::getAppPath() const { return mAppPath; }
QString Game::getLaunchCommand() const { return mLaunchCommand; }
QDateTime Game::getReleaseDate() const { return mReleaseDate; }
QString Game::getVersion() const { return mVersion; }
QString Game::getOriginalDescription() const { return mOriginalDescription; }
QString Game::getLanguage() const { return mLanguage; }
QString Game::getOrderTitle() const { return mOrderTitle; }
QString Game::getLibrary() const { return mLibrary; }

//===============================================================================================================
// GAME BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
GameBuilder::GameBuilder() {}

//-Class Functions---------------------------------------------------------------------------------------------
//Private:
QString GameBuilder::kosherizeRawDate(QString date)
{
    static const QString DEFAULT_MONTH = "-01";
    static const QString DEFAULT_DAY = "-01";

    if(Qx::String::isOnlyNumbers(date) && date.length() == 4) // Year only
        return date + DEFAULT_MONTH + DEFAULT_DAY;
    else if(Qx::String::isOnlyNumbers(date.left(4)) &&
            Qx::String::isOnlyNumbers(date.mid(5,2)) &&
            date.at(4) == '-' && date.length() == 7) // Year and month only
        return  date + DEFAULT_DAY;
    else if(Qx::String::isOnlyNumbers(date.left(4)) &&
            Qx::String::isOnlyNumbers(date.mid(5,2)) &&
            Qx::String::isOnlyNumbers(date.mid(8,2)) &&
            date.at(4) == '-' && date.at(7) == '-' && date.length() == 10) // Year month and day
        return  date;
    else
        return QString(); // Invalid date provided
}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
GameBuilder& GameBuilder::wID(QString rawID) { mGameBlueprint.mID = QUuid(rawID); return *this; }
GameBuilder& GameBuilder::wTitle(QString title) { mGameBlueprint.mTitle = title; return *this; }
GameBuilder& GameBuilder::wSeries(QString series) { mGameBlueprint.mSeries = series; return *this; }
GameBuilder& GameBuilder::wDeveloper(QString developer) { mGameBlueprint.mDeveloper = developer; return *this; }
GameBuilder& GameBuilder::wPublisher(QString publisher) { mGameBlueprint.mPublisher = publisher; return *this; }
GameBuilder& GameBuilder::wDateAdded(QString rawDateAdded) { mGameBlueprint.mDateAdded = QDateTime::fromString(rawDateAdded, Qt::ISODateWithMs); return *this; }
GameBuilder& GameBuilder::wDateModified(QString rawDateModified) { mGameBlueprint.mDateModified = QDateTime::fromString(rawDateModified, Qt::ISODateWithMs); return *this; }
GameBuilder& GameBuilder::wPlatform(QString platform) { mGameBlueprint.mPlatform = platform; return *this; }
GameBuilder& GameBuilder::wBroken(QString rawBroken)  { mGameBlueprint.mBroken = rawBroken.toInt() != 0; return *this; }
GameBuilder& GameBuilder::wPlayMode(QString playMode) { mGameBlueprint.mPlayMode = playMode; return *this; }
GameBuilder& GameBuilder::wStatus(QString status) { mGameBlueprint.mStatus = status; return *this; }
GameBuilder& GameBuilder::wNotes(QString notes)  { mGameBlueprint.mNotes = notes; return *this; }
GameBuilder& GameBuilder::wSource(QString source)  { mGameBlueprint.mSource = source; return *this; }
GameBuilder& GameBuilder::wAppPath(QString appPath)  { mGameBlueprint.mAppPath = appPath; return *this; }
GameBuilder& GameBuilder::wLaunchCommand(QString launchCommand) { mGameBlueprint.mLaunchCommand = launchCommand; return *this; }
GameBuilder& GameBuilder::wReleaseDate(QString rawReleaseDate)  { mGameBlueprint.mReleaseDate = QDateTime::fromString(kosherizeRawDate(rawReleaseDate), Qt::ISODate); return *this; }
GameBuilder& GameBuilder::wVersion(QString version)  { mGameBlueprint.mVersion = version; return *this; }
GameBuilder& GameBuilder::wOriginalDescription(QString originalDescription)  { mGameBlueprint.mOriginalDescription = originalDescription; return *this; }
GameBuilder& GameBuilder::wLanguage(QString language)  { mGameBlueprint.mLanguage = language; return *this; }
GameBuilder& GameBuilder::wOrderTitle(QString orderTitle)  { mGameBlueprint.mOrderTitle = orderTitle; return *this; }
GameBuilder& GameBuilder::wLibrary(QString library) { mGameBlueprint.mLibrary = library; return *this; }

Game GameBuilder::build() { return mGameBlueprint; }

//===============================================================================================================
// ADD APP
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
AddApp::AddApp() {}

//-Opperators----------------------------------------------------------------------------------------------------
//Public:
bool operator== (const AddApp& lhs, const AddApp& rhs) noexcept
{
    return lhs.mID == rhs.mID &&
           lhs.mAppPath == rhs.mAppPath &&
           lhs.mAutorunBefore == rhs.mAutorunBefore &&
           lhs.mLaunchCommand == rhs.mLaunchCommand &&
           lhs.mName == rhs.mName &&
           lhs.mWaitExit == rhs.mWaitExit &&
           lhs.mParentID == rhs.mParentID;
}

//-Hashing------------------------------------------------------------------------------------------------------
uint qHash(const AddApp& key, uint seed) noexcept
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.mID);
    seed = hash(seed, key.mAppPath);
    seed = hash(seed, key.mAutorunBefore);
    seed = hash(seed, key.mLaunchCommand);
    seed = hash(seed, key.mName);
    seed = hash(seed, key.mWaitExit);
    seed = hash(seed, key.mParentID);

    return seed;
}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:
QUuid AddApp::getID() const { return mID; }
QString AddApp::getAppPath() const { return mAppPath; }
bool AddApp::isAutorunBefore() const { return  mAutorunBefore; }
QString AddApp::getLaunchCommand() const { return mLaunchCommand; }
QString AddApp::getName() const { return mName; }
bool AddApp::isWaitExit() const { return mWaitExit; }
QUuid AddApp::getParentID() const { return mParentID; }
bool AddApp::isPlayable() const { return mAppPath != SPEC_PATH_EXTRA && mAppPath != SPEC_PATH_MSG && !mAutorunBefore; }

//===============================================================================================================
// ADD APP BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
AddAppBuilder::AddAppBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
AddAppBuilder& AddAppBuilder::wID(QString rawID) { mAddAppBlueprint.mID = QUuid(rawID); return *this; }
AddAppBuilder& AddAppBuilder::wAppPath(QString appPath) { mAddAppBlueprint.mAppPath = appPath; return *this; }
AddAppBuilder& AddAppBuilder::wAutorunBefore(QString rawAutorunBefore)  { mAddAppBlueprint.mAutorunBefore = rawAutorunBefore.toInt() != 0; return *this; }
AddAppBuilder& AddAppBuilder::wLaunchCommand(QString launchCommand) { mAddAppBlueprint.mLaunchCommand = launchCommand; return *this; }
AddAppBuilder& AddAppBuilder::wName(QString name) { mAddAppBlueprint.mName = name; return *this; }
AddAppBuilder& AddAppBuilder::wWaitExit(QString rawWaitExit)  { mAddAppBlueprint.mWaitExit = rawWaitExit.toInt() != 0; return *this; }
AddAppBuilder& AddAppBuilder::wParentID(QString rawParentID) { mAddAppBlueprint.mParentID = QUuid(rawParentID); return *this; }

AddApp AddAppBuilder::build() { return mAddAppBlueprint; }

//===============================================================================================================
// PLAYLIST
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
Playlist::Playlist() {}

//-Instance Functions------------------------------------------------------------------------------------------------------
//Public:
QUuid Playlist::getID() const { return mID; }
QString Playlist::getTitle() const { return mTitle; }
QString Playlist::getDescription() const { return mDescription; }
QString Playlist::getAuthor() const { return mAuthor; }

//===============================================================================================================
// PLAYLIST BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
PlaylistBuilder::PlaylistBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
PlaylistBuilder& PlaylistBuilder::wID(QString rawID) { mPlaylistBlueprint.mID = QUuid(rawID); return *this; }
PlaylistBuilder& PlaylistBuilder::wTitle(QString title) { mPlaylistBlueprint.mTitle = title; return *this; }
PlaylistBuilder& PlaylistBuilder::wDescription(QString description) { mPlaylistBlueprint.mDescription = description; return *this; }
PlaylistBuilder& PlaylistBuilder::wAuthor(QString author) { mPlaylistBlueprint.mAuthor = author; return *this; }

Playlist PlaylistBuilder::build() { return mPlaylistBlueprint; }

//===============================================================================================================
// PLAYLIST GAME
//===============================================================================================================

//-Constructor------------------------------------------------------------------------------------------------
//Public:
PlaylistGame::PlaylistGame() {}

//-Instance Functions------------------------------------------------------------------------------------------------
//Public:

int PlaylistGame::getID() const { return mID; }
QUuid PlaylistGame::getPlaylistID() const { return mPlaylistID; }
int PlaylistGame::getOrder() const { return mOrder; }
QUuid PlaylistGame::getGameID() const { return mGameID; }

//===============================================================================================================
// PLAYLIST GAME BUILDER
//===============================================================================================================

//-Constructor-------------------------------------------------------------------------------------------------
//Public:
   PlaylistGameBuilder::PlaylistGameBuilder() {}

//-Instance Functions------------------------------------------------------------------------------------------
//Public:
    PlaylistGameBuilder& PlaylistGameBuilder::wID(QString rawID) { mPlaylistGameBlueprint.mID = rawID.toInt(); return *this; }
    PlaylistGameBuilder& PlaylistGameBuilder::wPlaylistID(QString rawPlaylistID) { mPlaylistGameBlueprint.mPlaylistID = QUuid(rawPlaylistID); return *this; }

    PlaylistGameBuilder& PlaylistGameBuilder::wOrder(QString rawOrder)
    {
        bool validInt = false;
        mPlaylistGameBlueprint.mOrder = rawOrder.toInt(&validInt);
        if(!validInt)
            mPlaylistGameBlueprint.mOrder = -1;

        return *this;
    }

    PlaylistGameBuilder& PlaylistGameBuilder::wGameID(QString rawGameID) { mPlaylistGameBlueprint.mGameID = QUuid(rawGameID); return *this; }

    PlaylistGame PlaylistGameBuilder::build() { return mPlaylistGameBlueprint; }
};
