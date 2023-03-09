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
    static inline const QString NAME = QSL("TSleep");

    // Logging
    static inline const QString LOG_EVENT_START_SLEEP = QSL("Sleeping for %1 milliseconds");
    static inline const QString LOG_EVENT_SLEEP_INTERUPTED = QSL("Sleep interrupted");
    static inline const QString LOG_EVENT_FINISH_SLEEP = QSL("Finished sleeping");

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
