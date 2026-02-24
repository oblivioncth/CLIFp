#ifndef T_TITLEEXEC_H
#define T_TITLEEXEC_H

// Qt Includes
#include <QUuid>

// Project Includes
#include "task/t-exec.h"

namespace Qx { class ProcessBider; }

class QX_ERROR_TYPE(TTitleExecError, "TTitleExecError", 1256)
{
    friend class TTitleExec;
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
    TTitleExecError(const QString& pn = {}, Type t = NoError);

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

class TTitleExec : public TExec
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TTitleExec"_s;

    // Logging
    static inline const QString LOG_EVENT_RUNNING_TITLE = u"Starting main title process."_s;

    // Logging Bide
    static inline const QString LOG_EVENT_CHECKING_FOR_BIDE = u"Checking if main title process needs a bide..."_s;
    static inline const QString LOG_EVENT_BIDE_DETERMINED = u"Main title process %1 need bide..."_s;
    static inline const QString LOG_EVENT_BIDE_START = u"Beginning bide on main title process..."_s;
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
    Qx::ProcessBider* mBider;

    // Data

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TTitleExec(Core& core);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
#if _WIN32
    Qx::Error setupBide();
    void startBide();
#endif

    void complete(const Qx::Error& errorState) override;
    void cleanup(const Qx::Error& errorState);

public:
    // Member access
    QString name() const override;
    QStringList members() const override;

    // Run
    void perform() override;
    void stop() override;
};

#endif // T_TITLEEXEC_H
