#ifndef TMOUNT_H
#define TMOUNT_H

// Qx Includes
#include <qx/utility/qx-macros.h>

// Qt Includes
#include <QUuid>

// libfp includes
#include <fp/fp-daemon.h>

// Project Includes
#include "task/task.h"
#include "tools/mounter_proxy.h"
#include "tools/mounter_qmp.h"
#include "tools/mounter_router.h"

class TMount : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TMount"_s;

    // Logging
    static inline const QString LOG_EVENT_MOUNTING_DATA_PACK = u"Mounting Data Pack %1"_s;
    static inline const QString LOG_EVENT_MOUNT_INFO_DETERMINED = u"Mount Info: {.filePath = \"%1\", .driveId = \"%2\", .driveSerial = \"%3\"}"_s;
    static inline const QString LOG_EVENT_STOPPING_MOUNT = u"Stopping current mount(s)..."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Mounters
    MounterProxy* mMounterProxy;
    MounterQmp* mMounterQmp;
    MounterRouter* mMounterRouter;

    // Data
    bool mMounting;

    // Properties
    Fp::Daemon mDaemon;
    QUuid mTitleId;
    QString mPath;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TMount(QObject* parent);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    template<typename M>
        requires Qx::any_of<M, MounterProxy, MounterQmp, MounterRouter>
    void initMounter(M*& mounter);

public:
    QString name() const override;
    QStringList members() const override;

    QUuid titleId() const;
    QString path() const;
    Fp::Daemon daemon() const;

    void setTitleId(QUuid titleId);
    void setPath(QString path);
    void setDaemon(Fp::Daemon daemon);

    void perform() override;
    void stop() override;

//-Signals & Slots-------------------------------------------------------------------------------------------------------
private slots:
    void mounterFinishHandler(Qx::Error err);
    void postMount(Qx::Error errorStatus);
};

#endif // TMOUNT_H
