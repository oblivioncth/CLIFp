#ifndef FRAMEWORK_H
#define FRAMEWORK_H

// Qt Includes
#include <QObject>
#include <QThread>
#include <QPointer>

// Project Includes
#include "kernel/errorcode.h"
#include "kernel/directive.h"

class QGuiApplication;
class Driver;
class QClipboard;

class SafeDriver
{
    /*  This whole class exists to allow Framework to control Driver without using signals, safely.
     *  Call it overkill, but it makes Framework itself cleaner overall.
     *
     *  Specifically, this ensure that the Driver pointer is valid and only allows using it to invoke
     *  methods in drivers thread, which prevents accidental direct method use in Framework's thread.
     */
private:
    QPointer<Driver> mDriver;

public:
    SafeDriver(Driver* driver = nullptr);

    // This signature needs to be tweaked if we ever need the version that can pass method arguments added in Qt 6.7
    template<typename Functor, typename FunctorReturnType = void>
    bool invokeMethod(Functor&& function, FunctorReturnType* ret = nullptr);
    SafeDriver& operator=(Driver* driver);
};

class FrontendFramework : public QObject
{
    Q_OBJECT
//-Instance Variables----------------------------------------------------------------------------------------------
private:
    QThread mWorkerThread;
    QGuiApplication* mApp;
    SafeDriver mDriver;
    std::optional<ErrorCode> mExitCode;
    QClipboard* mSystemClipboard;

//-Constructor-------------------------------------------------------------------------------------------------------
public:
    explicit FrontendFramework(QGuiApplication* app); // Can be QCoreApplication* if dependency on QGui for QIcon et. al. is ever broken

//-Destructor----------------------------------------------------------------------------------------------------------
public:
    virtual ~FrontendFramework();

//-Class Functions---------------------------------------------------------------------------------------------------------
protected:
#ifdef __linux__
    static bool updateUserIcons();
#endif
    static const QIcon& appIconFromResources();

//-Instance Functions------------------------------------------------------------------------------------------------------
protected:
    // Async directive handlers
    virtual void handleDirective(const DMessage& d) = 0;
    virtual void handleDirective(const DError& d) = 0;
    virtual void handleDirective(const DProcedureStart& d) = 0;
    virtual void handleDirective(const DProcedureStop& d) = 0;
    virtual void handleDirective(const DProcedureProgress& d) = 0;
    virtual void handleDirective(const DProcedureScale& d) = 0;
    virtual void handleDirective(const DClipboardUpdate& d);
    virtual void handleDirective(const DStatusUpdate& d) = 0;

    // Sync directive handlers
    virtual void handleDirective(const DBlockingMessage& d) = 0;

    // Request directive handlers
    virtual void handleDirective(const DBlockingError& d, DBlockingError::Choice* response) = 0;
    virtual void handleDirective(const DSaveFilename& d, QString* response) = 0;
    virtual void handleDirective(const DExistingDir& d, QString* response) = 0;
    virtual void handleDirective(const DItemSelection& d, QString* response) = 0;
    virtual void handleDirective(const DYesOrNo& d, bool* response) = 0;

    // Control
    virtual bool aboutToExit();
    void shutdownDriver();
    void cancelDriverTask();
    void waitForWorkerThreadFinish(QDeadlineTimer deadline = QDeadlineTimer::Forever);
    bool readyToExit() const;
    void exit();

public:
    ErrorCode exec();

//-Signals & Slots------------------------------------------------------------------------------------------------------------
private slots:
    void threadFinishHandler();
    void asyncDirectiveHandler(const AsyncDirective& aDirective);
    void syncDirectiveHandler(const SyncDirective& sDirective);
    void requestDirectiveHandler(const RequestDirective& rDirective, void* response);
};

#endif // FRAMEWORK_H
