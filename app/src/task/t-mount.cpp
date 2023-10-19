// Unit Include
#include "t-mount.h"

// Qt Includes
#include <QDir>
#include <QRandomGenerator>

// Qx Includes
#include <qx/core/qx-base85.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// TMount
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TMount::TMount(QObject* parent) :
    Task(parent),
    mMounterProxy(nullptr),
    mMounterQmp(nullptr),
    mMounterRouter(nullptr),
    mMounting(false),
    mDaemon(Fp::FpProxy)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
template<typename M>
    requires Qx::any_of<M, MounterProxy, MounterQmp, MounterRouter>
void TMount::initMounter(M*& mounter)
{
    mounter = new M(this);

    connect(mounter, &M::eventOccurred, this, &Task::eventOccurred);
    connect(mounter, &M::errorOccurred, this, &Task::errorOccurred);
    connect(mounter, &M::mountFinished, this, &TMount::mounterFinishHandler);
}

//Public:
QString TMount::name() const { return NAME; }
QStringList TMount::members() const
{
    QStringList ml = Task::members();
    ml.append(u".daemon() = \""_s + ENUM_NAME(mDaemon) + u"\""_s);
    ml.append(u".titleId() = \""_s + mTitleId.toString() + u"\""_s);
    ml.append(u".path() = \""_s + QDir::toNativeSeparators(mPath) + u"\""_s);
    return ml;
}

QUuid TMount::titleId() const { return mTitleId; }
QString TMount::path() const { return mPath; }
Fp::Daemon TMount::daemon() const { return mDaemon; }

void TMount::setTitleId(QUuid titleId) { mTitleId = titleId; }
void TMount::setPath(QString path) { mPath = path; }
void TMount::setDaemon(Fp::Daemon daemon) { mDaemon = daemon; }

void TMount::perform()
{
    mMounting = true;

    // Log/label string
    QFileInfo packFileInfo(mPath);
    QString label = LOG_EVENT_MOUNTING_DATA_PACK.arg(packFileInfo.fileName());

    // Start mount
    emit longTaskStarted(label);

    // Update state
    emit longTaskProgressChanged(0);
    emit longTaskTotalChanged(0); // Cause busy state

    //-Setup Mounter(s)------------------------------------

    if(mDaemon == Fp::Daemon::FpProxy)
    {
        initMounter(mMounterProxy);
        mMounterProxy->setFilePath(mPath);
        mMounterProxy->setProxyServerPort(22501);
    }
    else
    {
        QString routerMountValue = QFileInfo(mPath).fileName();

        if(mDaemon == Fp::Daemon::Qemu)
        {
            // Generate random sequence of 16 lowercase alphabetical characters to act as Drive ID
            QByteArray alphaBytes;
            alphaBytes.resize(16);
            std::generate(alphaBytes.begin(), alphaBytes.end(), [](){
                // Funnel numbers into 0-25, use ASCI char 'a' (0x61) as a base value
                return (QRandomGenerator::global()->generate() % 26) + 0x61;
            });
            QString driveId = QString::fromLatin1(alphaBytes);

            // Convert UUID to 20 char drive serial by encoding to Base85
            Qx::Base85Encoding encoding(Qx::Base85Encoding::Btoa);
            encoding.resetZeroGroupCharacter(); // No shortcut characters
            QByteArray rawTitleId = mTitleId.toRfc4122(); // Binary representation of UUID
            Qx::Base85 driveSerialEnc = Qx::Base85::encode(rawTitleId, &encoding);
            QString driveSerial = driveSerialEnc.toString();

            initMounter(mMounterQmp);
            mMounterQmp->setDriveId(driveId);
            mMounterQmp->setDriveSerial(driveSerial);
            mMounterQmp->setFilePath(mPath);
            mMounterQmp->setQemuMountPort(22501);
            mMounterQmp->setQemuProdPort(0); // Unused

            routerMountValue = driveSerial;
        }

        initMounter(mMounterRouter);
        mMounterRouter->setMountValue(routerMountValue);
        mMounterRouter->setRouterPort(22500);
    }

    // Mount
    switch(mDaemon)
    {
        case Fp::Daemon::FpProxy:
            mMounterProxy->mount();
            break;

        case Fp::Daemon::Qemu:
            mMounterQmp->mount();
            break;

        case Fp::Daemon::Docker:
            mMounterRouter->mount();
            break;

        default:
            qCritical("Mount attempted with unknown daemon!");
            break;
    }

    // Await finished signal(s)...
}

void TMount::stop()
{
    if(mMounting)
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_MOUNT);

        // TODO: This could benefit from the mounters using a shared base, or
        // some other kind of type erasure like the duck typing above.
        if(mMounterProxy && mMounterProxy->isMounting())
            mMounterProxy->abort();
        if(mMounterQmp && mMounterQmp->isMounting())
            mMounterQmp->abort();
        if(mMounterRouter && mMounterRouter->isMounting())
            mMounterRouter->abort();
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TMount::mounterFinishHandler(Qx::Error err)
{
    if(sender() == mMounterQmp && !err.isValid())
        mMounterRouter->mount();
    else
        postMount(err);
}

void TMount::postMount(Qx::Error errorStatus)
{
    mMounting = false;

    // Reset mounter pointers ('this' will delete them  due to parenting so there's no leak)
    mMounterProxy = nullptr;
    mMounterQmp = nullptr;
    mMounterRouter = nullptr;

    // Handle result
    emit longTaskFinished();
    emit complete(errorStatus);
}
