// Unit Include
#include "title-command.h"

// Qx Includes
#include <qx/core/qx-genericerror.h>

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "kernel/core.h"

//===============================================================================================================
// TitleCommandError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TitleCommandError::TitleCommandError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TitleCommandError::isValid() const { return mType != NoError; }
QString TitleCommandError::specific() const { return mSpecific; }
TitleCommandError::Type TitleCommandError::type() const { return mType; }

//Private:
Qx::Severity TitleCommandError::deriveSeverity() const { return Qx::Critical; }
quint32 TitleCommandError::deriveValue() const { return mType; }
QString TitleCommandError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TitleCommandError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// TitleCommand
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
TitleCommand::TitleCommand(Core& coreRef, const QStringList& commandLine) :
    Command(coreRef, commandLine)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
Qx::Error TitleCommand::randomlySelectId(QUuid& mainIdBuffer, QUuid& subIdBuffer, Fp::Libraries lbFilter)
{
    logEvent(LOG_EVENT_SEL_RAND);

    // Reset buffers
    mainIdBuffer = QUuid();
    subIdBuffer = QUuid();

    // SQL Error tracker
    Fp::DbError searchError;

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Query all main games
    QList<QUuid> playableIds;
    if(searchError = database->getAllGameIds(playableIds, lbFilter); searchError.isValid())
    {
        postDirective<DError>(searchError);
        return searchError;
    }
    // NOTE: We used to check if ids were valid here and omit them if so. If bad IDs are found in DB, reintroduce.
    logEvent(LOG_EVENT_PLAYABLE_COUNT.arg(QLocale(QLocale::system()).toString(playableIds.size())));

    // Select main game
    mainIdBuffer = playableIds.value(QRandomGenerator::global()->bounded(playableIds.size()));
    logEvent(LOG_EVENT_INIT_RAND_ID.arg(mainIdBuffer.toString(QUuid::WithoutBraces)));

    // Get entry's playable additional apps
    Fp::Db::AddAppFilter aaf{.parent = mainIdBuffer, .playableOnly = true};

    QList<QUuid> playableSubIds;
    if(searchError = database->searchAddAppIds(playableSubIds, aaf); searchError.isValid())
    {
        postDirective<DError>(searchError);
        return searchError;
    }
    // NOTE: We used to check if ids were valid here and omit them if so. If bad IDs are found in DB, reintroduce.
    logEvent(LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT.arg(playableSubIds.size()));

    // Select final ID
    int randIndex = QRandomGenerator::global()->bounded(playableSubIds.size() + 1);

    if(randIndex == 0)
        logEvent(LOG_EVENT_RAND_DET_PRIM);
    else
    {
        subIdBuffer = playableSubIds.value(randIndex - 1);
        logEvent(LOG_EVENT_RAND_DET_ADD_APP.arg(subIdBuffer.toString(QUuid::WithoutBraces)));
    }

    // Return success
    return TitleCommandError();
}

Qx::Error TitleCommand::getRandomSelectionInfo(QString& infoBuffer, QUuid mainId, QUuid subId)
{
    logEvent(LOG_EVENT_RAND_GET_INFO);

    // Reset buffer
    infoBuffer = QString();

    // Template to fill
    QString infoFillTemplate = RAND_SEL_INFO;

    // SQL Error tracker
    Fp::DbError searchError;

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Get main entry info
    Fp::Game game;
    if(searchError = database->getGame(game, mainId); searchError.isValid())
    {
        postDirective<DError>(searchError);
        return searchError;
    }

    // Populate buffer with primary info
    infoFillTemplate = infoFillTemplate.arg(game.title(),
                                            game.developer(),
                                            game.publisher(),
                                            game.library() == Fp::Library::Game ? u"Game"_s : u"Animation"_s);

    // Handle possible sub info
    if(subId.isNull())
        infoFillTemplate = infoFillTemplate.arg(u"N/A"_s);
    else
    {
        // Get sub entry info
        Fp::AddApp addApp;
        if(searchError = database->getAddApp(addApp, subId); searchError.isValid())
        {
            postDirective<DError>(searchError);
            return searchError;
        }

        // Populate buffer with info
        infoFillTemplate = infoFillTemplate.arg(addApp.name());
    }

    // Set filled template to buffer
    infoBuffer = infoFillTemplate;

    // Return success
    return TitleCommandError();
}

//Protected:
QList<const QCommandLineOption*> TitleCommand::options() const { return CL_OPTIONS_SPECIFIC + Command::options(); }

Qx::Error TitleCommand::getTitleId(QUuid& id)
{
    // Reset buffer
    id = QUuid();

    // Get ID of provided title
    QUuid titleId;
    QUuid addAppId;

    if(mParser.isSet(CL_OPTION_ID)) // TODO: Check that directly supplied IDs actually belong to a title
    {
        QString idStr = mParser.value(CL_OPTION_ID);
        if((titleId = QUuid(idStr)).isNull())
        {
            TitleCommandError err(TitleCommandError::InvalidId, idStr);
            postDirective<DError>(err);
            return err;
        }
        QUuid origId = titleId;
        titleId = mCore.fpInstall().database()->handleGameRedirects(titleId); // Redirect shortcut
        if(titleId != origId)
            logEvent(LOG_EVENT_GAME_REDIRECT.arg(origId.toString(QUuid::WithoutBraces), titleId.toString(QUuid::WithoutBraces)));
    }
    else if(mParser.isSet(CL_OPTION_TITLE) || mParser.isSet(CL_OPTION_TITLE_STRICT))
    {
        // Check title
        bool titleStrict = mParser.isSet(CL_OPTION_TITLE_STRICT);
        QString title = titleStrict ? mParser.value(CL_OPTION_TITLE_STRICT) : mParser.value(CL_OPTION_TITLE);

        if(Qx::Error se = mCore.findGameIdFromTitle(titleId, title, titleStrict); se.isValid())
            return se;

        // Bail if canceled
        if(titleId.isNull())
            return TitleCommandError();

        // Check subtitle
        if(mParser.isSet(CL_OPTION_SUBTITLE) || mParser.isSet(CL_OPTION_SUBTITLE_STRICT))
        {
            bool subtitleStrict = mParser.isSet(CL_OPTION_SUBTITLE_STRICT);
            QString subtitle = subtitleStrict ? mParser.value(CL_OPTION_SUBTITLE_STRICT) : mParser.value(CL_OPTION_SUBTITLE);

            if(Qx::Error se = mCore.findAddAppIdFromName(addAppId, titleId, subtitle, subtitleStrict); se.isValid())
                return se;

            // Bail if canceled
            if(addAppId.isNull())
                return TitleCommandError();
        }
    }
    else if(mParser.isSet(CL_OPTION_RAND))
    {
        QString rawRandFilter = mParser.value(CL_OPTION_RAND);
        Fp::Libraries randFilter;

        // Check for valid filter
        if(RAND_ALL_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Library::All;
        else if(RAND_GAME_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Library::Game;
        else if(RAND_ANIM_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Library::Animation;
        else
        {
            TitleCommandError err(TitleCommandError::InvalidRandomFilter, rawRandFilter);
            postDirective<DError>(err);
            return err;
        }

        // Get ID
        if(Qx::Error re = randomlySelectId(titleId, addAppId, randFilter); re.isValid())
            return re;

        QString selInfo;
        if(Qx::Error rse = getRandomSelectionInfo(selInfo, titleId, addAppId); rse.isValid())
            return rse;

        // Display selection info
        postDirective<DMessage>(selInfo);
    }
    else
    {
        TitleCommandError err(TitleCommandError::MissingTitle);
        postDirective<DError>(err);
        return err;
    }

    // Set buffer
    id = addAppId.isNull() ? titleId : addAppId;

    return TitleCommandError();
}
