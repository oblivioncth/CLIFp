// Unit Include
#include "t-awaitdocker.h"

// Qx Includes
#include <qx/core/qx-system.h>

//===============================================================================================================
// TAwaitDockerError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TAwaitDockerError::TAwaitDockerError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TAwaitDockerError::isValid() const { return mType != NoError; }
QString TAwaitDockerError::specific() const { return mSpecific; }
TAwaitDockerError::Type TAwaitDockerError::type() const { return mType; }

//Private:
Qx::Severity TAwaitDockerError::deriveSeverity() const { return Qx::Critical; }
quint32 TAwaitDockerError::deriveValue() const { return mType; }
QString TAwaitDockerError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TAwaitDockerError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// TAwaitDocker
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TAwaitDocker::TAwaitDocker(Core& core) :
    Task(core)
{
    // Setup event listener
    mEventListener.setProgram(DOCKER);

    connect(&mEventListener, &QProcess::readyReadStandardOutput, this, &TAwaitDocker::eventDataReceived);

    mTimeoutTimer.setSingleShot(true);
    connect(&mTimeoutTimer, &QTimer::timeout, this, &TAwaitDocker::timeoutOccurred);
}

//-Instance Functions-------------------------------------------------------------
//Private:
TAwaitDockerError TAwaitDocker::imageRunningCheck(bool& running)
{
    // Directly check if the gamezip docker image is running
    running = false; // Default to no

    Qx::ExecuteResult res = Qx::execute(DOCKER,{
        "ps",
        "--filter",
        "status=running",
        "--filter",
        "name=" + mImageName,
        "--format",
        "{{.Names}}"
    }, 1000);

    if(res.exitCode != 0)
    {
        TAwaitDockerError err(TAwaitDockerError::DirectQueryFailed);
        postDirective<DError>(err);
        return err;
    }

    // Check result, should just contain image name due to filter/format options
    running = res.output == mImageName;

    return TAwaitDockerError();
}

TAwaitDockerError TAwaitDocker::startEventListener()
{
    // Arguments set here instead of ctor. since they depend on mImageName
    mEventListener.setArguments({
        "events",
        "--filter",
        "container=" + mImageName,
        "--filter",
        "event=start",
        "--format",
        "{{.Actor.Attributes.name}}",
    });

    mEventListener.start();
    if(!mEventListener.waitForStarted(1000))
    {
        TAwaitDockerError err(TAwaitDockerError::ListenFailed);
        postDirective<DError>(err);
        return err;
    }
    mTimeoutTimer.start(mTimeout);

    return TAwaitDockerError();
}

void TAwaitDocker::stopEventListening()
{
    logEvent(LOG_EVENT_STOPPING_LISTENER);

    // Just kill it, clean shutdown isn't needed
    mEventListener.close();
    mEventListener.waitForFinished();
}

//Public:
QString TAwaitDocker::name() const { return NAME; }
QStringList TAwaitDocker::members() const
{
    QStringList ml = Task::members();
    ml.append(u".imageName() = \""_s + mImageName + u"\""_s);
    ml.append(u".timeout() = "_s + QString::number(mTimeout));
    return ml;
}

QString TAwaitDocker::imageName() const { return mImageName; }
uint TAwaitDocker::timeout() const { return mTimeout; };

void TAwaitDocker::setImageName(const QString& imageName) { mImageName = imageName; }
void TAwaitDocker::setTimeout(uint msecs) { mTimeout = msecs; }

void TAwaitDocker::perform()
{
    // Error tracking
    TAwaitDockerError errorStatus;

    // Check if image is running
    logEvent(LOG_EVENT_DIRECT_QUERY.arg(mImageName));

    bool running;
    errorStatus = imageRunningCheck(running);
    if(errorStatus.isValid() || running)
    {
        emit complete(errorStatus);
        return;
    }

    // Listen for image started event
    logEvent(LOG_EVENT_STARTING_LISTENER);
    errorStatus = startEventListener();
    if(errorStatus.isValid())
        emit complete(errorStatus);

    // Await event...
}

void TAwaitDocker::stop()
{
    if(mEventListener.state() != QProcess::NotRunning)
    {
        mTimeoutTimer.stop();
        stopEventListening();

        TAwaitDockerError err(TAwaitDockerError::StartFailed, mImageName);
        emit complete(err);
    }
}

//-Signals & Slots------------------------------------------------------------------------------------------------------
//Private Slots:
void TAwaitDocker::eventDataReceived()
{
    // Read when full message has been received
    if(mEventListener.canReadLine())
    {
        // Check event, should just contain image name due to filter/format options
        QString eventData = QString::fromLatin1(mEventListener.readLine());
        eventData.chop(1); // Remove '\n'
        if(eventData == mImageName)
        {
            mTimeoutTimer.stop();
            logEvent(LOG_EVENT_START_RECEIVED);
            stopEventListening();
            emit complete(TAwaitDockerError());
        }
    }
}

void TAwaitDocker::timeoutOccurred()
{
    stopEventListening();

    // Check one last time directly in case the listener was starting up while the image started
    bool running;
    imageRunningCheck(running); // If this errors here, just assume that the image didn't start

    if(running)
    {
        logEvent(LOG_EVENT_FINAL_CHECK_PASS);
        emit complete(TAwaitDockerError());
    }
    else
    {
        TAwaitDockerError err(TAwaitDockerError::StartFailed, mImageName);
        postDirective<DError>(err);
        emit complete(err);
    }
}
