// Unit Include
#include "t-exec.h"

// Qt Includes
#include <QStandardPaths>

// Qx Includes
#include <qx/core/qx-algorithm.h>

// Project Includes
#include "utility.h"

namespace // Unit helper functions
{

QProcess* setupExeProcess(const QString& exePath, const QString& exeArgs)
{
    QProcess* process = new QProcess();

    // Force WINE
    process->setProgram("wine");

    // Set arguments
    process->setArguments({
        "start",
        "/wait",
        "/unix",
        exePath,
        exeArgs
    });

    return process;
}

QProcess* setupShellScriptProcess(const QString& scriptPath, const QString& scriptArgs)
{
    QProcess* process = new QProcess();

    // Set program
    process->setProgram("/bin/sh");

    // Set arguments
    QString bashCommand = "'" + scriptPath + "' " + scriptArgs;
    process->setArguments({"-c", bashCommand});

    return process;
}

QProcess* setupNativeProcess(const QString& execPath, const QStringList& execArgs)
{
    QProcess* process = new QProcess();

    // Set program
    process->setProgram(execPath);

    // Set arguments
    process->setArguments(execArgs);

    return process;
}

}

//===============================================================================================================
// TExec
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Private:
QString TExec::resolveExecutablePath()
{
    /* Mimic how Linux (and QProcess on Linux) search for an executable, with some exceptions
     * See: https://doc.qt.io/qt-6/qprocess.html#finding-the-executable
     *
     * The exceptions largely are that relative paths are always resolved relative to mDir instead
     * of how the system would handle it.
     */

    // Exception: If a windows batch script is called for (shouldn't happen), see if there is a matching shell script
    // as a last resort
    QFileInfo execInfo(mExecutable);
    if(execInfo.suffix() == SHELL_EXT_WIN)
        execInfo.setFile(mExecutable.chopped(sizeof(SHELL_EXT_WIN)) + SHELL_EXT_LINUX);

    // Mostly standard processing
    if(execInfo.isAbsolute())
        return execInfo.isExecutable() || execInfo.suffix() == EXECUTABLE_EXT_WIN ? execInfo.canonicalFilePath() : QString();
    else if(execInfo.filePath().contains('/')) // Relative, but not plain name
    {
        QFileInfo absolutePath(mDirectory.absoluteFilePath(execInfo.filePath()));
        return absolutePath.isExecutable() || absolutePath.suffix() == EXECUTABLE_EXT_WIN ? absolutePath.canonicalFilePath() : QString();
    }
    else // Plain name
        return QStandardPaths::findExecutable(execInfo.filePath()); // Searches system paths
}

QString TExec::escapeForShell(const QString& argStr)
{
    static const QSet<QChar> stdOutQuotesEscapes{
        '~','`','#','$','&','*','(',')','\\','|','[',']','{','}',';','<','>','?','!'
    };
    static const QSet<QChar> stdInQuotesEscapes{'$','!','\\'};
    QSet<QChar> curOutQuotesEscapes = stdOutQuotesEscapes;

    // Escape single quotes if args have an uneven amount
    if(!Qx::isEven(argStr.count('\'')))
        curOutQuotesEscapes.insert('\'');

    QString escapedArgs;
    bool inQuotes = false;

    for(int i = 0; i < argStr.size(); i++)
    {
        const QChar& chr = argStr.at(i);
        if(chr== '"' && (inQuotes || i != argStr.lastIndexOf('"')))
            inQuotes = !inQuotes;

        if(inQuotes)
            escapedArgs.append(stdInQuotesEscapes.contains(chr) ? '^' + chr : chr);
        else
            escapedArgs.append(curOutQuotesEscapes.contains(chr) ? '^' + chr : chr);
    }

    return escapedArgs;
}

QProcess* TExec::prepareProcess(const QFileInfo& execInfo)
{
    if(execInfo.suffix() == EXECUTABLE_EXT_WIN)
    {
        emit eventOccurred(NAME, LOG_EVENT_FORCED_BASH);

        // Resolve passed parameters
        QString exeParam = std::holds_alternative<QStringList>(mParameters) ?
                           collapseArguments(std::get<QStringList>(mParameters)) :
                           std::get<QString>(mParameters);

        return setupExeProcess(execInfo.filePath(), exeParam);
    }
    else if(execInfo.suffix() == SHELL_EXT_LINUX)
    {
        // Resolve passed parameters
        QString scriptParam = createEscapedShellArguments();
        return setupShellScriptProcess(execInfo.filePath(), scriptParam);
    }
    else
    {
        // Resolve passed parameters
        QStringList execParam = std::holds_alternative<QString>(mParameters) ?
                                QProcess::splitCommand(std::get<QString>(mParameters)) :
                                std::get<QStringList>(mParameters);

        return setupNativeProcess(execInfo.filePath(), execParam);
    }
}

void TExec::logProcessStart(const QProcess* process, ProcessType type)
{
    QString eventStr = process->program();
    if(!process->arguments().isEmpty())
        eventStr += " {\"" + process->arguments().join(R"(", ")") + "\"}";

    emit eventOccurred(NAME, LOG_EVENT_START_PROCESS.arg(ENUM_NAME(type), mIdentifier, eventStr));
}
