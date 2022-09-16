// Unit Include
#include "t-exec.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>

// Project Includes
#include "utility.h"

namespace // Unit only definitions
{

QString escapeForShell(QString argStr)
{
    static const QSet<QChar> escapeChars{'^','&','<','>','|'};
    QString escapedArgs;
    bool inQuotes = false;

    for(int i = 0; i < argStr.size(); i++)
    {
        const QChar& chr = argStr.at(i);
        if(chr== '"' && (inQuotes || i != argStr.lastIndexOf('"')))
            inQuotes = !inQuotes;

        escapedArgs.append((!inQuotes && escapeChars.contains(chr)) ? '^' + chr : chr);
    }

    return escapedArgs;
}

}

//===============================================================================================================
// TExec
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Private:
QString TExec::resolveShellArguments()
{
    // Determine arguments
    QString escapedArgs;
    if(std::holds_alternative<QString>(mParameters))
    {
        // Escape
        QString args = std::get<QString>(mParameters);
        escapedArgs = escapeForShell(args);
        if(args != escapedArgs)
            emit eventOccurred(NAME, LOG_EVENT_ARGS_ESCAPED.arg(args, escapedArgs));
    }
    else
    {
        // Collapse
        QStringList parameters = std::get<QStringList>(mParameters);
        QString collapsedParameters = collapseArguments(parameters);

        // Escape
        escapedArgs = escapeForShell(collapsedParameters);

        QStringList rebuild = QProcess::splitCommand(escapedArgs);
        if(rebuild != parameters)
        {
            emit eventOccurred(NAME, LOG_EVENT_ARGS_ESCAPED.arg("{\"" + parameters.join(R"(", ")") + "\"}",
                                                                "{\"" + rebuild.join(R"(", ")") + "\"}"));
        }
    }

    return escapedArgs;
}

QProcess* TExec::prepareDirectProcess()
{
    QProcess* childProcess = new QProcess();

    // Set program
    childProcess->setProgram(mFilename);

    // Set arguments
    if(std::holds_alternative<QString>(mParameters))
        childProcess->setNativeArguments(std::get<QString>(mParameters));
    else
        childProcess->setArguments(std::get<QStringList>(mParameters));

    return childProcess;
}

QProcess* TExec::prepareShellProcess()
{
    // This ain't for Linux son
    assert(QFileInfo(mFilename).suffix() != SHELL_EXT_LINUX);

    static const QString CMD_ARG_TEMPLATE = R"(/d /s /c ""%1" %2")";

    QProcess* childProcess = new QProcess();

    // Set program
    childProcess->setProgram("cmd.exe");

    // Set arguments
    QString cmdCommand = CMD_ARG_TEMPLATE.arg(mFilename, createEscapedShellArguments());
    childProcess->setNativeArguments(cmdCommand);

    return childProcess;
}

void TExec::logProcessStart(const QProcess* process, ProcessType type)
{
    QString eventStr = process->program();
    if(!process->nativeArguments().isEmpty())
        eventStr += " " + process->nativeArguments();

    emit eventOccurred(NAME, LOG_EVENT_START_PROCESS.arg(ENUM_NAME(type), eventStr));
}