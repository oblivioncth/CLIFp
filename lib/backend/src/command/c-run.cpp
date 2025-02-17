// Unit Include
#include "c-run.h"

// Project Includes
#include "kernel/core.h"
#include "task/t-exec.h"

//===============================================================================================================
// CRunError
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Private:
CRunError::CRunError(Type t, const QString& s) :
    mType(t),
    mSpecific(s)
{}

//-Instance Functions-------------------------------------------------------------
//Public:
bool CRunError::isValid() const { return mType != NoError; }
QString CRunError::specific() const { return mSpecific; }
CRunError::Type CRunError::type() const { return mType; }

//Private:
Qx::Severity CRunError::deriveSeverity() const { return Qx::Critical; }
quint32 CRunError::deriveValue() const { return mType; }
QString CRunError::derivePrimary() const { return ERR_STRINGS.value(mType); }
QString CRunError::deriveSecondary() const { return mSpecific; }

//===============================================================================================================
// CRUN
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
CRun::CRun(Core& coreRef) : Command(coreRef) {}

//-Instance Functions-------------------------------------------------------------
//Protected:
QList<const QCommandLineOption*> CRun::options() const { return CL_OPTIONS_SPECIFIC + Command::options(); }
QSet<const QCommandLineOption*> CRun::requiredOptions() const { return CL_OPTIONS_REQUIRED + Command::requiredOptions(); }
QString CRun::name() const { return NAME; }

Qx::Error CRun::perform()
{
    // Enqueue startup tasks
    if(Qx::Error ee = mCore.enqueueStartupTasks(); ee.isValid())
        return ee;

    QString inputPath = mCore.resolveFullAppPath(mParser.value(CL_OPTION_APP), u""_s); // No way of knowing platform
    QFileInfo inputInfo = QFileInfo(inputPath);

    TExec* runTask = new TExec(mCore);
    runTask->setIdentifier(NAME + u" program"_s);
    runTask->setStage(Task::Stage::Primary);
    runTask->setExecutable(QDir::cleanPath(inputInfo.absoluteFilePath())); // Like canonical but doesn't care if path DNE
    runTask->setDirectory(inputInfo.absolutePath());
    runTask->setParameters(mParser.value(CL_OPTION_PARAM));
    runTask->setEnvironment(mCore.childTitleProcessEnvironment());
    runTask->setProcessType(TExec::ProcessType::Blocking);

    mCore.enqueueSingleTask(runTask);
    postDirective<DStatusUpdate>(STATUS_RUN, inputInfo.fileName());

#ifdef _WIN32
    // Add wait task if required
    if(Qx::Error ee = mCore.conditionallyEnqueueBideTask(inputInfo); ee.isValid())
        return ee;
#endif

    // Return success
    return CRunError();
}

//Public:
bool CRun::requiresServices() const { return true; }
