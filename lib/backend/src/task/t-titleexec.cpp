// Unit Include
#include "t-titleexec.h"

// Qx Includes


// Project Includes

//===============================================================================================================
// TTitleExec
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
TTitleExec::TTitleExec(Core& core) :
    TExec(core)
{}

//-Instance Functions-------------------------------------------------------------
//Private:

//Public:
QString TExec::name() const { return NAME; }
QStringList TExec::members() const
{
    QStringList ml = Task::members();
    ml.append(u".executable() = \""_s + mExecutable + u"\""_s);
    ml.append(u".directory() = \""_s + mDirectory.absolutePath() + u"\""_s);
    if(std::holds_alternative<QString>(mParameters))
        ml.append(u".parameters() = \""_s + std::get<QString>(mParameters) + u"\""_s);
    else
        ml.append(u".parameters() = {\""_s + std::get<QStringList>(mParameters).join(uR"(", ")"_s) + u"\"}"_s);
    ml.append(u".processType() = "_s + ENUM_NAME(mProcessType));
    ml.append(u".identifier() = \""_s + mIdentifier + u"\""_s);
    return ml;
}


void TExec::perform()
{
    logEvent(LOG_EVENT_PREPARING_PROCESS.arg(ENUM_NAME(mProcessType), mIdentifier, mExecutable));

    // Return success
    emit completed(TExecError());
}

void TExec::stop()
{
    ...
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void TExec::postBlockingProcess()
{

}
