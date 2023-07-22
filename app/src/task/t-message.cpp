// Unit Include
#include "t-message.h"

//===============================================================================================================
// TMessage
//===============================================================================================================

//-Constructor--------------------------------------------------------------------
//Public:
TMessage::TMessage(QObject* parent) :
    Task(parent)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
QString TMessage::name() const { return NAME; }
QStringList TMessage::members() const
{
    QStringList ml = Task::members();
    ml.append(u".message() = \""_s + mMessage + u"\""_s);
    ml.append(u".isModal() = \""_s + QString(mModal ? u"true"_s : u"false"_s) + u"\""_s);
    return ml;
}

QString TMessage::message() const { return mMessage; }
bool TMessage::isModal() const { return mModal; }

void TMessage::setMessage(QString message) { mMessage = message; }
void TMessage::setModal(bool modal) { mModal = modal; }

void TMessage::perform()
{
    emit notificationReady(mMessage);
    emit eventOccurred(NAME, LOG_EVENT_SHOW_MESSAGE);

    // Return success
    emit complete(Qx::Error());
}
