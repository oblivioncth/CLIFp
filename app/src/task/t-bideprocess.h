#ifndef TBIDEPROCESS_H
#define TBIDEPROCESS_H

// Project Includes
#include "task/task.h"
#include "tools/processbider.h"

class TBideProcess : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TBideProcess"_s;

    // Logging
    static inline const QString LOG_EVENT_STOPPING_BIDE_PROCESS = u"Stopping current bide process..."_s;

    // Errors
    static inline const QString ERR_CANT_CLOSE_BIDE_PROCESS = u"Could not automatically end the running title! It will have to be closed manually."_s;

    // Functional
    static const uint STANDARD_GRACE = 2; // Seconds to allow the process to restart in cases it does

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    ProcessBider mProcessBider;

    // Data
    QString mProcessName;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TBideProcess(QObject* parent);

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
    void postBide(Qx::Error errorStatus);
};

#endif // TBIDEPROCESS_H
