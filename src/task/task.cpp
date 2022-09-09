// Unit Include
#include "task.h"

// Project Includes
#include "utility.h"

//===============================================================================================================
// Task
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
Task::Task() {}

//-Instance Functions-------------------------------------------------------------
//Public:
QStringList Task::members() const { return {".stage() = " + ENUM_NAME(mStage)}; }

Task::Stage Task::stage() const { return mStage; }
void Task::setStage(Stage stage) { mStage = stage; }

void Task::stop() { /* By default stopping isn't supported */ }



