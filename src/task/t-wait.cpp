// Unit Include
#include "t-wait.h"

//===============================================================================================================
// TWait
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TWait::TWait() :
    mProcessWaiter(nullptr, STANDARD_GRACE)
{
    // Setup waiter
    connect(&mProcessWaiter, &ProcessWaiter::statusChanged, this,  [this](QString statusMessage){
        emit eventOccurred(NAME, statusMessage);
    });
    connect(&mProcessWaiter, &ProcessWaiter::errorOccured, this, [this](Qx::GenericError errorMessage){
        emit errorOccurred(NAME, errorMessage);
    });
    connect(&mProcessWaiter, &ProcessWaiter::waitFinished, this, &TWait::postWait);
}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TWait::name() const { return NAME; }
QStringList TWait::members() const
{
    QStringList ml = Task::members();
    ml.append(".processName() = \"" + mProcessName + "\"");
    return ml;
}

QString TWait::processName() const { return mProcessName; }

void TWait::setProcessName(QString processName) { mProcessName = processName; }

void TWait::perform()
{
    // Start wait
    mProcessWaiter.start(mProcessName);
}

void TWait::stop()
{
    if(mProcessWaiter.isRunning())
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_WAIT_PROCESS);
        if(!mProcessWaiter.closeProcess())
            emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Error, ERR_CANT_CLOSE_WAIT_ON));
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TWait::postWait(ErrorCode errorStatus)
{
    emit complete(errorStatus);
}
