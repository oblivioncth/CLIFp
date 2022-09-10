// Unit Include
#include "t-mount.h"

// Qt Includes
#include <QDir>

//===============================================================================================================
// TMount
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TMount::TMount(QObject* parent) :
    Task(parent),
    mMounter(22501, 0, 22500) // Prod port not used yet
{
    //-Setup Mounter------------------------------------
    connect(&mMounter, &Mounter::errorOccured, this, [this](Qx::GenericError errorMsg){
        emit errorOccurred(NAME, errorMsg);
    });
    connect(&mMounter, &Mounter::eventOccured, this, [this](QString event){
        emit eventOccurred(NAME, event);
    });
    connect(&mMounter, &Mounter::mountProgressMaximumChanged, this, &Task::longTaskTotalChanged);
    connect(&mMounter, &Mounter::mountProgress, this, &Task::longTaskProgressChanged);
    connect(&mMounter, &Mounter::mountFinished, this, &TMount::postMount);
}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TMount::name() const { return NAME; }
QStringList TMount::members() const
{
    QStringList ml = Task::members();
    ml.append(".titleId() = \"" + mTitleId.toString() + "\"");
    ml.append(".path() = \"" + QDir::toNativeSeparators(mPath) + "\"");
    return ml;
}

QUuid TMount::titleId() const { return mTitleId; }
QString TMount::path() const { return mPath; }

void TMount::setTitleId(QUuid titleId) { mTitleId = titleId; }
void TMount::setPath(QString path) { mPath = path; }

void TMount::perform()
{
    // Log/label string
    QFileInfo packFileInfo(mPath);
    QString label = LOG_EVENT_MOUNTING_DATA_PACK.arg(packFileInfo.fileName());

    // Start mount
    emit longTaskStarted(label);
    mMounter.mount(mTitleId, mPath);
}

void TMount::stop()
{
    if(mMounter.isMounting())
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_MOUNT);
        mMounter.abort();
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TMount::postMount(ErrorCode errorStatus)
{
    // Handle result
    emit longTaskFinished();
    emit complete(errorStatus);
}
