#ifndef CPLAY_H
#define CPLAY_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/title-command.h"

class CPlay : public TitleCommand
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_PLAY = QSL("Playing");

    // Logging - Messages
    static inline const QString LOG_EVENT_ENQ_AUTO = QSL("Enqueuing automatic tasks...");
    static inline const QString LOG_EVENT_ID_MATCH_TITLE = QSL("ID matches main title: %1");
    static inline const QString LOG_EVENT_ID_MATCH_ADDAPP = QSL("ID matches additional app: %1 (Child of %2)");
    static inline const QString LOG_EVENT_QUEUE_CLEARED = QSL("Previous queue entries cleared due to auto task being a Message/Extra");
    static inline const QString LOG_EVENT_FOUND_AUTORUN = QSL("Found autorun-before additional app: %1");
    static inline const QString LOG_EVENT_DATA_PACK_TITLE = QSL("Selected title uses a data pack");

public:
    // Meta
    static inline const QString NAME = QSL("play");
    static inline const QString DESCRIPTION = QSL("Launch a title and all of it's support applications, in the same manner as using the GUI");

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CPlay(Core& coreRef);

//-Class Functions------------------------------------------------------------------------------------------------------
private:
    // Helper
    static Fp::AddApp buildAdditionalApp(const Fp::Db::QueryBuffer& addAppResult);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Queue
     //TODO: Eventually rework to return via ref arg a list of tasks and a bool if app is message/extra so that startup tasks can be enqueued afterwords and queue clearing is unnecessary
    Qx::Error enqueueAutomaticTasks(bool& wasStandalone, QUuid targetId);
    Qx::Error enqueueAdditionalApp(const Fp::AddApp& addApp, const QString& platform, Task::Stage taskStage);
    Qx::Error enqueueGame(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage);

protected:
    QString name() override;
    Qx::Error perform() override;
};
REGISTER_COMMAND(CPlay::NAME, CPlay, CPlay::DESCRIPTION);

#endif // CPLAY_H
