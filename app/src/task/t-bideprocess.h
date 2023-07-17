#ifndef TBIDEPROCESS_H
#define TBIDEPROCESS_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"
#include "tools/processbider.h"

class QX_ERROR_TYPE(TBideProcessError, "TBideProcessError", 1251)
{
    friend class TBideProcess;
    //-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        CantClose = 1,
    };

    //-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, QSL("")},
        {CantClose, QSL("Could not automatically end the running title! It will have to be closed manually.")},
    };

    //-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

    //-Constructor-------------------------------------------------------------
private:
    TBideProcessError(Type t = NoError, const QString& s = {});

    //-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

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
    static inline const QString NAME = QSL("TBideProcess");

    // Logging
    static inline const QString LOG_EVENT_STOPPING_BIDE_PROCESS = QSL("Stopping current bide process...");

    // Errors
    static inline const QString ERR_CANT_CLOSE_BIDE_PROCESS = QSL("Could not automatically end the running title! It will have to be closed manually.");

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
    TBideProcess(QObject* parent = nullptr);

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
