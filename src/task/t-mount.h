#ifndef TMOUNT_H
#define TMOUNT_H

// Project Includes
#include "task/task.h"
#include "tools/mounter.h"

class TMount : public Task
{
    Q_OBJECT;

//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TMount");

    // Logging
    static inline const QString LOG_EVENT_MOUNTING_DATA_PACK = "Mounting Data Pack %1";
    static inline const QString LOG_EVENT_STOPPING_MOUNT = "Stopping current mount(s)...";

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    Mounter mMounter;

    // Data
    QUuid mTitleId;
    QString mPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TMount();

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    QUuid titleId() const;
    QString path() const;

    void setTitleId(QUuid titleId);
    void setPath(QString path);

    void perform() override;
    void stop() override;

//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postMount(ErrorCode errorStatus);
};

#endif // TMOUNT_H
