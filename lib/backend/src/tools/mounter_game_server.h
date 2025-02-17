#ifndef MOUNTER_GAME_SERVER_H
#define MOUNTER_GAME_SERVER_H

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

class QX_ERROR_TYPE(MounterGameServerError, "MounterError", 1232)
{
    friend class MounterGameServer;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        ProxyMount = 1,
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {ProxyMount, u"Failed to mount data pack via proxy server."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    MounterGameServerError(Type t = NoError, const QString& s = {});

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

class MounterGameServer : public QObject, public Directorate
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"Mounter"_s;

    // Events - External
    static inline const QString EVENT_GAMESERVER_RESPONSE = u"Game Server Response: \"%1\""_s;

    // Events - Internal
    static inline const QString EVENT_MOUNTING = u"Mounting data pack via game server..."_s;
    static inline const QString EVENT_REQUEST_SENT = u"Sent HTTP request\n"
                                                     "\tOperation: %1\n"
                                                     "\tURL: %2\n"
                                                     "\tData: %3"_s;

    // Connections
    static const int GAME_SERVER_TRANSFER_TIMEOUT = 30000; // ms

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mMounting;
    int mGameServerPort;
    QString mFilePath;

    QNetworkAccessManager mNam;
    QPointer<QNetworkReply> mGameServerMountReply;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    explicit MounterGameServer(QObject* parent, Director* director);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    void finish(const MounterGameServerError& errorState);
    void noteProxyRequest(QNetworkAccessManager::Operation op, const QUrl& url, QByteArrayView data);
    void noteProxyResponse(const QString& response);

public:
    QString name() const override;
    bool isMounting();

    quint16 gameServerPort() const;
    QString filePath() const;

    void setGameServerPort(quint16 port);
    void setFilePath(const QString& path);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void gameServerMountFinishedHandler(QNetworkReply* reply);

public slots:
    void mount();
    void abort();

signals:
    void mountFinished(const MounterGameServerError& errorState);
};

#endif // MOUNTER_GAME_SERVER_H
