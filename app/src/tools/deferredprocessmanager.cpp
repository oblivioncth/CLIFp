// Unit Include
#include "deferredprocessmanager.h"

// Qx Includes
#include <qx/core/qx-system.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// DeferredProcessManager
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
DeferredProcessManager::DeferredProcessManager(QObject* parent) :
    QObject(parent),
    mClosingClients(false)
{}

//-Instance Functions-------------------------------------------------------------
//Private:
void DeferredProcessManager::signalEvent(const QString& event) { emit eventOccurred(NAME, event); }
void DeferredProcessManager::signalError(const Qx::GenericError& error) { emit errorOccurred(NAME, error); }

void DeferredProcessManager::signalProcessDataReceived(QProcess* process, const QString& msgTemplate)
{
    // Assemble details
    QString identifier = process->objectName();
    QString program = process->program();
    QString pid = QString::number(process->processId());
    QString output = QString::fromLocal8Bit(process->readAll());

    // Don't print extra line-break
    if(output.endsWith('\n'))
        output.chop(1);;
    if(output.endsWith('\r'))
        output.chop(1);

    // Signal data
    signalEvent(msgTemplate.arg(identifier, program, pid, output));
}

void DeferredProcessManager::signalProcessStdOutMessage(QProcess* process)
{
    signalProcessDataReceived(process, LOG_EVENT_PROCCESS_STDOUT);
}

void DeferredProcessManager::signalProcessStdErrMessage(QProcess* process)
{
    signalProcessDataReceived(process, LOG_EVENT_PROCCESS_STDERR);
}

// Public:
void DeferredProcessManager::manage(const QString& identifier, QProcess* process)
{
    // Connect signals
    connect(process, &QProcess::finished, this, &DeferredProcessManager::processFinishedHandler);
    connect(process, &QProcess::readyReadStandardOutput, this, &DeferredProcessManager::processStandardOutHandler);
    connect(process, &QProcess::readyReadStandardError, this, &DeferredProcessManager::processStandardErrorHandler);

    // Set process identifier
    process->setObjectName(identifier);

    // Store
    mManagedProcesses.insert(process);
}

void DeferredProcessManager::closeProcesses()
{
    if(!mClosingClients)
    {
        mClosingClients = true;

        /* Can't iterate over the set here as when a process finishes 'processFinishedHandler' gets called
         * which removes the QProcess handle from the hash, potentially invalidating the iterator. Even if it
         * coincidentally works some of the time, this is undefined behavior and should be avoided. Instead
         * simply check if the set is empty on each iteration
         */

        while(!mManagedProcesses.isEmpty())
        {
            // Get first process from set
            QProcess* proc = *mManagedProcesses.constBegin();

            /* Kill children of the process, as here the whole tree should be killed
             * A "clean" kill is used for this on Linux as the vanilla Launcher uses Node.js process.kill()
             * without a signal argument, which maps to SIGTERM (on Linux), which is a clean kill.
             *
             * However on Windows, its likely that the children are console processes that won't respond to a
             * clean kill, so here we opt for a force kill.
             */
            QList<quint32> children = Qx::processChildren(proc->processId(), true);
            for(quint32 cPid : children)
            {
#if defined __linux__
                Qx::cleanKillProcess(cPid);
#elif defined _WIN32
                Qx::forceKillProcess(cPid);
#endif
            }

            // Kill the main process itself
            proc->terminate(); // Try nice closure first
            if(!(proc->waitForFinished(800)))
            {
                proc->kill();
                proc->waitForFinished();
            }
        }
        mClosingClients = false;
    }
}

//-Signals & Slots-------------------------------------------------------------------------------------------------------
//Private Slots:
void DeferredProcessManager::processFinishedHandler(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Get calling process
    QProcess* process = qobject_cast<QProcess*>(sender());
    if(!process)
        qFatal("A non-QProcess called this slot!");

    // Remove from managed set
    mManagedProcesses.remove(process);

    // Flush incomplete messages
    process->setReadChannel(QProcess::StandardOutput);
    if(!process->atEnd())
        signalProcessStdOutMessage(process);
    process->setReadChannel(QProcess::StandardError);
    if(!process->atEnd())
        signalProcessStdErrMessage(process);

    // Assemble details
    QString identifier = process->objectName();
    QString program = process->program();
    QString status = ENUM_NAME(exitStatus);
    QString code = QString::number(exitCode);

    // Schedule handle for deletion
    process->close(); // Wipe IO buffers for posterity
    process->deleteLater();

    // Emit message based on whether the process was expected to be closed
    if(mClosingClients)
        signalEvent(LOG_EVENT_PROCCESS_CLOSED.arg(identifier, program, status, code));
    else
        signalError(Qx::GenericError(Qx::Warning, 12311, ERR_PROCESS_END_PREMATURE.arg(identifier, program, status, code)));
}

void DeferredProcessManager::processStandardOutHandler()
{
    // Get process
    QProcess* process = qobject_cast<QProcess*>(sender());
    if(!process)
        qFatal("A non-QProcess called this slot!");

    // Signal data if complete message is in buffer
    process->setReadChannel(QProcess::StandardOutput);
    if(process->canReadLine())
        signalProcessStdOutMessage(process);
}

void DeferredProcessManager::processStandardErrorHandler()
{
    // Get process
    QProcess* process = qobject_cast<QProcess*>(sender());
    if(!process)
        qFatal("A non-QProcess called this slot!");

    // Signal data if complete message is in buffer
    process->setReadChannel(QProcess::StandardError);
    if(process->canReadLine())
        signalProcessStdErrMessage(process);
}
