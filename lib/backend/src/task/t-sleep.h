#ifndef TSLEEP_H
#define TSLEEP_H

// Qt Includes
#include <QTimer>

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"

class TSleep : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TSleep"_s;

    // Logging
    static inline const QString LOG_EVENT_START_SLEEP = u"Sleeping for %1 milliseconds"_s;
    static inline const QString LOG_EVENT_SLEEP_INTERUPTED = u"Sleep interrupted"_s;
    static inline const QString LOG_EVENT_FINISH_SLEEP = u"Finished sleeping"_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    QTimer mSleeper;

    // Data
    uint mDuration;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TSleep(Core& core);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    uint duration() const;

    void setDuration(uint msecs);

    void perform() override;
    void stop() override;

//-Signals & Slots------------------------------------------------------------------------------------------------------
private slots:
    void timerFinished();
};

#endif // TSLEEP_H
