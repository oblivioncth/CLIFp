#ifndef TAWAITDOCKER_H
#define TAWAITDOCKER_H

// Project Includes
#include "task/task.h"

// Qt Includes
#include <QProcess>
#include <QTimer>

class TAwaitDocker : public Task
{
    Q_OBJECT;
//-Class Variables-------------------------------------------------------------------------------------------------
private:
    // Meta
    static inline const QString NAME = QStringLiteral("TAwaitDocker");

    // Functional
    static inline const QString DOCKER = QStringLiteral("docker");

    // Logging
    static inline const QString LOG_EVENT_DIRECT_QUERY = "Checking if docker image '%1' is running directly";
    static inline const QString LOG_EVENT_STARTING_LISTENER = "Docker image isn't running, starting listener...";
    static inline const QString LOG_EVENT_START_RECEIVED = "Received docker image start event";
    static inline const QString LOG_EVENT_FINAL_CHECK_PASS = "The docker image was found to be running after the final timeout check";
    static inline const QString LOG_EVENT_STOPPING_LISTENER = "Stopping event listener...";

    // Errors
    static inline const QString ERR_DIRECT_QUERY = "Failed to directly query docker image status.";
    static inline const QString ERR_CANT_LISTEN = "Failed to start the docker event listener.";
    static inline const QString ERR_DOCKER_DIDNT_START = "The start of docker image '%1' timed out!";

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
    TAwaitDocker(QObject* parent = nullptr);

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    ErrorCode imageRunningCheck(bool& running);
    ErrorCode startEventListener();
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
