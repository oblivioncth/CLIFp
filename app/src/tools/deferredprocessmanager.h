#ifndef DEFERREDPROCESSMANAGER_H
#define DEFERREDPROCESSMANAGER_H

// Qt Includes
#include <QObject>
#include <QProcess>
#include <QSet>

// Qx Includes
#include <qx/core/qx-genericerror.h>
#include <qx/utility/qx-macros.h>

class DeferredProcessManager : public QObject
{
    Q_OBJECT;
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QSL("DeferredProcessManager");

    // Log
    static inline const QString LOG_EVENT_PROCCESS_CLOSED = QSL("Deferred process '%1' ( %2 ) finished. Status: '%3', Code: %4");
    static inline const QString LOG_EVENT_PROCCESS_STDOUT = QSL("'%1' ( %2 | %3 ) <stdout> %4");
    static inline const QString LOG_EVENT_PROCCESS_STDERR = QSL("'%1' ( %2 | %3 ) <stderr> %4");

    // Error
    static inline const QString ERR_PROCESS_END_PREMATURE = "Deferred process '%1' ( %2 ) unexpectedly finished. Status: '%3', Code: %4";

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QSet<QProcess*> mManagedProcesses;
    bool mClosingClients;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    DeferredProcessManager(QObject* parent);

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
    void eventOccurred(QString name, QString event);
    void errorOccurred(QString name, Qx::GenericError error);
};

#endif // DEFERREDPROCESSMANAGER_H
