#ifndef TEXEC_H
#define TEXEC_H

// Project Includes
#include "../task.h"

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

//-Inner Classes--------------------------------------------------------------------------------------------------------
private:
    class ErrorCodes
    {
    //-Class Variables--------------------------------------------------------------------------------------------------
    public:
        static const ErrorCode NO_ERR = 0;
        static const ErrorCode EXECUTABLE_NOT_FOUND = 8;
        static const ErrorCode EXECUTABLE_NOT_VALID = 9;
        static const ErrorCode PROCESS_START_FAIL = 10;
    };

//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TExec");

    // Logging
    static inline const QString LOG_EVENT_CD = "Changed current directory to: %1";
    static inline const QString LOG_EVENT_START_PROCESS = "Started %1 process: %2";
    static inline const QString LOG_EVENT_END_PROCESS = "%1 process %2 finished";
    static inline const QString LOG_EVENT_ARGS_ESCAPED = "CMD arguments escaped from [[%1]] to [[%2]]";
    static inline const QString LOG_EVENT_BASILISK_EXCEPTION = NAME + " calls for Basilisk-Portable. Swapping for FPNavigator";
    static inline const QString LOG_EVENT_STOPPING_MAIN_PROCESS = "Stopping primary execution process...";

    // Errors
    static inline const QString ERR_EXE_NOT_FOUND = "Could not find %1!";
    static inline const QString ERR_EXE_NOT_STARTED = "Could not start %1!";
    static inline const QString ERR_EXE_NOT_VALID = "%1 is not an executable file!";

    // Suffixes
    static inline const QString BAT_SUFX = "bat";
    static inline const QString EXE_SUFX = "exe";

    // Processing constants
    static inline const QString CMD_EXE = "cmd.exe";
    static inline const QString CMD_ARG_TEMPLATE = R"(/d /s /c ""%1" %2")";

    // Deferred Processes
    static inline QList<QProcess*> smActiveChildProcesses;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    QProcess* mBlockingProcess;

    // Data
    QString mPath;
    QString mFilename;
    QStringList mParameters;
    QString mNativeParameters;
    ProcessType mProcessType;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TExec();

//-Class Functions-----------------------------------------------------------------------------------------------------
private:
    static QString createCloseProcessString(const QProcess* process, ProcessType type);

public:
    static QStringList closeChildProcesses();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    QString escapeNativeArgsForCMD(QString nativeArgs);
    bool cleanStartProcess(QProcess* process, QFileInfo exeInfo);

    void logProcessStart(const QProcess* process, ProcessType type);
    void logProcessEnd(const QProcess* process, ProcessType type);

public:
    QString name() const override;
    QStringList members() const override;

    QString path() const;
    QString filename() const;
    const QStringList& parameters() const;
    QString nativeParameters() const;
    ProcessType processType() const;

    void setPath(QString path);
    void setFilename(QString filename);
    void setParameters(const QStringList& parameters);
    void setNativeParameters(QString nativeParameters);
    void setProcessType(ProcessType processType);

    void perform() override;
    void stop() override;


//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postBlockingProcess();
};

#endif // TEXEC_H
