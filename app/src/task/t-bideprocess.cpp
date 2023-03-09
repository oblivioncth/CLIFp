// Unit Include
#include "t-bideprocess.h"

//===============================================================================================================
// TBideProcess
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TBideProcess::TBideProcess(QObject* parent) :
    Task(parent),
    mProcessBider(nullptr, STANDARD_GRACE)
{
    // Setup bider
    connect(&mProcessBider, &ProcessBider::statusChanged, this,  [this](QString statusMessage){
        emit eventOccurred(NAME, statusMessage);
    });
    connect(&mProcessBider, &ProcessBider::errorOccured, this, [this](Qx::GenericError errorMessage){
        emit errorOccurred(NAME, errorMessage);
    });
    connect(&mProcessBider, &ProcessBider::bideFinished, this, &TBideProcess::postBide);
}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TBideProcess::name() const { return NAME; }
QStringList TBideProcess::members() const
{
    QStringList ml = Task::members();
    ml.append(".processName() = \"" + mProcessName + "\"");
    return ml;
}

QString TBideProcess::processName() const { return mProcessName; }

void TBideProcess::setProcessName(QString processName) { mProcessName = processName; }

void TBideProcess::perform()
{
    // Start bide
    mProcessBider.start(mProcessName);
}

void TBideProcess::stop()
{
    if(mProcessBider.isRunning())
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_BIDE_PROCESS);
        if(!mProcessBider.closeProcess())
            emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Error, ERR_CANT_CLOSE_BIDE_PROCESS));
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TBideProcess::postBide(ErrorCode errorStatus)
{
    emit complete(errorStatus);
}
