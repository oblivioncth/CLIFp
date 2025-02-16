// Unit Include
#include "t-exec.h"

// Qt Includes
#include <QStandardPaths>

// Qx Includes
#include <qx/core/qx-algorithm.h>
#include <qx/core/qx-system.h>

using namespace  Qt::StringLiterals;

namespace // Unit helper functions
{

QProcess* setupExeProcess(const QString& exePath, const QStringList& exeArgs)
{
    /* TODO: Although fallback to this function is rare, there is a noteworthy limitation
     * with the current implementation of this. The existence of the executable in question
     * will have already been checked at this point, which means that after the swap the
     * new executable, Wine, is not checked. Because of this, if the user doesn't have
     * Wine on their system they will get a "failed to start error" that notes the
     * original executable name, which doesn't make it clear that the error was due
     * to missing wine.
     *
     * Currently this function has no way to fail/report errors, but at some point the
     * overall approach should be tweaked so that if Wine is swapped to, it is also checked
     * for in the same manner as the executable itself, with an error then returned if it
     * could not be found.
     */

    QProcess* process = new QProcess();

    // Force WINE
    process->setProgram(u"wine"_s);

    // Set arguments
    QStringList fullArgs{
        u"start"_s,
        u"/wait"_s,
        u"/unix"_s
    };
    fullArgs.append(exePath);
    fullArgs.append(exeArgs);

    process->setArguments(fullArgs);

    return process;
}

QProcess* setupShellProcess(const QString& commandOrScript, const QString& args)
{
    QProcess* process = new QProcess();

    // Set program
    process->setProgram(u"/bin/sh"_s);

    // Set arguments
    QString bashCommand = u"'"_s + commandOrScript + u"' "_s + args;
    process->setArguments({u"-c"_s, bashCommand});

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

bool isShellBuiltin(const QString& command)
{
    // Cash built-ins so no execution is required if the same one is checked for again
    static QSet<QString> knownBuiltins;
    if(knownBuiltins.contains(command))
        return true;

    // TODO: Use Qx::execute()/shellExecutre() in any other places that just need a quick program result
    Qx::ExecuteResult res = Qx::shellExecute(u"type"_s, command); // Ubuntu doesn't support '-t' (short name) switch
    if(res.exitCode != 0)
    {
        qWarning("Failed to query if %s is a built-in command!", qPrintable(command));
        return false;
    }
    if(res.output.contains(u"builtin"_s))
    {
        knownBuiltins.insert(command);
        return true;
    }

    return false;
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
     * The exceptions largely are that relative paths are always resolved relative to mDirectory instead
     * of how the system would handle it.
     */

    // Exception: If a windows batch script is called for by this point (uncommon), see if there is a matching shell script
    QFileInfo execInfo(mExecutable);
    if(execInfo.suffix() == SHELL_EXT_WIN)
    {
        execInfo.setFile(mExecutable.chopped(SHELL_EXT_WIN.size()) + SHELL_EXT_LINUX);
        logEvent(LOG_EVENT_FORCED_BASH);
    }

    // Mostly standard processing
    if(execInfo.isAbsolute())
        return execInfo.isExecutable() || execInfo.suffix() == EXECUTABLE_EXT_WIN ? execInfo.canonicalFilePath() : QString();
    else if(execInfo.filePath().contains('/')) // Relative, but not plain name
    {
        QFileInfo absolutePath(mDirectory.absoluteFilePath(execInfo.filePath()));
        return absolutePath.isExecutable() || absolutePath.suffix() == EXECUTABLE_EXT_WIN ? absolutePath.canonicalFilePath() : QString();
    }
    else // Plain name
    {
        // Shell built-ins get priority
        if(isShellBuiltin(mExecutable))
        {
            logEvent(mExecutable + u" is a shell builtin."_s);
            return mExecutable;
        }

        return QStandardPaths::findExecutable(execInfo.filePath()); // Searches system paths
    }
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

    auto lastQuoteIdx = argStr.lastIndexOf('"'); // If uneven number of quotes, treat last quote as a regular char
    for(int i = 0; i < argStr.size(); i++)
    {
        const QChar& chr = argStr.at(i);
        if(chr== '"' && (inQuotes || i != lastQuoteIdx))
            inQuotes = !inQuotes;

        if(inQuotes)
            escapedArgs.append(stdInQuotesEscapes.contains(chr) ? '\\' + chr : chr);
        else
            escapedArgs.append(curOutQuotesEscapes.contains(chr) ? '\\' + chr : chr);
    }

    return escapedArgs;
}

QProcess* TExec::prepareProcess(const QFileInfo& execInfo)
{
    if(execInfo.suffix() == EXECUTABLE_EXT_WIN)
    {
        logEvent(LOG_EVENT_FORCED_WIN);

        // Resolve passed parameters
        QStringList exeParam = std::holds_alternative<QString>(mParameters) ?
                               QProcess::splitCommand(std::get<QString>(mParameters)) :
                               std::get<QStringList>(mParameters);

        return setupExeProcess(execInfo.filePath(), exeParam);
    }
    else if(execInfo.suffix() == SHELL_EXT_LINUX || isShellBuiltin(execInfo.filePath()))
    {
        // Resolve passed parameters
        QString param = createEscapedShellArguments();
        return setupShellProcess(execInfo.filePath(), param);
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

void TExec::logPreparedProcess(const QProcess* process)
{
    logEvent(LOG_EVENT_FINAL_EXECUTABLE.arg(process->program()));
    logEvent(LOG_EVENT_FINAL_PARAMETERS.arg(!process->arguments().isEmpty() ?
                                            u"{\""_s + process->arguments().join(uR"(", ")"_s) + u"\"}"_s :
                                            u""_s));
}
