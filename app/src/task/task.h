#ifndef TASK_H
#define TASK_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-error.h>

// Project Includes
#include "frontend/message.h"

class Task : public QObject
{
    Q_OBJECT;
//-Class Enums-----------------------------------------------------------------------------------------------------
public:
    enum class Stage { Startup, Primary, Auxiliary, Shutdown };

//-Instance Variables------------------------------------------------------------------------------------------------
protected:
    Stage mStage;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    Task(QObject* parent); // Require tasks to have a parent

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    virtual ~Task() = default;

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    // Notifications/Logging (signal-forwarders)
    void emitEventOccurred(const QString& event);
    void emitErrorOccurred(const Qx::Error& error);
    void emitBlockingErrorOccurred(int* response, const Qx::Error& error, QMessageBox::StandardButtons choices);

public:
    virtual QString name() const = 0;
    virtual QStringList members() const;

    Stage stage() const;
    void setStage(Stage stage);

    virtual void perform() = 0;
    virtual void stop();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void notificationReady(const Message& msg);
    void eventOccurred(const QString& taskName, const QString& event);
    void errorOccurred(const QString& taskName, const Qx::Error& error);
    void blockingErrorOccurred(const QString& taskName, int* response, const Qx::Error& error, QMessageBox::StandardButtons choices);

    void longTaskStarted(const QString& procedure);
    void longTaskTotalChanged(quint64 total);
    void longTaskProgressChanged(quint64 progress);
    void longTaskFinished();

    void complete(const Qx::Error& errorState);
};

#endif // TASK_H
