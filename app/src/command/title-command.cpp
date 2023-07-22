// Unit Include
#include "title-command.h"

// Qx Includes
#include <qx/core/qx-genericerror.h>

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
TitleCommand::TitleCommand(Core& coreRef) :
    Command(coreRef)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
Qx::Error TitleCommand::randomlySelectId(QUuid& mainIdBuffer, QUuid& subIdBuffer, Fp::Db::LibraryFilter lbFilter)
{
    mCore.logEvent(NAME, LOG_EVENT_SEL_RAND);

    // Reset buffers
    mainIdBuffer = QUuid();
    subIdBuffer = QUuid();

    // SQL Error tracker
    Fp::DbError searchError;

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Query all main games
    Fp::Db::QueryBuffer mainGameIdQuery;
    searchError = database->queryAllGameIds(mainGameIdQuery, lbFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, searchError);
        return searchError;
    }

    QVector<QUuid> playableIds;

    // Enumerate main game IDs
    for(int i = 0; i < mainGameIdQuery.size; i++)
    {
        // Go to next record
        mainGameIdQuery.result.next();

        // Add ID to list
        QString gameIdString = mainGameIdQuery.result.value(Fp::Db::Table_Game::COL_ID).toString();
        QUuid gameId = QUuid(gameIdString);
        if(!gameId.isNull())
            playableIds.append(gameId);
        else
            mCore.logError(NAME, Qx::GenericError(Qx::Warning, 12011, LOG_WRN_INVALID_RAND_ID.arg(gameIdString)));
    }
    mCore.logEvent(NAME, LOG_EVENT_PLAYABLE_COUNT.arg(QLocale(QLocale::system()).toString(playableIds.size())));

    // Select main game
    mainIdBuffer = playableIds.value(QRandomGenerator::global()->bounded(playableIds.size()));
    mCore.logEvent(NAME, LOG_EVENT_INIT_RAND_ID.arg(mainIdBuffer.toString(QUuid::WithoutBraces)));

    // Get entry's playable additional apps
    Fp::Db::EntryFilter addAppFilter{.type = Fp::Db::EntryType::AddApp, .parent = mainIdBuffer, .playableOnly = true};

    Fp::Db::QueryBuffer addAppQuery;
    searchError = database->queryEntrys(addAppQuery, addAppFilter);
    if(searchError.isValid())
    {
        mCore.postError(NAME, searchError);
        return searchError;
    }
    mCore.logEvent(NAME, LOG_EVENT_INIT_RAND_PLAY_ADD_COUNT.arg(addAppQuery.size));

    QVector<QUuid> playableSubIds;

    // Enumerate entry's playable additional apps
    for(int i = 0; i < addAppQuery.size; i++)
    {
        // Go to next record
        addAppQuery.result.next();

        // Add ID to list
        QString addAppIdString = addAppQuery.result.value(Fp::Db::Table_Game::COL_ID).toString();
        QUuid addAppId = QUuid(addAppIdString);
        if(!addAppId.isNull())
            playableSubIds.append(addAppId);
        else
            mCore.logError(NAME, Qx::GenericError(Qx::Warning, 12101, LOG_WRN_INVALID_RAND_ID.arg(addAppIdString)));
    }

    // Select final ID
    int randIndex = QRandomGenerator::global()->bounded(playableSubIds.size() + 1);

    if(randIndex == 0)
        mCore.logEvent(NAME, LOG_EVENT_RAND_DET_PRIM);
    else
    {
        subIdBuffer = playableSubIds.value(randIndex - 1);
        mCore.logEvent(NAME, LOG_EVENT_RAND_DET_ADD_APP.arg(subIdBuffer.toString(QUuid::WithoutBraces)));
    }

    // Return success
    return TitleCommandError();
}

Qx::Error TitleCommand::getRandomSelectionInfo(QString& infoBuffer, QUuid mainId, QUuid subId)
{
    mCore.logEvent(NAME, LOG_EVENT_RAND_GET_INFO);

    // Reset buffer
    infoBuffer = QString();

    // Template to fill
    QString infoFillTemplate = RAND_SEL_INFO;

    // SQL Error tracker
    Fp::DbError searchError;

    // Get database
    Fp::Db* database = mCore.fpInstall().database();

    // Get main entry info
    std::variant<Fp::Game, Fp::AddApp> entry_v;
    searchError = database->getEntry(entry_v, mainId);
    if(searchError.isValid())
    {
        mCore.postError(NAME, searchError);
        return searchError;
    }

    Q_ASSERT(std::holds_alternative<Fp::Game>(entry_v));
    Fp::Game mainEntry = std::get<Fp::Game>(entry_v);

    // Populate buffer with primary info
    infoFillTemplate = infoFillTemplate.arg(mainEntry.title(),
                                            mainEntry.developer(),
                                            mainEntry.publisher(),
                                            mainEntry.library());

    // Determine variant
    if(subId.isNull())
        infoFillTemplate = infoFillTemplate.arg(u"N/A"_s);
    else
    {
        // Get sub entry info
        searchError = database->getEntry(entry_v, subId);
        if(searchError.isValid())
        {
            mCore.postError(NAME, searchError);
            return searchError;
        }

        Q_ASSERT(std::holds_alternative<Fp::AddApp>(entry_v));
        Fp::AddApp subEntry = std::get<Fp::AddApp>(entry_v);

        // Populate buffer with variant info
        infoFillTemplate = infoFillTemplate.arg(subEntry.name());
    }

    // Set filled template to buffer
    infoBuffer = infoFillTemplate;

    // Return success
    return TitleCommandError();
}

//Protected:
QList<const QCommandLineOption*> TitleCommand::options() { return CL_OPTIONS_SPECIFIC + Command::options(); }

Qx::Error TitleCommand::getTitleId(QUuid& id)
{
    // Reset buffer
    id = QUuid();

    // Get ID of provided title
    QUuid titleId;
    QUuid addAppId;

    if(mParser.isSet(CL_OPTION_ID))
    {
        QString idStr = mParser.value(CL_OPTION_ID);
        if((titleId = QUuid(idStr)).isNull())
        {
            TitleCommandError err(TitleCommandError::InvalidId, idStr);
            mCore.postError(NAME, err);
            return err;
        }
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
        Fp::Db::LibraryFilter randFilter;

        // Check for valid filter
        if(RAND_ALL_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Db::LibraryFilter::Either;
        else if(RAND_GAME_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Db::LibraryFilter::Game;
        else if(RAND_ANIM_FILTER_NAMES.contains(rawRandFilter))
            randFilter = Fp::Db::LibraryFilter::Anim;
        else
        {
            TitleCommandError err(TitleCommandError::InvalidRandomFilter, rawRandFilter);
            mCore.postError(NAME, err);
            return err;
        }

        // Get ID
        if(Qx::Error re = randomlySelectId(titleId, addAppId, randFilter); re.isValid())
            return re;

        // Show selection info
        if(mCore.notifcationVerbosity() == Core::NotificationVerbosity::Full)
        {
            QString selInfo;
            if(Qx::Error rse = getRandomSelectionInfo(selInfo, titleId, addAppId); rse.isValid())
                return rse;

            // Display selection info
            mCore.postMessage(selInfo);
        }
    }
    else
    {
        TitleCommandError err(TitleCommandError::MissingTitle);
        mCore.postError(NAME, err);
        return err;
    }

    // Set buffer
    id = !titleId.isNull() ? titleId : addAppId;

    return TitleCommandError();
}
