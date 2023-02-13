#ifndef CPLAY_H
#define CPLAY_H

#include "command/command.h"

class CPlay : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_PLAY = "Playing";

    // Error Messages - Prep
    static inline const QString ERR_NO_TITLE = "No title to start was specified.";
    static inline const QString ERR_RAND_FILTER_INVALID = "The provided string for random operation was not a valid filter.";
    static inline const QString ERR_ID_DUPLICATE_ENTRY_P = "Multiple entries with the specified ID were found.";
    static inline const QString ERR_ID_DUPLICATE_ENTRY_S = "This should not be possible and may indicate an error within the Flashpoint database";
    static inline const QString ERR_PARENT_INVALID = "The parent ID of the target additional app was not valid.";

    // Logging - Messages
    static inline const QString LOG_EVENT_ENQ_AUTO = "Enqueuing automatic tasks...";
    static inline const QString LOG_EVENT_ID_MATCH_TITLE = "ID matches main title: %1";
    static inline const QString LOG_EVENT_ID_MATCH_ADDAPP = "ID matches additional app: %1 (Child of %2)";
    static inline const QString LOG_EVENT_QUEUE_CLEARED = "Previous queue entries cleared due to auto task being a Message/Extra";
    static inline const QString LOG_EVENT_FOUND_AUTORUN = "Found autorun-before additional app: %1";
    static inline const QString LOG_EVENT_DATA_PACK_TITLE = "Selected title uses a data pack";
    static inline const QString LOG_EVENT_SEL_RAND = "Selecting a playable title at random...";
    static inline const QString LOG_EVENT_INIT_RAND_ID = "Randomly chose primary title is \"%1\"";
    static inline const QString LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT = "Chosen title has %1 playable additional-apps";
    static inline const QString LOG_EVENT_RAND_DET_PRIM = "Selected primary title";
    static inline const QString LOG_EVENT_RAND_DET_ADD_APP = "Selected additional-app \"%1\"";
    static inline const QString LOG_EVENT_RAND_GET_INFO = "Querying random game info...";
    static inline const QString LOG_EVENT_PLAYABLE_COUNT = "Found %1 playable primary titles";

    // Logging - Errors
    static inline const QString LOG_WRN_INVALID_RAND_ID = "A UUID found in the database during Random operation is invalid (%1)";

    // Random Selection Info
    static inline const QString RAND_SEL_INFO =
            "<b>Randomly Selected Game</b><br>"
            "<br>"
            "<b>Title:</b> %1<br>"
            "<b>Developer:</b> %2<br>"
            "<b>Publisher:</b> %3<br>"
            "<b>Library:</b> %4<br>"
            "<b>Variant:</b> %5<br>";

    // Random Filter Sets
    static inline const QStringList RAND_ALL_FILTER_NAMES{"all", "any"};
    static inline const QStringList RAND_GAME_FILTER_NAMES{"game", "arcade"};
    static inline const QStringList RAND_ANIM_FILTER_NAMES{"animation", "theatre"};

public:
    // Command line option strings
    static inline const QString CL_OPT_ID_S_NAME = "i";
    static inline const QString CL_OPT_ID_L_NAME = "id";
    static inline const QString CL_OPT_ID_DESC = "UUID of title to start";

    static inline const QString CL_OPT_TITLE_S_NAME = "t";
    static inline const QString CL_OPT_TITLE_L_NAME = "title";
    static inline const QString CL_OPT_TITLE_DESC = "Title to start";

    static inline const QString CL_OPT_TITLE_STRICT_S_NAME = "T";
    static inline const QString CL_OPT_TITLE_STRICT_L_NAME = "title-strict";
    static inline const QString CL_OPT_TITLE_STRICT_DESC = "Same as -t, but exact matches only";

    static inline const QString CL_OPT_SUBTITLE_S_NAME = "s";
    static inline const QString CL_OPT_SUBTITLE_L_NAME = "subtitle";
    static inline const QString CL_OPT_SUBTITLE_DESC = "Name of additional-app under the title to start. Must be used with -t / -T";

    static inline const QString CL_OPT_SUBTITLE_STRICT_S_NAME = "S";
    static inline const QString CL_OPT_SUBTITLE_STRICT_L_NAME = "subtitle-strict";
    static inline const QString CL_OPT_SUBTITLE_STRICT_DESC = "Same as -s, but exact matches only";

    static inline const QString CL_OPT_RAND_S_NAME = "r";
    static inline const QString CL_OPT_RAND_L_NAME = "random";
    static inline const QString CL_OPT_RAND_DESC = "Select a random title from the database to start. Must be followed by a library filter: " +
                                                   RAND_ALL_FILTER_NAMES.join("/") + ", " + RAND_GAME_FILTER_NAMES.join("/") + " or " + RAND_ANIM_FILTER_NAMES.join("/");
private:
    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{{CL_OPT_ID_S_NAME, CL_OPT_ID_L_NAME}, CL_OPT_ID_DESC, "id"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE{{CL_OPT_TITLE_S_NAME, CL_OPT_TITLE_L_NAME}, CL_OPT_TITLE_DESC, "title"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE_STRICT{{CL_OPT_TITLE_STRICT_S_NAME, CL_OPT_TITLE_STRICT_L_NAME}, CL_OPT_TITLE_STRICT_DESC, "title-strict"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_SUBTITLE{{CL_OPT_SUBTITLE_S_NAME, CL_OPT_SUBTITLE_L_NAME}, CL_OPT_SUBTITLE_DESC, "subtitle"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_SUBTITLE_STRICT{{CL_OPT_SUBTITLE_STRICT_S_NAME, CL_OPT_SUBTITLE_STRICT_L_NAME}, CL_OPT_SUBTITLE_STRICT_DESC, "subtitle-strict"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_RAND{{CL_OPT_RAND_S_NAME, CL_OPT_RAND_L_NAME}, CL_OPT_RAND_DESC, "random"}; // Takes value
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID, &CL_OPTION_TITLE, &CL_OPTION_TITLE_STRICT,
                                                                             &CL_OPTION_SUBTITLE, &CL_OPTION_SUBTITLE_STRICT, &CL_OPTION_RAND};

public:
    // Meta
    static inline const QString NAME = "play";
    static inline const QString DESCRIPTION = "Launch a title and all of it's support applications, in the same manner as using the GUI";

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CPlay(Core& coreRef);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    // Helper
    static Fp::AddApp buildAdditionalApp(const Fp::Db::QueryBuffer& addAppResult);
    static Fp::Game buildGame(const Fp::Db::QueryBuffer& gameResult);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Queue
     //TODO: Eventually rework to return via ref arg a list of tasks and a bool if app is message/extra so that startup tasks can be enq afterwords and queue clearing is unneccesary
    ErrorCode enqueueAutomaticTasks(bool&wasStandalone, QUuid targetId);
    ErrorCode enqueueAdditionalApp(const Fp::AddApp& addApp, const QString& platform, Task::Stage taskStage);
    ErrorCode enqueueGame(const Fp::Game& game, Task::Stage taskStage);

    // Helper
    ErrorCode randomlySelectId(QUuid& mainIdBuffer, QUuid& subIdBuffer, Fp::Db::LibraryFilter lbFilter);
    ErrorCode getRandomSelectionInfo(QString& infoBuffer, QUuid mainId, QUuid subId);

protected:
    const QList<const QCommandLineOption*> options();
    const QString name();

public:
    ErrorCode process(const QStringList& commandLine);
};
REGISTER_COMMAND(CPlay::NAME, CPlay, CPlay::DESCRIPTION);

#endif // CPLAY_H
