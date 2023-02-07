#ifndef TEXEC_H
#define TEXEC_H

// Project Includes
#include "task/task.h"
#include "tools/blockingprocessmanager.h"
#include "tools/deferredprocessmanager.h"

// Qt Includes
#include <QProcess>
#include <QFileInfo>
#include <QDir>

class TExec : public Task
{
    Q_OBJECT;
//-Class Enums-----------------------------------------------------------------------------------------------------
public:
    enum class ProcessType { Blocking, Deferred, Detached };

//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TExec");

    // Logging - Process Prep
    static inline const QString LOG_EVENT_PREPARING_PROCESS = "Preparing %1 process '%2' (%3)...";
    static inline const QString LOG_EVENT_FINAL_EXECUTABLE = "Final Executable: %1";
    static inline const QString LOG_EVENT_FINAL_PARAMETERS = "Final Parameters: %1";

    // Logging - Process Attribute Modification
    static inline const QString LOG_EVENT_ARGS_ESCAPED = "CMD arguments escaped from [[%1]] to [[%2]]";
    static inline const QString LOG_EVENT_FORCED_BASH = "Forced use of 'sh' from Windows 'bat'";
    static inline const QString LOG_EVENT_FORCED_WIN = "Forced use of WINE from Windows 'exe'";


    // Logging - Process Management
    static inline const QString LOG_EVENT_CD = "Changed current directory to: %1";
    static inline const QString LOG_EVENT_STARTING = "Starting '%1' (%2)";
    static inline const QString LOG_EVENT_STARTED_PROCESS = "Started '%1'";
    static inline const QString LOG_EVENT_STOPPING_BLOCKING_PROCESS = "Stopping blocking process '%1'...";

    // Errors
    static inline const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
    static inline const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
    static inline const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";

    // Extensions
    static inline const QString SHELL_EXT_WIN = "bat";
    static inline const QString SHELL_EXT_LINUX = "sh";
    static inline const QString EXECUTABLE_EXT_WIN = "exe";

    // Deferred Processes
    static inline DeferredProcessManager* smDeferredProcessManager;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    BlockingProcessManager* mBlockingProcessManager;

    // Data
    QString mExecutable;
    QDir mDirectory;
    std::variant<QString, QStringList> mParameters;
    QProcessEnvironment mEnvironment;
    ProcessType mProcessType;
    QString mIdentifier;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExec(QObject* parent = nullptr);

//-Class Functions-----------------------------------------------------------------------------------------------------
private:
    static QString collapseArguments(const QStringList& args);

public:
    static void installDeferredProcessManager(DeferredProcessManager* manager);
    static DeferredProcessManager* deferredProcessManager();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Helpers
    QString resolveExecutablePath();
    QString escapeForShell(const QString& argStr);
    QString createEscapedShellArguments();
    QProcess* prepareProcess(const QFileInfo& execInfo);
    bool cleanStartProcess(QProcess* process);

    // Logging
    void logPreparedProcess(const QProcess* process);

public:
    // Member access
    QString name() const override;
    QStringList members() const override;

    QString executable() const;
    QDir directory() const;
    const std::variant<QString, QStringList>& parameters() const;
    const QProcessEnvironment& environment() const;
    ProcessType processType() const;
    QString identifier() const;

    void setExecutable(QString executable);
    void setDirectory(QDir directory);
    void setParameters(const std::variant<QString, QStringList>& parameters);
    void setEnvironment(const QProcessEnvironment& environment);
    void setProcessType(ProcessType processType);
    void setIdentifier(QString identifier);

    // Run
    void perform() override;
    void stop() override;


//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postBlockingProcess();
};

#endif // TEXEC_H
