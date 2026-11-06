// Unit Include
#include "c-share.h"

// Qx Includes
#include <qx/core/qx-system.h>
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "kernel/core.h"
#include "task/t-message.h"

//===============================================================================================================
// CShareError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CShareError::CShareError(Type t, const QString& s, Qx::Severity sv) :
    mType(t),
    mSpecific(s),
    mSeverity(sv)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CShareError::isValid() const { return mType != NoError; }
QString CShareError::specific() const { return mSpecific; }
CShareError::Type CShareError::type() const { return mType; }

//Private:
Qx::Severity CShareError::deriveSeverity() const { return mSeverity; }
quint32 CShareError::deriveValue() const { return mType; }
QString CShareError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CShareError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CShare
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CShare::CShare(Core& coreRef, const QStringList& commandLine) : TitleCommand(coreRef, commandLine) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
QList<const QCommandLineOption*> CShare::options() const { return CL_OPTIONS_SPECIFIC + TitleCommand::options(); }
QString CShare::name() const { return NAME; }

//Public:
Qx::Error CShare::perform()
{
    bool tookAction = false;

    // Prioritize scheme (un)registration (registration causes unregistration to be ignored)
    if(mParser.isSet(CL_OPTION_CONFIGURE) || mParser.isSet(CL_OPTION_CONFIGURED))
    {
        tookAction = true;
        logEvent(LOG_EVENT_REGISTRATION);

        if(Qx::isDefaultProtocolHandler(SCHEME))
        {
            logEvent(MSG_ALREADY_REGISTERED);
            if(!mParser.isSet(CL_OPTION_CONFIGURED)) // Non-idempotent form
            {
                CShareError wrn(CShareError::AlreadyRegistered, {}, Qx::Warning);
                postDirective<DError>(wrn);
            }
        }
        else if(!Qx::setDefaultProtocolHandler(SCHEME, SCHEME_NAME))
        {
            CShareError err(CShareError::RegistrationFailed);
            postDirective<DError>(err);
            return err;
        }
        else
        {
            // Enqueue success message task
            TMessage* successMsg = new TMessage(mCore);
            successMsg->setStage(Task::Stage::Primary);
            successMsg->setText(MSG_REGISTRATION_COMPLETE);
            mCore.enqueueSingleTask(successMsg);
        }
    }
    else if(mParser.isSet(CL_OPTION_UNCONFIGURE) || mParser.isSet(CL_OPTION_UNCONFIGURED))
    {
        tookAction = true;
        logEvent(LOG_EVENT_UNREGISTRATION);

        if(!Qx::isDefaultProtocolHandler(SCHEME))
        {
            logEvent(MSG_NOT_REGISTERED);
            if(!mParser.isSet(CL_OPTION_UNCONFIGURED)) // Non-idempotent form
            {
                CShareError wrn(CShareError::NotRegistered, {}, Qx::Warning);
                postDirective<DError>(wrn);
            }
        }
#ifdef __linux__ // Function is too jank on linux right now, so always fail/no-op there
        else if(true)
#else
        else if(!Qx::removeDefaultProtocolHandler(SCHEME))
#endif
        {
            CShareError err(CShareError::UnregistrationFailed);
            postDirective<DError>(err);
            return err;
        }
        else
        {
            // Enqueue success message task
            TMessage* successMsg = new TMessage(mCore);
            successMsg->setStage(Task::Stage::Primary);
            successMsg->setText(MSG_UNREGISTRATION_COMPLETE);
            mCore.enqueueSingleTask(successMsg);
        }
    }

    // Generate share if requested. We also force this if no action was done so far so that
    // we can get TitleCommand's error for not providing any args
    bool findTitle = !tookAction || [this]{
        // See if any TitleCommand options were used
        // TODO: Make this check a TitleCommand protected function?
        for(auto op : qxAsConst(TitleCommand::options()))
            if(mParser.isSet(*op))
                return true;

        return false;
    }();

    if(findTitle)
    {
        // Get ID of title to share
        QUuid shareId;
        if(Qx::Error ide = getTitleId(shareId); ide.isValid())
            return ide;

        postDirective<DStatusUpdate>(STATUS_SHARE, shareId.toString(QUuid::WithoutBraces));

        // Generate URL
        QString idStr = shareId.toString(QUuid::WithoutBraces);
        QString shareUrl = mParser.isSet(CL_OPTION_UNIVERSAL) ? SCHEME_TEMPLATE_UNI.arg(idStr) : SCHEME_TEMPLATE_STD.arg(idStr);
        logEvent(LOG_EVENT_URL.arg(shareUrl));

        // Add URL to clipboard
        postDirective<DClipboardUpdate>(shareUrl);

        // Enqueue message task
        TMessage* urlMsg = new TMessage(mCore);
        urlMsg->setStage(Task::Stage::Primary);
        urlMsg->setText(MSG_GENERATED_URL.arg(shareUrl));
        urlMsg->setSelectable(true);
        mCore.enqueueSingleTask(urlMsg);
    }

    // Return success
    return Qx::Error();
}
