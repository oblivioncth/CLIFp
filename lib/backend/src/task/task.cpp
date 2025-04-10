// Unit Include
#include "task.h"

// Project Includes
#include "kernel/core.h"
#include "utility.h"

/* TODO: If a task ever needs to be executed conditionally (after being enqueued), create a Condition class with
 * a virtual method for checking the condition so that different checks can be implemented polymorphically. Then
 * add a 'setCondition()' method to the Task base so that any task can optionally have a condition. Then either
 * have driver perform the condition and only perform the task if it returns true, or build the task checking into
 * Task::perform()
 */

//===============================================================================================================
// Task
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
Task::Task(Core& core) :
    QObject(&core),
    Directorate(core.director())
{}

//-Instance Functions-------------------------------------------------------------
//Public:
QStringList Task::members() const { return {u".stage() = "_s + ENUM_NAME(mStage)}; }

Task::Stage Task::stage() const { return mStage; }
void Task::setStage(Stage stage) { mStage = stage; }

void Task::stop() { /* By default stopping isn't supported */ }



