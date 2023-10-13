#ifndef TMOUNT_H
#define TMOUNT_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Qt Includes
#include <QUuid>

// Project Includes
#include "task/task.h"
#ifdef FP_PROXY
    #include "tools/mounter_proxy.h"
#else
    #include "tools/mounter.h"
#endif

class TMount : public Task
{
    Q_OBJECT;

//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TMount"_s;

    // Logging
    static inline const QString LOG_EVENT_MOUNTING_DATA_PACK = u"Mounting Data Pack %1"_s;
    static inline const QString LOG_EVENT_STOPPING_MOUNT = u"Stopping current mount(s)..."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    Mounter mMounter;

    // Data
    bool mSkipQemu;
    QUuid mTitleId;
    QString mPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TMount(QObject* parent);

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
    void postMount(MounterError errorStatus);
};

#endif // TMOUNT_H
