// Unit Include
#include "t-bideprocess.h"

//===============================================================================================================
// TBideProcess
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TBideProcess::TBideProcess(QObject* parent) :
    Task(parent),
    mProcessBider(nullptr)
{
    // Setup bider
    mProcessBider.setRespawnGrace(STANDARD_GRACE);
    connect(&mProcessBider, &ProcessBider::statusChanged, this,  [this](QString statusMessage){
        emit eventOccurred(NAME, statusMessage);
    });
    connect(&mProcessBider, &ProcessBider::errorOccurred, this, [this](ProcessBiderError errorMessage){
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
    ml.append(u".processName() = \""_s + mProcessName + u"\""_s);
    return ml;
}

QString TBideProcess::processName() const { return mProcessName; }

void TBideProcess::setProcessName(QString processName) { mProcessName = processName; }

void TBideProcess::perform()
{
    // Start bide
    mProcessBider.setProcessName(mProcessName);
    mProcessBider.start();
}

void TBideProcess::stop()
{
    if(mProcessBider.isRunning())
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_BIDE_PROCESS);
        if(ProcessBiderError err = mProcessBider.closeProcess(); err.isValid())
            emit errorOccurred(NAME, err);
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TBideProcess::postBide(Qx::Error errorStatus)
{
    emit complete(errorStatus);
}
