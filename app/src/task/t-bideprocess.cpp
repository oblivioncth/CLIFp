// Unit Include
#include "t-bideprocess.h"

//===============================================================================================================
// TBideProcessError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
TBideProcessError::TBideProcessError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool TBideProcessError::isValid() const { return mType != NoError; }
QString TBideProcessError::specific() const { return mSpecific; }
TBideProcessError::Type TBideProcessError::type() const { return mType; }

//Private:
Qx::Severity TBideProcessError::deriveSeverity() const { return Qx::Err; }
quint32 TBideProcessError::deriveValue() const { return mType; }
QString TBideProcessError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString TBideProcessError::deriveSecondary() const { return mSpecific; }

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
    mProcessBider.start(mProcessName);
}

void TBideProcess::stop()
{
    if(mProcessBider.isRunning())
    {
        emit eventOccurred(NAME, LOG_EVENT_STOPPING_BIDE_PROCESS);
        if(!mProcessBider.closeProcess())
            emit errorOccurred(NAME, TBideProcessError(TBideProcessError::CantClose));
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TBideProcess::postBide(Qx::Error errorStatus)
{
    emit complete(errorStatus);
}
