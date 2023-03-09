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
    static inline const QString NAME = QSL("TMessage");

    // Logging
    static inline const QString LOG_EVENT_SHOW_MESSAGE = QSL("Displayed message");

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Data
    QString mMessage;
    bool mModal;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TMessage(QObject* parent = nullptr);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    QString message() const;
    bool isModal() const;

    void setMessage(QString message);
    void setModal(bool modal);

    void perform() override;
};

#endif // TMESSAGE_H