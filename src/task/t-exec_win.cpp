// Unit Include
#include "t-exec.h"

// Qx Includes
#include <qx/core/qx-regularexpression.h>

namespace // Unit only definitions
{

QString escapeForShell(QString args)
{
    static const QSet<QChar> escapeChars{'^','&','<','>','|'};
    QString escapedArgs;
    bool inQuotes = false;

    for(int i = 0; i < args.size(); i++)
    {
        const QChar& chr = args.at(i);
        if(chr== '"' && (inQuotes || i != args.lastIndexOf('"')))
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

    QProcess* childProcess = new QProcess();

    // Set program
    childProcess->setProgram("cmd.exe");

    // Set arguments
    static const QString CMD_ARG_TEMPLATE = R"(/d /s /c ""%1" %2")";
    if(std::holds_alternative<QString>(mParameters))
    {
        // Escape
        QString args = std::get<QString>(mParameters);
        QString escapedArgs = escapeForShell(args);
        if(args != escapedArgs)
            emit eventOccurred(NAME, LOG_EVENT_ARGS_ESCAPED.arg(args, escapedArgs));

        // Set
        childProcess->setNativeArguments(CMD_ARG_TEMPLATE.arg(mFilename, escapedArgs));
    }
    else
    {
        // Collapse
        QStringList parameters = std::get<QStringList>(mParameters);
        QString reduction;
        for(const QString& param : parameters)
        {
            if(!param.isEmpty())
            {
                reduction += ' ';

                if(param.contains(Qx::RegularExpression::WHITESPACE) && !(param.front() == '\"' && param.back() == '\"'))
                    reduction += '\"' + param + '\"';
                else
                    reduction += param;
            }

        }

        // Escape
        QString escapedArgs = escapeForShell(reduction);

        QStringList rebuild = QProcess::splitCommand(escapedArgs);
        if(rebuild != parameters)
        {
            emit eventOccurred(NAME, LOG_EVENT_ARGS_ESCAPED.arg("{\"" + parameters.join(R"(", ")") + "\"}",
                                                                "{\"" + rebuild.join(R"(", ")") + "\"}"));
        }

        // Set
        childProcess->setNativeArguments(CMD_ARG_TEMPLATE.arg(mFilename, escapedArgs));
    }

    return childProcess;
}
