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
    mMounter()
{
    // Connect mounter signals
    connect(&mMounter, &Mounter::errorOccured, this, [this](MounterError errorMsg){
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
    ml.append(u".titleId() = \""_s + mTitleId.toString() + u"\""_s);
    ml.append(u".path() = \""_s + QDir::toNativeSeparators(mPath) + u"\""_s);
    return ml;
}

bool TMount::isSkipQemu() const { return mSkipQemu; }
QUuid TMount::titleId() const { return mTitleId; }
QString TMount::path() const { return mPath; }

void TMount::setSkipQemu(bool skip) { mSkipQemu = skip; }
void TMount::setTitleId(QUuid titleId) { mTitleId = titleId; }
void TMount::setPath(QString path) { mPath = path; }

void TMount::perform()
{
    // Log/label string
    QFileInfo packFileInfo(mPath);
    QString label = LOG_EVENT_MOUNTING_DATA_PACK.arg(packFileInfo.fileName());

    //-Setup Mounter------------------------------------
    mMounter.setWebServerPort(22500);
    mMounter.setQemuMountPort(22501);
    mMounter.setQemuProdPort(0);
    mMounter.setQemuEnabled(!mSkipQemu);

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
void TMount::postMount(MounterError errorStatus)
{
    // Handle result
    emit longTaskFinished();
    emit complete(errorStatus);
}
