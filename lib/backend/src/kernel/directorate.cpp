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
void Directorate::logError(const Qx::Error& error) const { Q_ASSERT(mDirector); mDirector->logError(name(), error); }
void Directorate::logEvent(const QString& event) const { Q_ASSERT(mDirector); mDirector->logEvent(name(), event); }

//Public:
Director* Directorate::director() const { return mDirector; }

//Public:
void Directorate::setDirector(Director* director) { mDirector = director; }
