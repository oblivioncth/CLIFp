#ifndef BLOCKINGPROCESSMANAGER_H
#define BLOCKINGPROCESSMANAGER_H

// Qt Includes
#include <QObject>
#include <QProcess>

// Qx Includes
#include <qx/core/qx-genericerror.h>
#include <qx/utility/qx-macros.h>

class BlockingProcessManager : public QObject
{
    Q_OBJECT;
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"BlockingProcessManager"_s;

    // Log
    static inline const QString LOG_EVENT_PROCCESS_CLOSED = u"Blocking process '%1' ( %2 ) finished. Status: '%3', Code: %4"_s;
    static inline const QString LOG_EVENT_PROCCESS_STDOUT = u"( %1 ) <stdout> %2"_s;
    static inline const QString LOG_EVENT_PROCCESS_STDERR = u"( %1 ) <stderr> %2"_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QProcess* mProcess;
    QString mIdentifier;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    BlockingProcessManager(QProcess* process, const QString& identifier, QObject* parent);

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
