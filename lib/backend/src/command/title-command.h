#ifndef TITLE_COMMAND_H
#define TITLE_COMMAND_H

// libfp Includes
#include <fp/fp-db.h>

// Qx Includes
#include <qx/core/qx-fixedstringlist.h>

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
        {NoError, u""_s},
        {InvalidId, u"The provided string was not a valid GUID/UUID."_s},
        {InvalidRandomFilter, u"The provided string for random operation was not a valid filter."_s},
        {MissingTitle, u"No title was specified."_s},
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
    //static inline const QString LOG_WRN_INVALID_RAND_ID = u"A UUID found in the database during Random operation is invalid (%1)"_s;

    // Logging - Messages
    static inline const QString LOG_EVENT_SEL_RAND = u"Selecting a playable title at random..."_s;
    static inline const QString LOG_EVENT_INIT_RAND_ID = u"Randomly chose primary title is \"%1\""_s;
    static inline const QString LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT = u"Chosen title has %1 playable additional-apps"_s;
    static inline const QString LOG_EVENT_GAME_REDIRECT = u"Game redirected: %1 -> %2"_s;
    static inline const QString LOG_EVENT_RAND_DET_PRIM = u"Selected primary title"_s;
    static inline const QString LOG_EVENT_RAND_DET_ADD_APP = u"Selected additional-app \"%1\""_s;
    static inline const QString LOG_EVENT_RAND_GET_INFO = u"Querying random game info..."_s;
    static inline const QString LOG_EVENT_PLAYABLE_COUNT = u"Found %1 playable primary titles"_s;

    // Random Selection Info
    static inline const QString RAND_SEL_INFO =
        u"<b>Randomly Selected Game</b><br>"
        "<br>"
        "<b>Title:</b> %1<br>"
        "<b>Developer:</b> %2<br>"
        "<b>Publisher:</b> %3<br>"
        "<b>Library:</b> %4<br>"
        "<b>Variant:</b> %5<br>"_s;

    /* Random Filter Sets
     * Normally we'd use QStringList for this but we used Qx::StringViewList to ensure compile time construction to prevent SIOF
     * issues with the command line option below that uses them as part of its construction.
     *
     * TODO: Make these Qx::StringViewList for formalities-sake once Clang 19 is common (see Qx doc about the alias)
     */
    static constexpr Qx::FixedStringList RAND_ALL_FILTER_NAMES{u"all", u"any"};
    static constexpr Qx::FixedStringList RAND_GAME_FILTER_NAMES{u"game", u"arcade"};
    static constexpr Qx::FixedStringList RAND_ANIM_FILTER_NAMES{u"animation", u"theatre"};

protected:
    // Command line options
    static inline const QCommandLineOption CL_OPTION_ID{
        {u"i"_s, u"id"_s},
        u"UUID of title to process"_s,
        u"id"_s
    }; // Takes value

    static inline const QCommandLineOption CL_OPTION_TITLE{
        {u"t"_s, u"title"_s},
        u"Title to process"_s,
        u"title"_s
    }; // Takes value

    static inline const QCommandLineOption CL_OPTION_TITLE_STRICT{
        {u"T"_s, u"title-strict"_s},
        u"Same as -t, but exact matches only"_s,
        u"title-strict"_s
    }; // Takes value

    static inline const QCommandLineOption CL_OPTION_SUBTITLE{
        {u"s"_s, u"subtitle"_s},
        u"Name of additional-app under the title to process. Must be used with -t / -T"_s,
        u"subtitle"_s
    }; // Takes value

    static inline const QCommandLineOption CL_OPTION_SUBTITLE_STRICT{
        {u"S"_s, u"subtitle-strict"_s},
        u"Same as -s, but exact matches only"_s,
        u"subtitle-strict"_s
    }; // Takes value
    static inline const QCommandLineOption CL_OPTION_RAND{
        {u"r"_s, u"random"_s},
        u"Select a random title from the database to start. Must be followed by a library filter: "_s +
            RAND_ALL_FILTER_NAMES.join('/') + u", "_s + RAND_GAME_FILTER_NAMES.join('/') + u" or "_s + RAND_ANIM_FILTER_NAMES.join('/'),
        u"random"_s
    }; // Takes value

    static inline const QList<const QCommandLineOption*> CL_OPTIONS_SPECIFIC{&CL_OPTION_ID, &CL_OPTION_TITLE, &CL_OPTION_TITLE_STRICT, &CL_OPTION_SUBTITLE,
                                                                              &CL_OPTION_SUBTITLE_STRICT, &CL_OPTION_RAND};

public:
    // Meta
    static inline const QString NAME = u"title-command"_s;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TitleCommand(Core& coreRef, const QStringList& commandLine);

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    virtual ~TitleCommand() = default;

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Helper
    Qx::Error randomlySelectId(QUuid& mainIdBuffer, QUuid& subIdBuffer, Fp::Libraries lbFilter);
    Qx::Error getRandomSelectionInfo(QString& infoBuffer, QUuid mainId, QUuid subId);
    Qx::Error checkSubtitle(QUuid& addAppId, const QUuid& parentId);

protected:
    virtual QList<const QCommandLineOption*> options() const override;
    Qx::Error getTitleId(QUuid& id);
};

#endif // TITLE_COMMAND_H
