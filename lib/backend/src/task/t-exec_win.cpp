// Unit Include
#include "t-exec.h"

// Qt Includes
#include <QStandardPaths>

// Qx Includes
#include <qx/core/qx-regularexpression.h>

namespace // Unit helper functions
{

// No need for these to be able to return error state for the time being
void setupBatchScriptProcess(QProcess* process, const QString& scriptPath, const QString& scriptArgs)
{
    static const QString CMD_ARG_TEMPLATE = uR"(/d /s /c ""%1" %2")"_s;

    // Set program
    process->setProgram(u"cmd.exe"_s);

    // Set arguments
    QString cmdCommand = CMD_ARG_TEMPLATE.arg(scriptPath, scriptArgs);
    process->setNativeArguments(cmdCommand);
}

void setupNativeProcess(QProcess* process, const QString& exePath, std::variant<QString, QStringList> exeArgs)
{
    // Set program
    process->setProgram(exePath);

    // Set arguments
    if(std::holds_alternative<QString>(exeArgs))
        process->setNativeArguments(std::get<QString>(exeArgs));
    else
        process->setArguments(std::get<QStringList>(exeArgs));
}

}

//===============================================================================================================
// TExec
//===============================================================================================================

//-Instance Functions-------------------------------------------------------------
//Private:
QString TExec::resolveExecutablePath()
{
    /* Mimic how Windows (and QProcess on Windows) search for an executable, with some exceptions
     * See: https://doc.qt.io/qt-6/qprocess.html#finding-the-executable
     *
     * The exceptions largely are that relative paths are always resolved relative to mDirectory instead
     * of how the system would handle it.
     *
     * On windows you can start an application (.exe/.bat) with just it's filename, so the use of
     * QStandardPaths::findExecutable() with a second argument here ensures that both extensions are checked for.
     */
    QFileInfo execInfo(mExecutable);

    // Mostly standard processing
    if(execInfo.isAbsolute())
        return QStandardPaths::findExecutable(execInfo.fileName(), {execInfo.absolutePath()});
    else // Relative
    {
        // First check relative to mDirectory
        QFileInfo absolutePath(mDirectory.absoluteFilePath(execInfo.filePath()));
        QString resolvedPath = QStandardPaths::findExecutable(absolutePath.fileName(), {absolutePath.absolutePath()});

        // Then, if the path is a plain filename, check system path
        if(resolvedPath.isEmpty() && !execInfo.filePath().contains('/')) // Plain name relative
            resolvedPath = findSystemExecutable(execInfo.filePath());

        return resolvedPath;
    }
}

QString TExec::escapeForShell(const QString& argStr)
{
    static const QSet<QChar> escapeChars{'^','&','<','>','|'};
    QString escapedArgs;
    bool inQuotes = false;

    auto lastQuoteIdx = argStr.lastIndexOf('"'); // If uneven number of quotes, treat last quote as a regular char
    for(int i = 0; i < argStr.size(); i++)
    {
        const QChar& chr = argStr.at(i);
        if(chr== '"' && (inQuotes || i != lastQuoteIdx))
            inQuotes = !inQuotes;

        escapedArgs.append((!inQuotes && escapeChars.contains(chr)) ? '^' + chr : chr);
    }

    return escapedArgs;
}

TExecError TExec::prepareProcess(QProcess* process, const QFileInfo& execInfo)
{
    if(execInfo.suffix() == SHELL_EXT_WIN)
    {
        // Resolve passed parameters
        QString scriptParam = createEscapedShellArguments();
        setupBatchScriptProcess(process, execInfo.filePath(), scriptParam);
    }
    else
        setupNativeProcess(process, execInfo.filePath(), mParameters);

    return TExecError();
}

void TExec::logPreparedProcess(const QProcess* process)
{
    logEvent(LOG_EVENT_FINAL_EXECUTABLE.arg(process->program()));
    logEvent(LOG_EVENT_FINAL_PARAMETERS.arg(!process->nativeArguments().isEmpty() ?
                                                            process->nativeArguments() :
                                                            !process->arguments().isEmpty() ?
                                                            u"{\""_s + process->arguments().join(uR"(", ")"_s) + u"\"}"_s :
                                                            u""_s));
}
