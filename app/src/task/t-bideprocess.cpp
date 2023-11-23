// Unit Include
#include "t-bideprocess.h"

//===============================================================================================================
// TBideProcessError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TBideProcessError::TBideProcessError(const QString& pn, Type t) :
    mType(t),
    mProcessName(pn)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TBideProcessError::isValid() const { return mType != NoError; }
TBideProcessError::Type TBideProcessError::type() const { return mType; }
QString TBideProcessError::processName() const { return mProcessName; }

//Private:
Qx::Severity TBideProcessError::deriveSeverity() const { return Qx::Critical; }
quint32 TBideProcessError::deriveValue() const { return mType; }
QString TBideProcessError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TBideProcessError::deriveSecondary() const { return mProcessName; }

//===============================================================================================================
// TBideProcess
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TBideProcess::TBideProcess(QObject* parent) :
    Task(parent)
{
    // Setup bider
    using namespace std::chrono_literals;
    static const auto grace = 2s;
    mProcessBider.setRespawnGrace(grace);
    mProcessBider.setInitialGrace(true); // Process will be stopped at first
    connect(&mProcessBider, &Qx::ProcessBider::established, this, [this]{
        emit eventOccurred(NAME, LOG_EVENT_BIDE_RUNNING.arg(mProcessName));
        emit eventOccurred(NAME, LOG_EVENT_BIDE_ON.arg(mProcessName));
    });
    connect(&mProcessBider, &Qx::ProcessBider::processStopped, this, [this]{
        emit eventOccurred(NAME, LOG_EVENT_BIDE_QUIT.arg(mProcessName));
    });
    connect(&mProcessBider, &Qx::ProcessBider::graceStarted, this, [this]{
        emit eventOccurred(NAME, LOG_EVENT_BIDE_GRACE.arg(QString::number(grace.count()), mProcessName));
    });
    connect(&mProcessBider, &Qx::ProcessBider::errorOccurred, this, [this](Qx::ProcessBiderError err){
        emit errorOccurred(NAME, err);
    });
    connect(&mProcessBider, &Qx::ProcessBider::finished, this, &TBideProcess::postBide);
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
    if(mProcessBider.isBiding())
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_BIDE_PROCESS);
        mProcessBider.closeProcess();
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TBideProcess::postBide(Qx::ProcessBider::ResultType type)
{
    if(type == Qx::ProcessBider::Fail)\
        emit complete(TBideProcessError(mProcessName, TBideProcessError::BideFail));
    else
    {
        emit eventOccurred(NAME, LOG_EVENT_BIDE_FINISHED.arg(mProcessName));
        emit complete(TBideProcessError());
    }
}
