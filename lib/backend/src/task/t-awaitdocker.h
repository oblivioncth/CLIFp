#ifndef TAWAITDOCKER_H
#define TAWAITDOCKER_H

// Project Includes
#include "task/task.h"

// Qt Includes
#include <QProcess>
#include <QTimer>

class QX_ERROR_TYPE(TAwaitDockerError, "TAwaitDockerError", 1260)
{
    friend class TAwaitDocker;
//-Class Enums-------------------------------------------------------------
public:
    enum Type
    {
        NoError = 0,
        DirectQueryFailed = 1,
        ListenFailed = 2,
        StartFailed = 3
    };

//-Class Variables-------------------------------------------------------------
private:
    static inline const QHash<Type, QString> ERR_STRINGS{
        {NoError, u""_s},
        {DirectQueryFailed, u"Failed to directly query docker image status."_s},
        {ListenFailed, u"Failed to start the docker event listener."_s},
        {StartFailed, u"The start of the docker image timed out."_s}
    };

//-Instance Variables-------------------------------------------------------------
private:
    Type mType;
    QString mSpecific;

//-Constructor-------------------------------------------------------------
private:
    TAwaitDockerError(Type t = NoError, const QString& s = {});

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

class TAwaitDocker : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = u"TAwaitDocker"_s;

    // Functional
    static inline const QString DOCKER = u"docker"_s;

    // Logging
    static inline const QString LOG_EVENT_DIRECT_QUERY = u"Checking if docker image '%1' is running directly"_s;
    static inline const QString LOG_EVENT_STARTING_LISTENER = u"Docker image isn't running, starting listener..."_s;
    static inline const QString LOG_EVENT_START_RECEIVED = u"Received docker image start event"_s;
    static inline const QString LOG_EVENT_FINAL_CHECK_PASS = u"The docker image was found to be running after the final timeout check"_s;
    static inline const QString LOG_EVENT_STOPPING_LISTENER = u"Stopping event listener..."_s;

//-Instance Variables------------------------------------------------------------------------------------------------
private:
    // Functional
    QProcess mEventListener;
    QTimer mTimeoutTimer;

    // Data
    QString mImageName;
    uint mTimeout;

//-Constructor----------------------------------------------------------------------------------------------------------
public:
    TAwaitDocker(Core& core);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    TAwaitDockerError imageRunningCheck(bool& running);
    TAwaitDockerError startEventListener();
    void stopEventListening();

public:
    QString name() const override;
    QStringList members() const override;

    QString imageName() const;
    uint timeout() const;

    void setImageName(const QString& imageName);
    void setTimeout(uint msecs);

    void perform() override;
    void stop() override;

//-Signals & Slots------------------------------------------------------------------------------------------------------
private slots:
    void eventDataReceived();
    void timeoutOccurred();

};

#endif // TAWAITDOCKER_H
