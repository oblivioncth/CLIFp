// Unit Include
#include "t-awaitdocker.h"

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
TAwaitDocker::TAwaitDocker(QObject* parent) :
    Task(parent)
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

    // Setup check command
    QProcess dockerPs;
    dockerPs.setProgram(DOCKER);
    dockerPs.setArguments({
        "ps",
        "--filter",
        "status=running",
        "--filter",
        "name=" + mImageName,
        "--format",
        "{{.Names}}"
    });

    // Execute (wait by blocking since this should be really fast)
    dockerPs.start();
    if(!dockerPs.waitForStarted(1000))
    {
        TAwaitDockerError err(TAwaitDockerError::DirectQueryFailed);
        emit errorOccurred(NAME, err);
        return err;
    }
    if(!dockerPs.waitForFinished(1000))
    {
        dockerPs.kill(); // Force close
        dockerPs.waitForFinished();

        TAwaitDockerError err(TAwaitDockerError::DirectQueryFailed);
        emit errorOccurred(NAME, err);
        return err;
    }

    // Check result, should just contain image name due to filter/format options
    QString queryResult = QString::fromLatin1(dockerPs.readAllStandardOutput());
    queryResult.chop(1); // Remove '\n'
    running = queryResult == mImageName;

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
        emit errorOccurred(NAME, err);
        return err;
    }
    mTimeoutTimer.start(mTimeout);

    return TAwaitDockerError();
}

void TAwaitDocker::stopEventListening()
{
    emit eventOccurred(NAME, LOG_EVENT_STOPPING_LISTENER);

    // Just kill it, clean shutdown isn't needed
    mEventListener.close();
    mEventListener.waitForFinished();
}

//Public:
QString TAwaitDocker::name() const { return NAME; }
QStringList TAwaitDocker::members() const
{
    QStringList ml = Task::members();
    ml.append(".imageName() = \"" + mImageName + "\"");
    ml.append(".timeout() = " + QString::number(mTimeout));
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
    emit eventOccurred(NAME, LOG_EVENT_DIRECT_QUERY.arg(mImageName));

    bool running;
    errorStatus = imageRunningCheck(running);
    if(errorStatus.isValid() || running)
    {
        emit complete(errorStatus);
        return;
    }

    // Listen for image started event
    emit eventOccurred(NAME, LOG_EVENT_STARTING_LISTENER);
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
            emit eventOccurred(NAME, LOG_EVENT_START_RECEIVED);
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
        emit eventOccurred(NAME, LOG_EVENT_FINAL_CHECK_PASS);
        emit complete(TAwaitDockerError());
    }
    else
    {
        TAwaitDockerError err(TAwaitDockerError::StartFailed, mImageName);
        emit errorOccurred(NAME, err);
        emit complete(err);
    }
}
