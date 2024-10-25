#ifndef TASK_H
#define TASK_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-error.h>

// Project Includes
#include "kernel/directorate.h"

class Core;

class Task : public QObject, public Directorate
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
    Task(Core& core);

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    virtual ~Task() = default;

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    virtual QStringList members() const;

    Stage stage() const;
    void setStage(Stage stage);

    virtual void perform() = 0;
    virtual void stop();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
signals:
    void complete(const Qx::Error& errorState);
};

#endif // TASK_H
