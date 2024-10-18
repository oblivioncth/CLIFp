#ifndef DEFERREDPROCESSMANAGER_H
#define DEFERREDPROCESSMANAGER_H

// Qt Includes
#include <QObject>
#include <QProcess>
#include <QSet>

// Qx Includes
#include <qx/core/qx-genericerror.h>
#include <qx/utility/qx-macros.h>

// Project Includes
#include "kernel/directorate.h"

class Core;

class DeferredProcessManager : public QObject, public Directorate
{
    Q_OBJECT;
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"DeferredProcessManager"_s;

    // Log
    static inline const QString LOG_EVENT_PROCCESS_CLOSED = u"Deferred process '%1' ( %2 ) finished. Status: '%3', Code: %4"_s;
    static inline const QString LOG_EVENT_PROCCESS_STDOUT = u"'%1' ( %2 | %3 ) <stdout> %4"_s;
    static inline const QString LOG_EVENT_PROCCESS_STDERR = u"'%1' ( %2 | %3 ) <stderr> %4"_s;

    // Error
    static inline const QString ERR_PROCESS_END_PREMATURE = u"Deferred process '%1' ( %2 ) unexpectedly finished. Status: '%3', Code: %4"_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QSet<QProcess*> mManagedProcesses;
    bool mClosingClients;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    DeferredProcessManager(Core& core);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    QString name() const override;
    void handleProcessDataReceived(QProcess* process, const QString& msgTemplate);
    void handleProcessStdOutMessage(QProcess* process);
    void handleProcessStdErrMessage(QProcess* process);

public:
    void manage(const QString& identifier, QProcess* process);
    void closeProcesses();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void processFinishedHandler(int exitCode, QProcess::ExitStatus exitStatus);
    void processStandardOutHandler();
    void processStandardErrorHandler();
};

#endif // DEFERREDPROCESSMANAGER_H
