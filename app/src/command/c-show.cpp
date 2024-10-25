// Unit Include
#include "c-show.h"

// Project Includes
#include "kernel/core.h"
#include "task/t-message.h"
#include "task/t-extra.h"

//===============================================================================================================
// CShowError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CShowError::CShowError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CShowError::isValid() const { return mType != NoError; }
QString CShowError::specific() const { return mSpecific; }
CShowError::Type CShowError::type() const { return mType; }

//Private:
Qx::Severity CShowError::deriveSeverity() const { return Qx::Critical; }
quint32 CShowError::deriveValue() const { return mType; }
QString CShowError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CShowError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CSHOW
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CShow::CShow(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
QList<const QCommandLineOption*> CShow::options() const { return CL_OPTIONS_SPECIFIC + Command::options(); }
QString CShow::name() const { return NAME; }

Qx::Error CShow::perform()
{
    // Enqueue show task
    if(mParser.isSet(CL_OPTION_MSG))
    {
        TMessage* messageTask = new TMessage(mCore);
        messageTask->setStage(Task::Stage::Primary);
        messageTask->setText(mParser.value(CL_OPTION_MSG));

        mCore.enqueueSingleTask(messageTask);
        mCore.setStatus(STATUS_SHOW_MSG, messageTask->text());
    }
    else if(mParser.isSet(CL_OPTION_EXTRA))
    {
        TExtra* extraTask = new TExtra(mCore);
        extraTask->setStage(Task::Stage::Primary);
        extraTask->setDirectory(QDir(mCore.fpInstall().extrasDirectory().absolutePath() + '/' + mParser.value(CL_OPTION_EXTRA)));

        mCore.enqueueSingleTask(extraTask);
        mCore.setStatus(STATUS_SHOW_EXTRA, extraTask->directory().dirName());
    }
    else
    {
        CShowError err(CShowError::MissingThing);
        postDirective<DError>(err);
        return err;
    }

    // Return success
    return CShowError();
}
