// Unit Include
#include "t-sleep.h"

//===============================================================================================================
// TSleep
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TSleep::TSleep(Core& core) :
    Task(core)
{
    // Setup timer
    mSleeper.setSingleShot(true);
    connect(&mSleeper, &QTimer::timeout, this, &TSleep::timerFinished);
}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TSleep::name() const { return NAME; }
QStringList TSleep::members() const
{
    QStringList ml = Task::members();
    ml.append(u".duration() = "_s + QString::number(mDuration));
    return ml;
}

uint TSleep::duration() const { return mDuration; }

void TSleep::setDuration(uint msecs) { mDuration = msecs; }

void TSleep::perform()
{
    logEvent(LOG_EVENT_START_SLEEP.arg(mDuration));
    mSleeper.start(mDuration);
}

void TSleep::stop()
{
    logEvent(LOG_EVENT_SLEEP_INTERUPTED);
    mSleeper.stop();
    complete(Qx::Error());
}

//-Signals & Slots------------------------------------------------------------------------------------------------------
//Privates Slots:
void TSleep::timerFinished()
{
    logEvent(LOG_EVENT_FINISH_SLEEP);
    complete(Qx::Error());
}
