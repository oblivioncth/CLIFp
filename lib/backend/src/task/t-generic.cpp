// Unit Include
#include "t-generic.h"

//===============================================================================================================
// TGeneric
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TGeneric::TGeneric(Core& core) :
    Task(core)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TGeneric::name() const { return NAME; }
QStringList TGeneric::members() const
{
    QStringList ml = Task::members();
    ml.append(u".description() = \""_s + mDescription + u"\""_s);
    ml.append(u".action() = <FUNCTOR>"_s);
    return ml;
}

QString TGeneric::description() const { return mDescription; }
void TGeneric::setDescription(const QString& desc) { mDescription = desc; }
void TGeneric::setAction(std::function<Qx::Error()> action) { mAction = action; }

void TGeneric::perform()
{
    logEvent(LOG_EVENT_START_ACTION.arg(mDescription));

    Qx::Error err = mAction();
    if(err.isValid())
        postDirective<DError>(err);

    logEvent(LOG_EVENT_END_ACTION);
    emit complete(err);
}
