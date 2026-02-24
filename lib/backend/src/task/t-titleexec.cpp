// Unit Include
#include "t-titleexec.h"

// libfp Includes
#include <fp/fp-install.h>

// Project Includes
#include "kernel/core.h"

//===============================================================================================================
// TTitleExecError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TTitleExecError::TTitleExecError(const QString& pn, Type t) :
    mType(t),
    mProcessName(pn)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TTitleExecError::isValid() const { return mType != NoError; }
TTitleExecError::Type TTitleExecError::type() const { return mType; }
QString TTitleExecError::processName() const { return mProcessName; }

//Private:
Qx::Severity TTitleExecError::deriveSeverity() const { return Qx::Critical; }
quint32 TTitleExecError::deriveValue() const { return mType; }
QString TTitleExecError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TTitleExecError::deriveSecondary() const { return mProcessName; }

//===============================================================================================================
// TTitleExec
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
TTitleExec::TTitleExec(Core& core) :
    TExec(core),
    mBider(nullptr)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
void TTitleExec::complete(const Qx::Error& errorState)
{
    if(errorState || !mBider)
    {
        cleanup(errorState);
        return;
    }

#if _WIN32
    startBide();
#else
    qFatal("TTitleExec is improperly routed!");
#endif
}

void TTitleExec::cleanup(const Qx::Error& errorState)
{
    Task::complete(errorState);
}

//Public:
QString TTitleExec::name() const { return NAME; }
QStringList TTitleExec::members() const
{
    QStringList ml = TExec::members();
    return ml;
}

void TTitleExec::perform()
{
    logEvent(LOG_EVENT_RUNNING_TITLE);

#ifdef _WIN32
    if(auto err = setupBide(); err)
    {
        complete(err);
        return;
    }
#endif

    TExec::perform();
}

void TTitleExec::stop()
{
    TExec::stop();
    if(mBider && mBider->isBiding())
    {
        logEvent(LOG_EVENT_STOPPING_BIDE_PROCESS);
        mBider->closeProcess();
    }
}
