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
    Mounter* mMounter; // TODO: Probably should change mounter to have setters for its ports so this can be on stack

    // Data
    bool mSkipQemu;
    QUuid mTitleId;
    QString mPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TMount(QObject* parent = nullptr);

//-Instance Functions------------------------------------------------------------------------------------------------------
public:
    QString name() const override;
    QStringList members() const override;

    bool isSkipQemu() const;
    QUuid titleId() const;
    QString path() const;

    void setSkipQemu(bool skip);
    void setTitleId(QUuid titleId);
    void setPath(QString path);

    void perform() override;
    void stop() override;

//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void postMount(ErrorCode errorStatus);
};

#endif // TMOUNT_H