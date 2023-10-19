#ifndef TMESSAGE_H
#define TMESSAGE_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

class TMessage : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TMessage"_s;

    // Logging
    static inline const QString LOG_EVENT_SHOW_MESSAGE = u"Displayed message"_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QString mText;
    bool mBlocking;
    bool mSelectable;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TMessage(QObject* parent);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    QString text() const;
    bool isBlocking() const;
    bool isSelectable() const;

    void setText(const QString& txt);
    void setBlocking(bool block);
    void setSelectable(bool sel);

    void perform() override;
};

#endif // TMESSAGE_H
