#ifndef TGENERIC_H
#define TGENERIC_H

// Qx Includes
#include <qx/core/qx-genericerror.h>

// Project Includes
#include "task/task.h"

class TGeneric : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TGeneric"_s;

    // Logging
    static inline const QString LOG_EVENT_START_ACTION = u"Starting generic action: %1"_s;
    static inline const QString LOG_EVENT_END_ACTION = u"Finished generic action."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    QString mDescription;
    std::function<Qx::Error()> mAction;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TGeneric(QObject* parent);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    QString description() const;
    void setDescription(const QString& desc);
    void setAction(std::function<Qx::Error()> action);

    void perform() override;
    //void stop() override; Not supported, so try to keep these short
};

#endif // TGENERIC_H
