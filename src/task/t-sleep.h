#ifndef TSLEEP_H
#define TSLEEP_H

// Project Includes
#include "task/task.h"

// Qt Includes
#include <QTimer>

class TSleep : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TSleep");

    // Logging
    static inline const QString LOG_EVENT_START_SLEEP = "Sleeping for %1 milliseconds";
    static inline const QString LOG_EVENT_SLEEP_INTERUPTED = "Sleep interrupted";
    static inline const QString LOG_EVENT_FINISH_SLEEP = "Finished sleeping";

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    QTimer mSleeper;

    // Data
    uint mDuration;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TSleep(QObject* parent = nullptr);

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
