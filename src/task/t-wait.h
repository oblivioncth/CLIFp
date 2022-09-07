#ifndef TWAIT_H
#define TWAIT_H

// Project Includes
#include "../task.h"
#include "../processwaiter.h"

class TWait : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TWait");

    // Logging
    static inline const QString LOG_EVENT_STOPPING_WAIT_PROCESS = "Stopping current wait on execution process...";

    // Errors
    static inline const QString ERR_CANT_CLOSE_WAIT_ON = "Could not automatically end the running title! It will have to be closed manually.";

    // Functional
    static const uint STANDARD_GRACE = 2; // Seconds to allow the process to restart in cases it does

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    ProcessWaiter mProcessWaiter;

    // Data
    QString mProcessName;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TWait();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    QString processName() const;

    void setProcessName(QString processName);

    void perform() override;
    void stop() override;

//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postWait(ErrorCode errorStatus);
};

#endif // TWAIT_H
