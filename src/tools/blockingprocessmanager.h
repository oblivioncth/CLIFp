#ifndef BLOCKINGPROCESSMANAGER_H
#define BLOCKINGPROCESSMANAGER_H

// Qt Includes
#include <QObject>
#include <QProcess>

// Qx Includes
#include <qx/core/qx-genericerror.h>

class BlockingProcessManager : public QObject
{
    Q_OBJECT;
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("BlockingProcessManager");

    // Log
    static inline const QString LOG_EVENT_PROCCESS_CLOSED = "Blocking process '%1' ( %2 ) finished. Status: '%3', Code: %4";
    static inline const QString LOG_EVENT_PROCCESS_STDOUT = "( %1 ) stdout: %2";
    static inline const QString LOG_EVENT_PROCCESS_STDERR = "( %1 ) stderr: %2";

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QProcess* mProcess;
    QString mIdentifier;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    BlockingProcessManager(QProcess* process, const QString& identifier, QObject* parent = nullptr);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    void signalEvent(const QString& event);
    void signalProcessDataReceived(const QString& msgTemplate);

public:
    void closeProcess();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void processFinishedHandler(int exitCode, QProcess::ExitStatus exitStatus);
    void processStandardOutHandler();
    void processStandardErrorHandler();

signals:
    void eventOccurred(QString name, QString event);
    void finished();
};

#endif // DEFERREDPROCESSMANAGER_H
