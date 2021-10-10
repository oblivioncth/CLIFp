#ifndef FLASHPOINT_ITEMS_H
#define FLASHPOINT_ITEMS_H

#include <QString>
#include <QDateTime>
#include <QUuid>

namespace FP
{

//-Class Forward Declarations---------------------------------------------------------------------------------------
class GameBuilder;
class AddAppBuilder;
class PlaylistBuilder;
class PlaylistGameBuilder;

//-Namespace Global Classes-----------------------------------------------------------------------------------------
class Game
{
    friend class GameBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mID;
    QString mTitle;
    QString mSeries;
    QString mDeveloper;
    QString mPublisher;
    QDateTime mDateAdded;
    QDateTime mDateModified;
    QString mPlatform;
    bool mBroken;
    QString mPlayMode;
    QString mStatus;
    QString mNotes;
    QString mSource;
    QString mAppPath;
    QString mLaunchCommand;
    QDateTime mReleaseDate;
    QString mVersion;
    QString mOriginalDescription;
    QString mLanguage;
    QString mOrderTitle;
    QString mLibrary;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Game();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    QUuid getID() const;
    QString getTitle() const;
    QString getSeries() const;
    QString getDeveloper() const;
    QString getPublisher() const;
    QDateTime getDateAdded() const;
    QDateTime getDateModified() const;
    QString getPlatform() const;
    bool isBroken() const;
    QString getPlayMode() const;
    QString getStatus() const;
    QString getNotes() const;
    QString getSource() const;
    QString getAppPath() const;
    QString getLaunchCommand() const;
    QDateTime getReleaseDate() const;
    QString getVersion() const;
    QString getOriginalDescription() const;
    QString getLanguage() const;
    QString getOrderTitle() const;
    QString getLibrary() const;
};

class GameBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    Game mGameBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    GameBuilder();

//-Class Functions---------------------------------------------------------------------------------------------
private:
    static QString kosherizeRawDate(QString date);

//-Instance Functions------------------------------------------------------------------------------------------
public:
    GameBuilder& wID(QString rawID);
    GameBuilder& wTitle(QString title);
    GameBuilder& wSeries(QString series);
    GameBuilder& wDeveloper(QString developer);
    GameBuilder& wPublisher(QString publisher);
    GameBuilder& wDateAdded(QString rawDateAdded);
    GameBuilder& wDateModified(QString rawDateModified);
    GameBuilder& wPlatform(QString platform);
    GameBuilder& wBroken(QString rawBroken);
    GameBuilder& wPlayMode(QString playMode);
    GameBuilder& wStatus(QString status);
    GameBuilder& wNotes(QString notes);
    GameBuilder& wSource(QString source);
    GameBuilder& wAppPath(QString appPath);
    GameBuilder& wLaunchCommand(QString launchCommand);
    GameBuilder& wReleaseDate(QString rawReleaseDate);
    GameBuilder& wVersion(QString version);
    GameBuilder& wOriginalDescription(QString originalDescription);
    GameBuilder& wLanguage(QString language);
    GameBuilder& wOrderTitle(QString orderTitle);
    GameBuilder& wLibrary(QString library);

    Game build();
};

class AddApp
{
    friend class AddAppBuilder;
//-Class Variables-----------------------------------------------------------------------------------------------
private:
    QString SPEC_PATH_MSG = ":message:";
    QString SPEC_PATH_EXTRA = ":extras:";

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mID;
    QString mAppPath;
    bool mAutorunBefore;
    QString mLaunchCommand;
    QString mName;
    bool mWaitExit;
    QUuid mParentID;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    AddApp();

//-Operators-----------------------------------------------------------------------------------------------------------
public:
    friend bool operator== (const AddApp& lhs, const AddApp& rhs) noexcept;

//-Hashing-------------------------------------------------------------------------------------------------------------
public:
    friend uint qHash(const AddApp& key, uint seed) noexcept;

public:

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getID() const;
    QString getAppPath() const;
    bool isAutorunBefore() const;
    QString getLaunchCommand() const;
    QString getName() const;
    bool isWaitExit() const;
    QUuid getParentID() const;
    bool isPlayable() const;
};

class AddAppBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    AddApp mAddAppBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    AddAppBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    AddAppBuilder& wID(QString rawID);
    AddAppBuilder& wAppPath(QString appPath);
    AddAppBuilder& wAutorunBefore(QString rawAutorunBefore);
    AddAppBuilder& wLaunchCommand(QString launchCommand);
    AddAppBuilder& wName(QString name);
    AddAppBuilder& wWaitExit(QString rawWaitExit);
    AddAppBuilder& wParentID(QString rawParentID);

    AddApp build();
};

class Playlist
{
    friend class PlaylistBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mID;
    QString mTitle;
    QString mDescription;
    QString mAuthor;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Playlist();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getID() const;
    QString getTitle() const;
    QString getDescription() const;
    QString getAuthor() const;

};

class PlaylistBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    Playlist mPlaylistBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistBuilder& wID(QString rawID);
    PlaylistBuilder& wTitle(QString title);
    PlaylistBuilder& wDescription(QString description);
    PlaylistBuilder& wAuthor(QString author);

    Playlist build();
};

class PlaylistGame
{
    friend class PlaylistGameBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    int mID;
    QUuid mPlaylistID;
    int mOrder;
    QUuid mGameID;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    int getID() const;
    QUuid getPlaylistID() const;
    int getOrder() const;
    QUuid getGameID() const;
};

class PlaylistGameBuilder
{
//-Instance Variables------------------------------------------------------------------------------------------
private:
    PlaylistGame mPlaylistGameBlueprint;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder();

//-Instance Functions------------------------------------------------------------------------------------------
public:
    PlaylistGameBuilder& wID(QString rawID);
    PlaylistGameBuilder& wPlaylistID(QString rawPlaylistID);
    PlaylistGameBuilder& wOrder(QString rawOrder);
    PlaylistGameBuilder& wGameID(QString rawGameID);

    PlaylistGame build();
};

}

#endif // FLASHPOINT_ITEMS_H
