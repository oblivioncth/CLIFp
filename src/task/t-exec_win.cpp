// Unit Include
#include "t-exec.h"

// Qt Includes
#include <QStandardPaths>

// Qx Includes
#include <qx/core/qx-regularexpression.h>

// Project Includes
#include "utility.h"

namespace // Unit helper functions
{

QProcess* setupBatchScriptProcess(const QString& scriptPath, const QString& scriptArgs)
{
    static const QString CMD_ARG_TEMPLATE = R"(/d /s /c ""%1" %2")";

    QProcess* childProcess = new QProcess();

    // Set program
    childProcess->setProgram("cmd.exe");

    // Set arguments
    QString cmdCommand = CMD_ARG_TEMPLATE.arg(scriptPath, scriptArgs);
    childProcess->setNativeArguments(cmdCommand);

    return childProcess;
}

QProcess* setupNativeProcess(const QString& exePath, std::variant<QString, QStringList> exeArgs)
{
    QProcess* childProcess = new QProcess();

    // Set program
    childProcess->setProgram(exePath);

    // Set arguments
    if(std::holds_alternative<QString>(exeArgs))
        childProcess->setNativeArguments(std::get<QString>(exeArgs));
    else
        childProcess->setArguments(std::get<QStringList>(exeArgs));

    return childProcess;
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

        // Then, if the path is a plain filename, check system paths
        if(resolvedPath.isEmpty() && !execInfo.filePath().contains('/')) // Plain name relative
            resolvedPath = QStandardPaths::findExecutable(execInfo.filePath()); // Searches system paths

        return resolvedPath;
    }
}

QString TExec::escapeForShell(const QString& argStr)
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

QProcess* TExec::prepareProcess(const QFileInfo& execInfo)
{
    if(execInfo.suffix() == SHELL_EXT_WIN)
    {
        // Resolve passed parameters
        QString scriptParam = createEscapedShellArguments();
        return setupBatchScriptProcess(execInfo.filePath(), scriptParam);
    }
    else
        return setupNativeProcess(execInfo.filePath(), mParameters);
}

void TExec::logPreparedProcess(const QProcess* process)
{
    emit eventOccurred(NAME, LOG_EVENT_FINAL_EXECUTABLE.arg(process->program()));
    emit eventOccurred(NAME, LOG_EVENT_FINAL_PARAMETERS.arg(!process->nativeArguments().isEmpty() ?
                                                            process->nativeArguments() :
                                                            !process->arguments().isEmpty() ?
                                                            "{\"" + process->arguments().join(R"(", ")") + "\"}" :
                                                            ""));
}
