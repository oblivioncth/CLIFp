// Unit Include
#include "c-share.h"

// Qx Includes
#include <qx/core/qx-system.h>

// Project Includes
#include "task/t-message.h"
#include "utility.h"

//===============================================================================================================
// CShareError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CShareError::CShareError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CShareError::isValid() const { return mType != NoError; }
QString CShareError::specific() const { return mSpecific; }
CShareError::Type CShareError::type() const { return mType; }

//Private:
Qx::Severity CShareError::deriveSeverity() const { return Qx::Critical; }
quint32 CShareError::deriveValue() const { return mType; }
QString CShareError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CShareError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CShare
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CShare::CShare(Core& coreRef) : TitleCommand(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
QList<const QCommandLineOption*> CShare::options() const { return CL_OPTIONS_SPECIFIC + TitleCommand::options(); }
QString CShare::name() const { return NAME; }

Qx::Error CShare::perform()
{
    // Prioritize scheme (un)registration
    if(mParser.isSet(CL_OPTION_CONFIGURE))
    {
        logEvent(LOG_EVENT_REGISTRATION);

        if(!Qx::setDefaultProtocolHandler(SCHEME, SCHEME_NAME))
        {
            CShareError err(CShareError::RegistrationFailed);
            postError(err);
            return err;
        }

        // Enqueue success message task
        TMessage* successMsg = new TMessage(&mCore);
        successMsg->setStage(Task::Stage::Primary);
        successMsg->setText(MSG_REGISTRATION_COMPLETE);
        mCore.enqueueSingleTask(successMsg);

        return CShareError();
    }
    else if(mParser.isSet(CL_OPTION_UNCONFIGURE))
    {
        logEvent(LOG_EVENT_UNREGISTRATION);

#ifdef __linux__ // Function is too jank on linux right now, so always fail/no-op there
        if(true)
#else
        if(!Qx::removeDefaultProtocolHandler(SCHEME, CLIFP_PATH))
#endif
        {
            CShareError err(CShareError::UnregistrationFailed);
            postError(err);
            return err;
        }

        // Enqueue success message task
        TMessage* successMsg = new TMessage(&mCore);
        successMsg->setStage(Task::Stage::Primary);
        successMsg->setText(MSG_UNREGISTRATION_COMPLETE);
        mCore.enqueueSingleTask(successMsg);

        return CShareError();
    }

    // Get ID of title to share
    QUuid shareId;
    if(Qx::Error ide = getTitleId(shareId); ide.isValid())
        return ide;

    mCore.setStatus(STATUS_SHARE, shareId.toString(QUuid::WithoutBraces));

    // Generate URL
    QString idStr = shareId.toString(QUuid::WithoutBraces);
    QString shareUrl = mParser.isSet(CL_OPTION_UNIVERSAL) ? SCHEME_TEMPLATE_UNI.arg(idStr) : SCHEME_TEMPLATE_STD.arg(idStr);
    logEvent(LOG_EVENT_URL.arg(shareUrl));

    // Add URL to clipboard
    mCore.requestClipboardUpdate(shareUrl);

    // Enqueue message task
    TMessage* urlMsg = new TMessage(&mCore);
    urlMsg->setStage(Task::Stage::Primary);
    urlMsg->setText(MSG_GENERATED_URL.arg(shareUrl));
    urlMsg->setSelectable(true);
    mCore.enqueueSingleTask(urlMsg);

    // Return success
    return Qx::Error();
}
