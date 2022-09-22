// Unit Include
#include "t-exec.h"

// Qx Includes
#include <qx/core/qx-algorithm.h>

// Project Includes
#include "utility.h"

//===============================================================================================================
// TExec
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Private:
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

QProcess* TExec::prepareDirectProcess()
{
    QProcess* childProcess = new QProcess();
    QFileInfo fileInfo(mFilename);


    // Force WINE if windows app made it through to this point (probably won't happen)
    if(fileInfo.suffix() == EXECUTABLE_EXT_WIN)
    {
        emit eventOccurred(NAME, LOG_EVENT_FORCED_BASH);

        // Set program
        childProcess->setProgram("wine");

        // Set arguments
        childProcess->setArguments({
            "start",
            "/wait",
            "/unix",
            mFilename,
            createEscapedShellArguments()
        });

    }
    else
    {
        // Set program
        childProcess->setProgram(mFilename);

        // Set arguments
        if(std::holds_alternative<QString>(mParameters))
            childProcess->setArguments(QProcess::splitCommand(std::get<QString>(mParameters)));
        else
            childProcess->setArguments(std::get<QStringList>(mParameters));
    }

    return childProcess;
}

QProcess* TExec::prepareShellProcess()
{
    QProcess* childProcess = new QProcess();

    QFileInfo fileInfo(mFilename);
    QString finalFile = mFilename;

    // Force shell script if windows BAT made it through to this point (probably won't happen)
    if(fileInfo.suffix() == SHELL_EXT_WIN)
    {
        emit eventOccurred(NAME, LOG_EVENT_FORCED_BASH);
        finalFile.replace('.' + SHELL_EXT_WIN, '.' + SHELL_EXT_LINUX);
    }

    // Set program
    childProcess->setProgram("/bin/sh");

    // Set arguments
    QString bashCommand = mFilename + ' ' + createEscapedShellArguments();
    childProcess->setArguments({"-c", bashCommand});

    return childProcess;
}

void TExec::logProcessStart(const QProcess* process, ProcessType type)
{
    QString eventStr = process->program();
    if(!process->arguments().isEmpty())
        eventStr += " {\"" + process->arguments().join(R"(", ")") + "\"}";

    emit eventOccurred(NAME, LOG_EVENT_START_PROCESS.arg(ENUM_NAME(type), mIdentifier, eventStr));
}
