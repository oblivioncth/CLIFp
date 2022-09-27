// Unit Include
#include "blockingprocessmanager.h"

// Project Includes
#include "utility.h"

//===============================================================================================================
// BlockingProcessManager
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
BlockingProcessManager::BlockingProcessManager(QProcess* process, const QString& identifier, QObject* parent) :
    QObject(parent),
    mProcess(process),
    mIdentifier(identifier)
{
    // Adopt process
    mProcess->setParent(this);

    // Connect handlers
    connect(mProcess, &QProcess::readyReadStandardOutput, this, &BlockingProcessManager::processStandardOutHandler);
    connect(mProcess, &QProcess::readyReadStandardOutput, this, &BlockingProcessManager::processStandardOutHandler);
    connect(mProcess, &QProcess::finished, this, &BlockingProcessManager::processFinishedHandler);
}

//-Instance Functions-------------------------------------------------------------
//Private:
void BlockingProcessManager::signalEvent(const QString& event) { emit eventOccurred(NAME, event); }

void BlockingProcessManager::signalProcessDataReceived(const QString& msgTemplate)
{
    // Assemble details
    QString pid = QString::number(mProcess->processId());
    QString output = QString::fromLocal8Bit(mProcess->readAll());

    // Don't print extra linebreak
    if(output.endsWith('\n'))
        output.chop(1);;
    if(output.endsWith('\r'))
        output.chop(1);

    // Signal data
    signalEvent(msgTemplate.arg(pid, output));
}

//Public:
void BlockingProcessManager::closeProcess()
{
    // Try soft kill
    mProcess->terminate();
    bool closed = mProcess->waitForFinished(2000); // Allow up to 2 seconds to close
    /* NOTE: Initially there was concern that the above has a race condition in that the app could
     * finish closing after terminate was invoked, but before the portion of waitForFinished
     * that checks if the process is still running (since it returns false if it isn't) is reached.
     * This would make the return value misleading (it would appear as if the process is still running
     * (I think this behavior of waitForFinished is silly but that's an issue for another time); however,
     * debug testing showed that after sitting at a break point before waitForFinished, but after terminate
     * for a significant amount of time and then executing waitForFinished it still initially treated the
     * process as if it was running. Likely QProcess isn't made aware that the app stopped until some kind
     * of event is processed after it ends that is processed within waitForFinished, meaning there is no
     * race condition
     */

    // Hard kill if necessary
    if(!closed)
    {
        mProcess->kill(); // Hard kill
        mProcess->waitForFinished();
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void BlockingProcessManager::processFinishedHandler(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Flush incomplete messages
    mProcess->setReadChannel(QProcess::StandardOutput);
    if(!mProcess->atEnd())
        signalProcessDataReceived(LOG_EVENT_PROCCESS_STDOUT);
    mProcess->setReadChannel(QProcess::StandardError);
    if(!mProcess->atEnd())
        signalProcessDataReceived(LOG_EVENT_PROCCESS_STDERR);

    // Assemble details
    QString program = mProcess->program();
    QString status = ENUM_NAME(exitStatus);
    QString code = QString::number(exitCode);

    // Process will get deleted when the manager is destroyed
    mProcess->close(); // Wipe IO buffers for posterity

    // Notify process end
    signalEvent(LOG_EVENT_PROCCESS_CLOSED.arg(mIdentifier, program, status, code));

    // Notify management completion
    emit finished();
}

void BlockingProcessManager::processStandardOutHandler()
{
    // Signal data if complete message is in buffer
    mProcess->setReadChannel(QProcess::StandardOutput);
    if(mProcess->canReadLine())
        signalProcessDataReceived(LOG_EVENT_PROCCESS_STDOUT);
}

void BlockingProcessManager::processStandardErrorHandler()
{
    // Signal data if complete message is in buffer
    mProcess->setReadChannel(QProcess::StandardError);
    if(mProcess->canReadLine())
        signalProcessDataReceived(LOG_EVENT_PROCCESS_STDERR);
}
