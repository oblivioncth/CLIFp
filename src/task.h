#ifndef TASK_H
#define TASK_H

// Qt Includes
#include <QObject>
#include <QMessageBox>

// Qx Includes
#include <qx/core/qx-genericerror.h>

// Magic Enum Includes
#include "magic_enum.hpp"

/* The macro and typedef here exist to recreate the ones from core.h since that can't
 * be included here as it would cause infinite include recursion. When this is included
 * by core, the macro/typedef there is fine because the standard allows redefinition
 * of macros/typedefs as long as the definitions are the same in all locations.
 */

//-Macros----------------------------------------------------------------------
#define ENUM_NAME(eenum) QString(magic_enum::enum_name(eenum).data())

//-Typedef---------------------------------------------------------------------
typedef int ErrorCode;


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
    Task();

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
