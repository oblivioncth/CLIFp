#ifndef TASK_H
#define TASK_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-genericerror.h>

// Project Includes
#include "kernel/errorcode.h"

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
public:
    virtual QString name() const = 0;
    virtual QStringList members() const;

    Stage stage() const;
    void setStage(Stage stage);

    virtual void perform() = 0;
    virtual void stop();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void notificationReady(QString message);
    void eventOccurred(QString taskName, QString event);
    void errorOccurred(QString taskName, Qx::GenericError error);
    void blockingErrorOccured(QString taskName, int* response, Qx::GenericError error, QMessageBox::StandardButtons choices);

    void longTaskStarted(QString procedure);
    void longTaskTotalChanged(quint64 total);
    void longTaskProgressChanged(quint64 progress);
    void longTaskFinished();

    void complete(ErrorCode errorStatus);
};

#endif // TASK_H
