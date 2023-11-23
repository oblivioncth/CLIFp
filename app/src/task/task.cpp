// Unit Include
#include "task.h"

// Project Includes
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
Task::Task(QObject* parent) :
    QObject(parent)
{}

//-Instance Functions-------------------------------------------------------------
//Protected:
// Notifications/Logging (signal-forwarders)
void Task::emitEventOccurred(const QString& event) { emit eventOccurred(name(), event); }
void Task::emitErrorOccurred(const Qx::Error& error) { emit errorOccurred(name(), error); }
void Task::emitBlockingErrorOccurred(int* response, const Qx::Error& error, QMessageBox::StandardButtons choices) { emit blockingErrorOccurred(name(), response, error, choices); }

//Public:
QStringList Task::members() const { return {u".stage() = "_s + ENUM_NAME(mStage)}; }

Task::Stage Task::stage() const { return mStage; }
void Task::setStage(Stage stage) { mStage = stage; }

void Task::stop() { /* By default stopping isn't supported */ }



