// Unit Include
#include "t-awaitdocker.h"

// Qt Includes

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
    mEventListener.setArguments({
        "events",
        "--filter",
        "container=" + mImageName,
        "--filter",
        "event=start",
        "--format",
        "{{.Actor.Attributes.name}}",
    });

    connect(&mEventListener, &QProcess::readyReadStandardOutput, this, &TAwaitDocker::eventDataReceived);

    mTimeoutTimer.setSingleShot(true);
    connect(&mTimeoutTimer, &QTimer::timeout, this, &TAwaitDocker::timeoutOccurred);
}

//-Instance Functions-------------------------------------------------------------
//Private:
ErrorCode TAwaitDocker::imageRunningCheck(bool& running)
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
        emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_DIRECT_QUERY));
        return ErrorCode::CANT_QUERY_DOCKER;
    }
    if(!dockerPs.waitForFinished(1000))
    {
        dockerPs.kill(); // Force close
        dockerPs.waitForFinished();
        emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_DIRECT_QUERY));
        return ErrorCode::CANT_QUERY_DOCKER;
    }

    // Check result, should just contain image name due to filter/format options
    QString queryResult = QString::fromLatin1(dockerPs.readAllStandardOutput());
    queryResult.chop(1); // Remove '\n'
    running = queryResult == mImageName;

    return ErrorCode::NO_ERR;
}

ErrorCode TAwaitDocker::startEventListener()
{
    mEventListener.start();
    if(!mEventListener.waitForStarted(1000))
    {
        emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_CANT_LISTEN));
        return ErrorCode::CANT_LISTEN_DOCKER;
    }
    mTimeoutTimer.start(mTimeout);

    return ErrorCode::NO_ERR;
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
    ErrorCode errorStatus = ErrorCode::NO_ERR;

    // Check if image is running
    emit eventOccurred(NAME, LOG_EVENT_DIRECT_QUERY.arg(mImageName));

    bool running;
    errorStatus = imageRunningCheck(running);
    if(errorStatus || running)
    {
        emit complete(errorStatus);
        return;
    }

    // Listen for image started event
    emit eventOccurred(NAME, LOG_EVENT_STARTING_LISTENER);
    errorStatus = startEventListener();
    if(errorStatus)
        emit complete(errorStatus);

    // Await event...
}

void TAwaitDocker::stop()
{
    if(mEventListener.state() != QProcess::NotRunning)
    {
        mTimeoutTimer.stop();
        stopEventListening();
        emit complete(ErrorCode::DOCKER_DIDNT_START);
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
            emit eventOccurred(NAME, LOG_EVENT_START_RECEIVED);
            mTimeoutTimer.stop();
            stopEventListening();
            emit complete(ErrorCode::NO_ERR);
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
        emit complete(ErrorCode::NO_ERR);
    }
    else
    {
        emit errorOccurred(NAME, Qx::GenericError(Qx::GenericError::Critical, ERR_DOCKER_DIDNT_START.arg(mImageName)));
        emit complete(ErrorCode::DOCKER_DIDNT_START);
    }
}
