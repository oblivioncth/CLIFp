#ifndef FLASHPOINT_ITEMS_H
#define FLASHPOINT_ITEMS_H

// Qt Includes
#include <QString>
#include <QDateTime>
#include <QUuid>

namespace Fp
{
//-Enums----------------------------------------------------------------------------------------------------------
enum class ImageType {Logo, Screenshot};

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
    QUuid mId;
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
    QUuid getId() const;
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
    GameBuilder& wId(QString rawId);
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
    QUuid mId;
    QString mAppPath;
    bool mAutorunBefore;
    QString mLaunchCommand;
    QString mName;
    bool mWaitExit;
    QUuid mParentId;

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
    QUuid getId() const;
    QString getAppPath() const;
    bool isAutorunBefore() const;
    QString getLaunchCommand() const;
    QString getName() const;
    bool isWaitExit() const;
    QUuid getParentId() const;
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
    AddAppBuilder& wId(QString rawId);
    AddAppBuilder& wAppPath(QString appPath);
    AddAppBuilder& wAutorunBefore(QString rawAutorunBefore);
    AddAppBuilder& wLaunchCommand(QString launchCommand);
    AddAppBuilder& wName(QString name);
    AddAppBuilder& wWaitExit(QString rawWaitExit);
    AddAppBuilder& wParentId(QString rawParentId);

    AddApp build();
};

class Playlist
{
    friend class PlaylistBuilder;

//-Instance Variables-----------------------------------------------------------------------------------------------
private:
    QUuid mId;
    QString mTitle;
    QString mDescription;
    QString mAuthor;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Playlist();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QUuid getId() const;
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
    PlaylistBuilder& wId(QString rawId);
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
    int mId;
    QUuid mPlaylistId;
    int mOrder;
    QUuid mGameId;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    PlaylistGame();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    int getId() const;
    QUuid getPlaylistId() const;
    int getOrder() const;
    QUuid getGameId() const;
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
    PlaylistGameBuilder& wId(QString rawId);
    PlaylistGameBuilder& wPlaylistId(QString rawPlaylistId);
    PlaylistGameBuilder& wOrder(QString rawOrder);
    PlaylistGameBuilder& wGameId(QString rawGameId);

    PlaylistGame build();
};

}

#endif // FLASHPOINT_ITEMS_H
