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
    QString identifier = mManagedProcesses.value(process);
    QString program = process->program();
    QString pid = QString::number(process->processId());
    QString output = QString::fromLocal8Bit(process->readAll());

    // Don't print extra linebreak
    if(!output.isEmpty() && output.back() == '\n')
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

    // Store
    mManagedProcesses[process] = identifier;
}

void DeferredProcessManager::closeProcesses()
{
    if(!mClosingClients)
    {
        mClosingClients = true;

        /* Can't iterate over the hash here as when a process finishes 'processFinishedHandler' gets called
         * which removes the QProcess handle from the hash, potentially invalidating the iterator. Even if it
         * coincidentally works some of the time, this is undefined behavior and should be avoided. Instead
         * simply check if the hash is empty on each iteration
         */

        while(!mManagedProcesses.isEmpty())
        {
            // Get first key from hash
            QProcess* proc = mManagedProcesses.constBegin().key();

            /* Kill children of the process, as here the whole tree should be killed
             * A "clean" kill is used for this as the vanilia Launcher uses Node.js process.kill()
             * without a signal argument, which maps to SIGTERM (on linux), which is a clean kill.
             */
            QList<quint32> children = Qx::processChildren(proc->processId(), true);
            for(quint32 cPid : children)
                Qx::cleanKillProcess(cPid);

            // Kill the main process itself
            proc->terminate(); // Try nice closure first
            if(!(proc->waitForFinished(2000)))
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
    // Get process and its identifier, remove it from hash
    QProcess* process = qobject_cast<QProcess*>(sender());
    if(!process)
        throw std::runtime_error(std::string(Q_FUNC_INFO) + " a non-QProcess called this slot!");
    QString identifier = mManagedProcesses.value(process);
    mManagedProcesses.remove(process);

    // Flush incomplete messages
    process->setReadChannel(QProcess::StandardOutput);
    if(!process->atEnd())
        signalProcessStdOutMessage(process);
    process->setReadChannel(QProcess::StandardError);
    if(!process->atEnd())
        signalProcessStdErrMessage(process);

    // Assemble details
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
        signalError(Qx::GenericError(Qx::GenericError::Warning, ERR_PROCESS_END_PREMATURE.arg(identifier, program, status, code)));
}

void DeferredProcessManager::processStandardOutHandler()
{
    // Get process
    QProcess* process = qobject_cast<QProcess*>(sender());
    if(!process)
        throw std::runtime_error(std::string(Q_FUNC_INFO) + " a non-QProcess called this slot!");

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
        throw std::runtime_error(std::string(Q_FUNC_INFO) + " a non-QProcess called this slot!");

    // Signal data if complete message is in buffer
    process->setReadChannel(QProcess::StandardError);
    if(process->canReadLine())
        signalProcessStdErrMessage(process);
}
