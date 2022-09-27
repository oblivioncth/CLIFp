#ifndef DEFERREDPROCESSMANAGER_H
#define DEFERREDPROCESSMANAGER_H

// Qt Includes
#include <QObject>
#include <QProcess>

// Qx Includes
#include <qx/core/qx-genericerror.h>

class DeferredProcessManager : public QObject
{
    Q_OBJECT;
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("DeferredProcessManager");

    // Log
    static inline const QString LOG_EVENT_PROCCESS_CLOSED = "Deferred process '%1' ( %2 ) finished. Status: '%3', Code: %4";
    static inline const QString LOG_EVENT_PROCCESS_STDOUT = "'%1' ( %2 | %3 ) stdout: %4";
    static inline const QString LOG_EVENT_PROCCESS_STDERR = "'%1' ( %2 | %3 ) stderr: %4";

    // Error
    static inline const QString ERR_PROCESS_END_PREMATURE = "Deferred process '%1' ( %2 ) unexpectedly finished. Status: '%3', Code: %4";

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QHash<QProcess*, QString> mManagedProcesses;
    bool mClosingClients;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    DeferredProcessManager(QObject* parent = nullptr);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    void signalEvent(const QString& event);
    void signalError(const Qx::GenericError& error);
    void signalProcessDataReceived(QProcess* process, const QString& msgTemplate);
    void signalProcessStdOutMessage(QProcess* process);
    void signalProcessStdErrMessage(QProcess* process);

public:
    void manage(const QString& identifier, QProcess* process);
    void closeProcesses();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void processFinishedHandler(int exitCode, QProcess::ExitStatus exitStatus);
    void processStandardOutHandler();
    void processStandardErrorHandler();

signals:
    void eventOccurred(QString taskName, QString event);
    void errorOccurred(QString taskName, Qx::GenericError error);
};

#endif // DEFERREDPROCESSMANAGER_H
