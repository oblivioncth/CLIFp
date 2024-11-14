// Unit Include
#include "frontend/framework.h"

// Qt Includes
#include <QGuiApplication>
#include <QIcon>
#include <QClipboard>

// Qx Includes
#include <qx/utility/qx-helpers.h>

// Project Includes
#include "kernel/driver.h"
#include "_frontend_project_vars.h"

//===============================================================================================================
// SafeDriver
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
SafeDriver::SafeDriver(Driver* driver) : mDriver(driver){}

//-Instance Functions------------------------------------------------------
//Public:
template<typename Functor, typename FunctorReturnType>
bool SafeDriver::invokeMethod(Functor&& function, FunctorReturnType* ret)
{
    if(!mDriver)
    {
        qWarning("Tried using driver when it does not exist!");
        return false;
    }

    return QMetaObject::invokeMethod(mDriver, function, Qt::QueuedConnection, ret);
}

SafeDriver& SafeDriver::operator=(Driver* driver) { mDriver = driver; return *this; }

//===============================================================================================================
// Framework
//===============================================================================================================

//-Constructor-------------------------------------------------------------
//Public:
FrontendFramework::FrontendFramework(QGuiApplication* app) :
    mApp(app),
    mExitCode(0),
    mSystemClipboard(app->clipboard())
{
    /* Turns out when embedding a qrc into static lib, this call is needed to load it. Not sure if this macro
     * must be used in code called directly by the application, or if it can be in purely library code, but
     * it works in this case where the base class of classes derived in application code calls it.
     */
    Q_INIT_RESOURCE(frontend_framework);

    // Prepare app
    mApp->setApplicationName(PROJECT_APP_NAME);
    mApp->setApplicationVersion(PROJECT_VERSION_STR);

    // Register metatypes
    qRegisterMetaType<AsyncDirective>();
    qRegisterMetaType<SyncDirective>();
    qRegisterMetaType<RequestDirective>();

    // Create driver
    Driver* driver = new Driver(mApp->arguments());
    driver->moveToThread(&mWorkerThread);

    // Connect driver - Operation
    connect(&mWorkerThread, &QThread::started, driver, &Driver::drive); // Thread start causes driver start
    connect(driver, &Driver::finished, this, [this](ErrorCode ec){ mExitCode = ec; }); // Result handling
    connect(driver, &Driver::finished, driver, &QObject::deleteLater); // Have driver clean up itself
    connect(driver, &Driver::finished, &mWorkerThread, &QThread::quit); // Have driver finish cause thread finish
    connect(&mWorkerThread, &QThread::finished, this, &FrontendFramework::threadFinishHandler); // Start execution finish when thread quits

    // Connect driver - Directives
    connect(driver, &Driver::asyncDirectiveAccounced, this, &FrontendFramework::asyncDirectiveHandler);
    connect(driver, &Driver::syncDirectiveAccounced, this, &FrontendFramework::syncDirectiveHandler, Qt::BlockingQueuedConnection);
    connect(driver, &Driver::requestDirectiveAccounced, this, &FrontendFramework::requestDirectiveHandler, Qt::BlockingQueuedConnection);

    // Store driver for use later
    mDriver = driver;

    // Setup handling for abrupt closure messages sent through windowing system
    connect(mApp, &QCoreApplication::aboutToQuit, this, [this]{
        /* If this happens and the driver thread is still running, an abrupt closure is about to occur,
         * like from a system shutdown or logoff. We try to ensure a clean shutdown by telling driver
         * to quit here and then blocking until it's done. We have to process the sending of the
         * cross-thread signal manually as the doc's note that automatic event processing is stopped
         * after this signal (aboutToQuit) is emitted.
         */
        if(mWorkerThread.isRunning())
        {
            shutdownDriver();
            mApp->processEvents();
            waitForWorkerThreadFinish(QDeadlineTimer(30'000)); // Timeout just in case
        }
    });

#ifdef __linux__
    if(!updateUserIcons())
        qWarning("Failed to upate user app icons!");
#endif
}

//-Destructor-------------------------------------------------------------
//Public:
FrontendFramework::~FrontendFramework()
{
    // Just to be safe, but never should be the case
    if(mWorkerThread.isRunning())
    {
        mWorkerThread.quit();
        mWorkerThread.wait();
    }

    Q_CLEANUP_RESOURCE(frontend_framework); // Not sure if needed
}

//-Class Functions-----------------------------------------------------------------------------
//Protected:
const QIcon& FrontendFramework::appIconFromResources() { static QIcon ico(u":/frontend/app/CLIFp.ico"_s); return ico; }

//-Instance Functions--------------------------------------------------------------------------
//Protected:
/* This implementation needs to be moved to FrontendGui if this ever drops the QGui dependency */
void FrontendFramework::handleDirective(const DClipboardUpdate& d) { mSystemClipboard->setText(d.text); }

bool FrontendFramework::aboutToExit()
{
    /* Derivatives can override this to do things last minute, and return true,
     * or they can return false in order to delay exit until they manually call exit()
     */
    return true;
}

void FrontendFramework::shutdownDriver() { mDriver.invokeMethod(&Driver::quitNow); }
void FrontendFramework::waitForWorkerThreadFinish(QDeadlineTimer deadline) { mWorkerThread.wait(deadline); }
void FrontendFramework::cancelDriverTask() { mDriver.invokeMethod(&Driver::cancelActiveLongTask); }
bool FrontendFramework::readyToExit() const { return mExitCode && mWorkerThread.isFinished(); }

void FrontendFramework::exit()
{
    // Ignore if driver thread is not finished
    if(!readyToExit())
        return;

    mApp->exit(*mExitCode);
}

//Public:
ErrorCode FrontendFramework::exec()
{
    // Run thread
    mWorkerThread.start();

    // Loop until quit
    return mApp->exec();
}

//-Signals & Slots------------------------------------------------------------------------------------------------------------
//Private Slots:
void FrontendFramework::threadFinishHandler()
{
    if(aboutToExit())
        exit();
}

void FrontendFramework::asyncDirectiveHandler(const AsyncDirective& aDirective)
{
    std::visit([this](const auto& d) {
        handleDirective(d);  // ADL dispatches to the correct handle function
    }, aDirective);
}

void FrontendFramework::syncDirectiveHandler(const SyncDirective& sDirective)
{
    std::visit([this](const auto& d) {
        handleDirective(d);  // ADL dispatches to the correct handle function
    }, sDirective);
}

void FrontendFramework::requestDirectiveHandler(const RequestDirective& rDirective, void* response)
{
    Q_ASSERT(response);
    std::visit([this, response](const auto& d) {
        using ResponseT = typename std::decay_t<decltype(d)>::response_type; // Could use template lambda to avoid decltype
        handleDirective(d, static_cast<ResponseT*>(response));  // ADL dispatches to the correct handle function
    }, rDirective);
}
