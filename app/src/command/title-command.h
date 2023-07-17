#ifndef TITLE_COMMAND_H
#define TITLE_COMMAND_H

// Project Includes
#include "command/command.h"

class QX_ERROR_TYPE(TitleCommandError, "TitleCommandError", 1211)
{
    friend class TitleCommand;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        InvalidId = 1,
        InvalidRandomFilter = 2,
        MissingTitle = 3
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {InvalidId, QSL("The provided string was not a valid GUID/UUID.")},
        {InvalidRandomFilter, QSL("The provided string for random operation was not a valid filter.")},
        {MissingTitle, QSL("No title was specified.")},
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    TitleCommandError(Type t = NoError, const QString& s = {});

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

class TitleCommand : public Command
{
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Logging - Errors
    static inline const QString LOG_WRN_INVALID_RAND_ID = QSL("A UUID found in the database during Random operation is invalid (%1)");

    // Logging - Messages
    static inline const QString LOG_EVENT_SEL_RAND = QSL("Selecting a playable title at random...");
    static inline const QString LOG_EVENT_INIT_RAND_ID = QSL("Randomly chose primary title is \"%1\"");
    static inline const QString LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT = QSL("Chosen title has %1 playable additional-apps");
    static inline const QString LOG_EVENT_RAND_DET_PRIM = QSL("Selected primary title");
    static inline const QString LOG_EVENT_RAND_DET_ADD_APP = QSL("Selected additional-app \"%1\"");
    static inline const QString LOG_EVENT_RAND_GET_INFO = QSL("Querying random game info...");
    static inline const QString LOG_EVENT_PLAYABLE_COUNT = QSL("Found %1 playable primary titles");

    // Random Selection Info
    static inline const QString RAND_SEL_INFO = QSL(
        "<b>Randomly Selected Game</b><br>"
        "<br>"
        "<b>Title:</b> %1<br>"
        "<b>Developer:</b> %2<br>"
        "<b>Publisher:</b> %3<br>"
        "<b>Library:</b> %4<br>"
        "<b>Variant:</b> %5<br>"
        );
    // Random Filter Sets
    static inline const QStringList RAND_ALL_FILTER_NAMES{QSL("all"), QSL("any")};
    static inline const QStringList RAND_GAME_FILTER_NAMES{QSL("game"), QSL("arcade")};
    static inline const QStringList RAND_ANIM_FILTER_NAMES{QSL("animation"), QSL("theatre")};

    // Command line option strings
    static inline const QString CL_OPT_ID_S_NAME = QSL("i");
    static inline const QString CL_OPT_ID_L_NAME = QSL("id");
    static inline const QString CL_OPT_ID_DESC = QSL("UUID of title to process");

    static inline const QString CL_OPT_TITLE_S_NAME = QSL("t");
    static inline const QString CL_OPT_TITLE_L_NAME = QSL("title");
    static inline const QString CL_OPT_TITLE_DESC = QSL("Title to process");

    static inline const QString CL_OPT_TITLE_STRICT_S_NAME = QSL("T");
    static inline const QString CL_OPT_TITLE_STRICT_L_NAME = QSL("title-strict");
    static inline const QString CL_OPT_TITLE_STRICT_DESC = QSL("Same as -t, but exact matches only");

    static inline const QString CL_OPT_SUBTITLE_S_NAME = QSL("s");
    static inline const QString CL_OPT_SUBTITLE_L_NAME = QSL("subtitle");
    static inline const QString CL_OPT_SUBTITLE_DESC = QSL("Name of additional-app under the title to process. Must be used with -t / -T");

    static inline const QString CL_OPT_SUBTITLE_STRICT_S_NAME = QSL("S");
    static inline const QString CL_OPT_SUBTITLE_STRICT_L_NAME = QSL("subtitle-strict");
    static inline const QString CL_OPT_SUBTITLE_STRICT_DESC = QSL("Same as -s, but exact matches only");

    static inline const QString CL_OPT_RAND_S_NAME = QSL("r");
    static inline const QString CL_OPT_RAND_L_NAME = QSL("random");
    static inline const QString CL_OPT_RAND_DESC = "Select a random title from the database to start. Must be followed by a library filter: " +
                                                   RAND_ALL_FILTER_NAMES.join("/") + ", " + RAND_GAME_FILTER_NAMES.join("/") + " or " + RAND_ANIM_FILTER_NAMES.join("/");

protected:
    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{{CL_OPT_ID_S_NAME, CL_OPT_ID_L_NAME}, CL_OPT_ID_DESC, "id"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE{{CL_OPT_TITLE_S_NAME, CL_OPT_TITLE_L_NAME}, CL_OPT_TITLE_DESC, "title"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_TITLE_STRICT{{CL_OPT_TITLE_STRICT_S_NAME, CL_OPT_TITLE_STRICT_L_NAME}, CL_OPT_TITLE_STRICT_DESC, "title-strict"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_SUBTITLE{{CL_OPT_SUBTITLE_S_NAME, CL_OPT_SUBTITLE_L_NAME}, CL_OPT_SUBTITLE_DESC, "subtitle"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_SUBTITLE_STRICT{{CL_OPT_SUBTITLE_STRICT_S_NAME, CL_OPT_SUBTITLE_STRICT_L_NAME}, CL_OPT_SUBTITLE_STRICT_DESC, "subtitle-strict"}; // Takes value
    static inline const QCommandLineOption CL_OPTION_RAND{{CL_OPT_RAND_S_NAME, CL_OPT_RAND_L_NAME}, CL_OPT_RAND_DESC, "random"}; // Takes value

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID, &CL_OPTION_TITLE, &CL_OPTION_TITLE_STRICT, &CL_OPTION_SUBTITLE,
                                                                              &CL_OPTION_SUBTITLE_STRICT, &CL_OPTION_RAND};

public:
    // Meta
    static inline const QString NAME = QSL("title-command");

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TitleCommand(Core& coreRef);

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    virtual ~TitleCommand() = default;

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Helper
    Qx::Error randomlySelectId(QUuid& mainIdBuffer, QUuid& subIdBuffer, Fp::Db::LibraryFilter lbFilter);
    Qx::Error getRandomSelectionInfo(QString& infoBuffer, QUuid mainId, QUuid subId);

protected:
    virtual QList<const QCommandLineOption*> options() override;
    Qx::Error getTitleId(QUuid& id);
};

#endif // TITLE_COMMAND_H
