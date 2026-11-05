// Unit Include
#include "t-exec.h"

// Standard Library Includes
#include <functional>

// Qx Includes
#include <qx/core/qx-algorithm.h>
#include <qx/core/qx-system.h>

using namespace  Qt::StringLiterals;

namespace // Unit helper functions
{

/* This is quite a bit of plumbing just to let these implementation detail non-member functions
 * construct TExecError, but we're rolling with it for now.
 *
 * ErrorFactory could have just been a function alias instead so that the raw lambda is passed as an arg,
 * but then the default args wouldn't survive.
 */
struct ErrorFactory
{
    std::function<TExecError(TExecError::Type, const QString&)> fn;
    TExecError operator()(TExecError::Type t = TExecError::NoError, const QString& s = {}) const
    {
        return fn(t, s);
    }
};

TExecError setupExeProcess(QProcess* process, const QString& exePath, const QStringList& exeArgs, const ErrorFactory& ef)
{
    Q_ASSERT(process);

    // Force WINE
    QString trueExe = u"wine"_s;
    QString wine = TExec::findSystemExecutable(trueExe);
    if(wine.isEmpty())
        return ef(TExecError::CouldNotFind, trueExe);

    process->setProgram(wine);

    // Set arguments
    QStringList fullArgs{
        u"start"_s,
        u"/wait"_s,
        u"/unix"_s
    };
    fullArgs.append(exePath);
    fullArgs.append(exeArgs);

    process->setArguments(fullArgs);

    return ef();
}

TExecError setupShellProcess(QProcess* process, const QString& commandOrScript, const QString& args, const ErrorFactory& ef)
{
    Q_ASSERT(process);

    // Set program
    process->setProgram(u"/bin/sh"_s);

    // Set arguments
    QString bashCommand = u"'"_s + commandOrScript + u"' "_s + args;
    process->setArguments({u"-c"_s, bashCommand});

    return ef();
}

TExecError setupNativeProcess(QProcess* process, const QString& execPath, const QStringList& execArgs, const ErrorFactory& ef)
{
    Q_ASSERT(process);

    // Set program
    process->setProgram(execPath);

    // Set arguments
    process->setArguments(execArgs);

    return ef();
}

bool isShellBuiltin(const QString& command)
{
    // Cache built-ins so no execution is required if the same one is checked for again
    static QSet<QString> knownBuiltins;
    if(knownBuiltins.contains(command))
        return true;

    // TODO: Use Qx::execute()/shellExecute() in any other places that just need a quick program result
    Qx::ExecuteResult res = Qx::shellExecute(u"type"_s, command); // Ubuntu doesn't support '-t' (short name) switch
    if(res.exitCode > 0)
        return false; // Program not found by 'type' at all, so obviously not a built-in
    else if(res.exitCode < 0)
    {
        qWarning("Failed to query if %s is a built-in command!", qPrintable(command));
        return false;
    }
    else if(res.output.trimmed().endsWith(u"shell builtin"_s))
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

        // Check system path
        return findSystemExecutable(execInfo.filePath());
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

TExecError TExec::prepareProcess(QProcess* process, const QFileInfo& execInfo)
{
    Q_ASSERT(process);

    // To let the non-member functions produce errors
    ErrorFactory ef{
        [sv = mStage == Stage::Shutdown ? Qx::Err : Qx::Critical](TExecError::Type t, const QString& s) { return TExecError(t, s, sv); }
    };

    if(execInfo.suffix() == EXECUTABLE_EXT_WIN)
    {
        logEvent(LOG_EVENT_FORCED_WIN);

        // Resolve passed parameters
        QStringList exeParam = std::holds_alternative<QString>(mParameters) ?
                               QProcess::splitCommand(std::get<QString>(mParameters)) :
                               std::get<QStringList>(mParameters);

        return setupExeProcess(process, execInfo.filePath(), exeParam, ef);
    }
    else if(execInfo.suffix() == SHELL_EXT_LINUX || isShellBuiltin(execInfo.filePath()))
    {
        // Resolve passed parameters
        QString param = createEscapedShellArguments();
        return setupShellProcess(process, execInfo.filePath(), param, ef);
    }
    else
    {
        // Resolve passed parameters
        QStringList execParam = std::holds_alternative<QString>(mParameters) ?
                                QProcess::splitCommand(std::get<QString>(mParameters)) :
                                std::get<QStringList>(mParameters);

        return setupNativeProcess(process, execInfo.filePath(), execParam, ef);
    }
}

void TExec::logPreparedProcess(const QProcess* process)
{
    logEvent(LOG_EVENT_FINAL_EXECUTABLE.arg(process->program()));
    logEvent(LOG_EVENT_FINAL_PARAMETERS.arg(!process->arguments().isEmpty() ?
                                            u"{\""_s + process->arguments().join(uR"(", ")"_s) + u"\"}"_s :
                                            u""_s));
}
