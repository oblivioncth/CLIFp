#ifndef DRIVER_H
#define DRIVER_H

// Shared Library Support
#include "clifp_backend_export.h"

// Qt Includes
#include <QObject>

// Project Includes
#include "kernel/errorcode.h"
#include "kernel/directive.h"

class DriverPrivate;

class CLIFP_BACKEND_EXPORT Driver : public QObject
{
    Q_OBJECT;
    Q_DECLARE_PRIVATE(Driver);

//-Instance Variables------------------------------------------------------------------------------------------------------------
private:
    std::unique_ptr<DriverPrivate> d_ptr;

//-Constructor-------------------------------------------------------------------------------------------------
public:
    Driver(QStringList arguments);

//-Destructor-------------------------------------------------------------------------------------------------
public:
    ~Driver(); // Required for d_ptr destructor to compile

//-Signals & Slots------------------------------------------------------------------------------------------------------------
public slots:
    // Worker main
    void drive();

    // Termination
    void cancelActiveLongTask();
    void quitNow();

signals:
    // Worker status
    void finished(ErrorCode errorCode);

    // Director forwarders
    void asyncDirectiveAccounced(const AsyncDirective& aDirective);
    void syncDirectiveAccounced(const SyncDirective& sDirective);
    void requestDirectiveAccounced(const RequestDirective& rDirective, void* response);
};

#endif // DRIVER_H
