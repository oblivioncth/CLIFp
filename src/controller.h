#ifndef CONTROLLER_H
#define CONTROLLER_H

// Qt Includes
#include <QObject>

// Project Includes
#include "statusrelay.h"
#include "core.h"

class Controller : public QObject
{
    Q_OBJECT
//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    QThread mWorkerThread;
    StatusRelay mStatusRelay;

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
    void quitRequestHandler();
    void finisher(ErrorCode errorCode);

signals:
    void quit();
    void operate();
};

#endif // CONTROLLER_H
