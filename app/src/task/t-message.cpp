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
    ml.append(u".text() = \""_s + mText + u"\""_s);
    ml.append(u".isBlocking() = \""_s + QString(mBlocking ? u"true"_s : u"false"_s) + u"\""_s);
    ml.append(u".isSelectable() = \""_s + QString(mSelectable ? u"true"_s : u"false"_s) + u"\""_s);
    return ml;
}

QString TMessage::text() const { return mText; }
bool TMessage::isBlocking() const { return mBlocking; }
bool TMessage::isSelectable() const { return mSelectable; }

void TMessage::setText(const QString& text) { mText = text; }
void TMessage::setBlocking(bool block) { mBlocking = block; }
void TMessage::setSelectable(bool sel) { mSelectable = sel; }

void TMessage::perform()
{
    emit notificationReady(Message{
        .text = mText,
        .blocking = mBlocking,
        .selectable = mSelectable
    });
    emit eventOccurred(NAME, LOG_EVENT_SHOW_MESSAGE);

    // Return success
    emit complete(Qx::Error());
}
