#ifndef CPLAY_H
#define CPLAY_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "command/title-command.h"
#include "task/task.h"

class TExec;

class QX_ERROR_TYPE(CPlayError, "CPlayError", 1212)
{
    friend class CPlay;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        InvalidUrl = 1,
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {InvalidUrl, u"The provided 'flashpoint://' scheme URL is invalid."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    CPlayError(Type t = NoError, const QString& s = {});

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class CPlay : public TitleCommand
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Status
    static inline const QString STATUS_PLAY = u"Playing"_s;

    // General
    static inline const QRegularExpression URL_REGEX = QRegularExpression(
        u"flashpoint:\\/\\/(?<id>[0-9a-fA-F]{8}\\b-[0-9a-fA-F]{4}\\b-[0-9a-fA-F]{4}\\b-[0-9a-fA-F]{4}\\b-[0-9a-fA-F]{12})\\/?$"_s
    );
    static inline const QHash<QString, QString> FULLSCREEN_PARAMS{
        /* Might want to do this in a more flexible way to account for exe name differences,
         * but for now it's just a map of the lowercase basename of the executeable to the parameter(s)
         * to use for fullscreen
         */
        {u"ruffle"_s, u"--fullscreen"_s}
    };

    // Command line option strings
    static inline const QString CL_OPT_FULLSCREEN_S_NAME = u"f"_s;
    static inline const QString CL_OPT_FULLSCREEN_L_NAME = u"fullscreen"_s;
    static inline const QString CL_OPT_FULLSCREEN_DESC = u"Runs the title in fullscreen mode, if supported."_s;

    static inline const QString CL_OPT_URL_S_NAME = u"u"_s;
    static inline const QString CL_OPT_URL_L_NAME = u"url"_s;
    static inline const QString CL_OPT_URL_DESC = u""_s;

    static inline const QString CL_OPT_RUFFLE_L_NAME = u"ruffle"_s;
    static inline const QString CL_OPT_RUFFLE_DESC = u"Forces the use of Ruffle for Flash games."_s;

    static inline const QString CL_OPT_FLASH_L_NAME = u"flash"_s;
    static inline const QString CL_OPT_FLASH_DESC = u"Forces the use of the standard app (usually Flash Player) for Flash games."_s;

    // Command line options
    static inline const QCommandLineOption CL_OPTION_URL{{CL_OPT_URL_S_NAME, CL_OPT_URL_L_NAME}, CL_OPT_URL_DESC, u"url"_s}; // Takes value
    static inline const QCommandLineOption CL_OPTION_FULLSCREEN{{CL_OPT_FULLSCREEN_S_NAME, CL_OPT_FULLSCREEN_L_NAME}, CL_OPT_FULLSCREEN_DESC}; // Boolean
    static inline const QCommandLineOption CL_OPTION_RUFFLE{{CL_OPT_RUFFLE_L_NAME}, CL_OPT_RUFFLE_DESC}; // Boolean
    static inline const QCommandLineOption CL_OPTION_FLASH{{CL_OPT_FLASH_L_NAME}, CL_OPT_FLASH_DESC}; // Boolean
    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_URL, &CL_OPTION_FULLSCREEN, &CL_OPTION_RUFFLE, &CL_OPTION_FLASH};

    // Logging - Messages
    static inline const QString LOG_EVENT_HANDLING_AUTO = u"Handling automatic tasks..."_s;
    static inline const QString LOG_EVENT_URL_ID = u"ID from URL: %1"_s;
    static inline const QString LOG_EVENT_ID_MATCH_TITLE = u"ID matches main title: %1"_s;
    static inline const QString LOG_EVENT_ID_MATCH_ADDAPP = u"ID matches additional app: %1 (Child of %2)"_s;
    static inline const QString LOG_EVENT_FOUND_AUTORUN = u"Found autorun-before additional app: %1"_s;
    static inline const QString LOG_EVENT_DATA_PACK_TITLE = u"Selected title uses a data pack"_s;
    static inline const QString LOG_EVENT_SERVER_OVERRIDE = u"Selected title overrides the server to: %1"_s;
    static inline const QString LOG_EVENT_FORCING_FULLSCREEN = u"Fullscreen requested..."_s;
    static inline const QString LOG_EVENT_FULLSCREEN_UNSUPPORTED = u"No fullscreen parameter is known for this application."_s;
    static inline const QString LOG_EVENT_FULLSCREEN_SUPPORTED = u"Fullscreen parameter: %1"_s;
    static inline const QString LOG_EVENT_USING_RUFFLE_SUPPORTED = u"Using Ruffle for this title (supported)"_s;
    static inline const QString LOG_EVENT_USING_RUFFLE_UNSUPPORTED = u"Using Ruffle for this title (unsupported)"_s;
    static inline const QString LOG_EVENT_FORCING_RUFFLE = u"Forcing the use of Ruffle for this title"_s;
    static inline const QString LOG_EVENT_FORCING_FLASH = u"Forcing the use of the standard Flash application for this title"_s;

public:
    // Meta
    static inline const QString NAME = u"play"_s;
    static inline const QString DESCRIPTION = u"Launch a game/animation"_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    CPlay(Core& coreRef, const QStringList& commandLine);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    void addExtraExecParameters(TExec* execTask, Task::Stage taskStage);
    QString getServerOverride(const Fp::GameData& gd);
    bool useRuffle(const Fp::Game& game, Task::Stage stage);
    Qx::Error handleEntry(const Fp::Game& game);
    Qx::Error handleEntry(const Fp::AddApp& addApp);
    Qx::Error enqueueAdditionalApp(const Fp::AddApp& addApp, const Fp::Game& parent, Task::Stage taskStage);
    Qx::Error enqueueGame(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage);
    TExec* createStdGameExecTask(const Fp::Game& game, const Fp::GameData& gameData, Task::Stage taskStage);
    TExec* createStdAddAppExecTask(const Fp::AddApp& addApp, const Fp::Game& parent, Task::Stage taskStage);
    TExec* createRuffleTask(const QString& name, const QString& originalParams);

protected:
    QList<const QCommandLineOption*> options() const override;
    QString name() const override;

public:
    Qx::Error perform() override;

public:
    bool requiresServices() const override;
};

#endif // CPLAY_H
