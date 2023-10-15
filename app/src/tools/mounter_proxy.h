#ifndef MOUNTER_PROXY_H
#define MOUNTER_PROXY_H

// Qt Includes
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

// Qx Includes
#include <qx/core/qx-error.h>
#include <qx/utility/qx-macros.h>

class QX_ERROR_TYPE(MounterProxyError, "MounterError", 1232)
{
    friend class MounterProxy;
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
    MounterProxyError(Type t = NoError, const QString& s = {});

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

class MounterProxy : public QObject
{
    Q_OBJECT
//-Class Variables------------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"Mounter"_s;

    // Events - External
    static inline const QString EVENT_PROXY_RESPONSE = u"Proxy Response: \"%1\""_s;

    // Events - Internal
    static inline const QString EVENT_MOUNTING = u"Mounting data pack via proxy server..."_s;
    static inline const QString EVENT_REQUEST_SENT = u"Sent HTTP request\n"
                                                     "\tOperation: %1\n"
                                                     "\tURL: %2\n"
                                                     "\tData: %3"_s;

    // Connections
    static const int PROXY_TRANSFER_TIMEOUT = 30000; // ms

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    bool mMounting;
    int mProxyServerPort;
    QString mFilePath;

    QNetworkAccessManager mNam;
    QPointer<QNetworkReply> mProxyMountReply;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    explicit MounterProxy(QObject* parent = nullptr);

//-Instance Functions---------------------------------------------------------------------------------------------------------
private:
    void finish(const MounterProxyError& errorState);
    void noteProxyRequest(QNetworkAccessManager::Operation op, const QUrl& url, QByteArrayView data);
    void noteProxyResponse(const QString& response);

public:
    bool isMounting();

    quint16 proxyServerPort() const;
    QString filePath() const;

    void setProxyServerPort(quint16 port);
    void setFilePath(const QString& path);

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void proxyMountFinishedHandler(QNetworkReply* reply);

public slots:
    void mount();
    void abort();

signals:
    void eventOccurred(QString name, const QString& event);
    void errorOccurred(QString name, MounterProxyError errorMessage);
    void mountFinished(MounterProxyError errorState);

    // For now these just cause a busy state
    void mountProgress(qint64 progress);
    void mountProgressMaximumChanged(qint64 maximum);
};

#endif // MOUNTER_PROXY_H
