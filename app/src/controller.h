#ifndef CONTROLLER_H
#define CONTROLLER_H

// Qt Includes
#include <QObject>
#include <QThread>

// Project Includes
#include "frontend/statusrelay.h"
#include "kernel/director.h"

class Controller : public QObject
{
    Q_OBJECT
//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    QThread mWorkerThread;
    StatusRelay mStatusRelay;
    ErrorCode mExitCode;
    bool mReadyToExit;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    explicit Controller(QObject* parent = nullptr);

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    ~Controller();

//-Instance Functions------------------------------------------------------------------------------------------------------
private:
    bool windowsAreOpen();

public:
    void run();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void driverFinishedHandler(ErrorCode code);
    void quitRequestHandler();
    void longTaskCanceledHandler();
    void finisher();
    void exit();

signals:
    void quit();
};

#endif // CONTROLLER_H
