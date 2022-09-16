#ifndef TEXEC_H
#define TEXEC_H

// Project Includes
#include "task/task.h"

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

    // Logging
    static inline const QString LOG_EVENT_CD = "Changed current directory to: %1";
    static inline const QString LOG_EVENT_START_PROCESS = "Started %1 process: %2";
    static inline const QString LOG_EVENT_END_PROCESS = "%1 process %2 finished";
    static inline const QString LOG_EVENT_ARGS_ESCAPED = "CMD arguments escaped from [[%1]] to [[%2]]";
    static inline const QString LOG_EVENT_FORCED_BASH = "Forced use of 'sh' from Windows 'bat'";
    static inline const QString LOG_EVENT_FORCED_WIN = "Forced use of WINE from Windows 'exe'";
    static inline const QString LOG_EVENT_STOPPING_MAIN_PROCESS = "Stopping primary execution process...";

    // Errors
    static inline const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
    static inline const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
    static inline const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";

    // Extensions
    static inline const QString SHELL_EXT_WIN = "bat";
    static inline const QString SHELL_EXT_LINUX = "sh";
    static inline const QString EXECUTABLE_EXT_WIN = "exe";

    // Deferred Processes
    static inline QList<QProcess*> smActiveChildProcesses;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    QProcess* mBlockingProcess;

    // Data
    QString mPath;
    QString mFilename;
    std::variant<QString, QStringList> mParameters;
    ProcessType mProcessType;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExec(QObject* parent = nullptr);

//-Class Functions-----------------------------------------------------------------------------------------------------
private:
    static QString collapseArguments(const QStringList& args);
    static QString createCloseProcessString(const QProcess* process, ProcessType type);

public:
    static QStringList closeChildProcesses();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    // Helpers
    QString createEscapedShellArguments();
    QProcess* prepareDirectProcess();
    QProcess* prepareShellProcess();
    bool cleanStartProcess(QProcess* process, QFileInfo exeInfo);

    // Logging
    void logProcessStart(const QProcess* process, ProcessType type);
    void logProcessEnd(const QProcess* process, ProcessType type);

public:
    // Member access
    QString name() const override;
    QStringList members() const override;

    QString path() const;
    QString filename() const;
    const std::variant<QString, QStringList>& parameters() const;
    ProcessType processType() const;

    void setPath(QString path);
    void setFilename(QString filename);
    void setParameters(const std::variant<QString, QStringList>& parameters);
    void setProcessType(ProcessType processType);

    // Run
    void perform() override;
    void stop() override;


//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postBlockingProcess();
};

#endif // TEXEC_H
