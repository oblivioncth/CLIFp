// Unit Include
#include "t-generic.h"

//===============================================================================================================
// TGeneric
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TGeneric::TGeneric(QObject* parent) :
    Task(parent)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TGeneric::name() const { return NAME; }
QStringList TGeneric::members() const
{
    QStringList ml = Task::members();
    ml.append(".description() = " + mDescription);
    ml.append(".action() = <FUNCTOR>");
    return ml;
}

QString TGeneric::description() const { return mDescription; }
void TGeneric::setDescription(const QString& desc) { mDescription = desc; }
void TGeneric::setAction(std::function<Qx::Error()> action) { mAction = action; }

void TGeneric::perform()
{
    emit eventOccurred(NAME, LOG_EVENT_START_ACTION.arg(mDescription));

    Qx::Error err = mAction();
    if(err.isValid())
        emit errorOccurred(NAME, err);

    emit eventOccurred(NAME, LOG_EVENT_END_ACTION);
    emit complete(err);
}
