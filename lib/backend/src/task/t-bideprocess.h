#ifndef TBIDEPROCESS_H
#define TBIDEPROCESS_H

// Qx Includes
#include <qx/core/qx-processbider.h>

// Project Includes
#include "task/task.h"

class QX_ERROR_TYPE(TBideProcessError, "TBideError", 1256)
{
    friend class TBideProcess;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError,
        BideFail,
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {BideFail, u"Could not bide on process."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mProcessName;

//-Constructor-------------------------------------------------------------
private:
    TBideProcessError(const QString& pn = {}, Type t = NoError);

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString processName() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class TBideProcess : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TBideProcess"_s;

    // Logging
    static inline const QString LOG_EVENT_BIDE_GRACE = u"Waiting %1 seconds for process %2 to be running"_s;
    static inline const QString LOG_EVENT_BIDE_RUNNING = u"Wait-on process %1 is running"_s;
    static inline const QString LOG_EVENT_BIDE_ON = u"Waiting for process %1 to finish"_s;
    static inline const QString LOG_EVENT_BIDE_QUIT = u"Wait-on process %1 has finished"_s;
    static inline const QString LOG_EVENT_BIDE_FINISHED = u"Wait-on process %1 was not running after the grace period"_s;
    static inline const QString LOG_EVENT_STOPPING_BIDE_PROCESS = u"Stopping current bide process..."_s;

    // Errors
    static inline const QString ERR_CANT_CLOSE_BIDE_PROCESS = u"Could not automatically end the running title! It will have to be closed manually."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    Qx::ProcessBider mProcessBider;

    // Data
    QString mProcessName;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TBideProcess(Core& core);

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
    void postBide(Qx::ProcessBider::ResultType type);
};

#endif // TBIDEPROCESS_H
