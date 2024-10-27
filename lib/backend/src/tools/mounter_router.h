#ifndef MOUNTER_ROUTER_H
#define MOUNTER_ROUTER_H

// Qt Includes
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

// Qx Includes
#include <qx/core/qx-error.h>
#include <qx/utility/qx-macros.h>

// Project Includes
#include "kernel/directorate.h"

class QX_ERROR_TYPE(MounterRouterError, "MounterRouterError", 1234)
{
    friend class MounterRouter;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        Failed = 1
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {Failed, u"Failed to mount data pack via router."_s},
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    MounterRouterError(Type t = NoError, const QString& s = {});

//-Instance Functions-------------------------------------------------------------
public:
    bool isValid() const;
    Type type() const;
    QString specific() const;

private:
    Qx::Severity deriveSeverity() const override;
    quint32 deriveValue() const override;
    QString derivePrimary() const override;
    QString deriveSecondary() const override;
};

class MounterRouter : public QObject, public Directorate
{
    Q_OBJECT

//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"Mounter"_s;

    // Error
    static inline const QString ERR_QMP_CONNECTION_ABORT = u"The connection was aborted."_s;
    static inline const QString ERR_QMP_COMMAND = u"Command %1 - [%2] \"%3\""_s;

    // Events - External
    static inline const QString EVENT_ROUTER_RESPONSE = u"Mount.php Response: \"%1\""_s;

    // Events - Internal
    static inline const QString EVENT_MOUNTING_THROUGH_ROUTER = u"Mounting data pack via router..."_s;
    static inline const QString EVENT_REQUEST_SENT = u"Sent request (%1): %2"_s;

    // Connections
    static const int PHP_TRANSFER_TIMEOUT = 30000; // ms

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mMounting;
    int mRouterPort;
    QString mMountValue;

    QNetworkAccessManager mNam;
    QPointer<QNetworkReply> mRouterMountReply;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    explicit MounterRouter(QObject* parent, Director* director);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    void finish(const MounterRouterError& result);

public:
    QString name() const override;
    bool isMounting();

    quint16 routerPort() const;
    QString mountValue() const;

    void setRouterPort(quint16 port);
    void setMountValue(const QString& value);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void mountFinishedHandler(QNetworkReply* reply);

public slots:
    void mount();
    void abort();

signals:
    void mountFinished(MounterRouterError errorState);
};

#endif // MOUNTER_ROUTER_H
