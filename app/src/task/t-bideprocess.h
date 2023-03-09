#ifndef TBIDEPROCESS_H
#define TBIDEPROCESS_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Project Includes
#include "task/task.h"
#include "tools/processbider.h"

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
    void postBide(ErrorCode errorStatus);
};

#endif // TBIDEPROCESS_H
