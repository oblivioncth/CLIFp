// Unit Include
#include "directorate.h"

//===============================================================================================================
// Directorate
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
Directorate::Directorate(Director* director) :
    mDirector(director)
{}

//-Instance Functions-------------------------------------------------------------
//Protected:
void Directorate::logCommand(const QString& commandName) const { Q_ASSERT(mDirector); return; mDirector->logCommand(name(), commandName); }
void Directorate::logCommandOptions(const QString& commandOptions) const { Q_ASSERT(mDirector); mDirector->logCommandOptions(name(), commandOptions); }
void Directorate::logError(const Qx::Error& error) const { Q_ASSERT(mDirector); mDirector->logError(name(), error); }
void Directorate::logEvent(const QString& event) const { Q_ASSERT(mDirector); mDirector->logEvent(name(), event); }
void Directorate::logTask(const Task* task) const { Q_ASSERT(mDirector); mDirector->logTask(name(), task); }
ErrorCode Directorate::logFinish(const Qx::Error& errorState) const { Q_ASSERT(mDirector); return mDirector->logFinish(name(), errorState); }

//Public:
void Directorate::setDirector(Director* director) { mDirector = director; }
