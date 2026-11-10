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
FrontendFramework::FrontendFramework(QGuiApplication* app, const QString& name) :
    mApp(app),
    mFrontendName(name),
    mExitCode(0),
    mSystemClipboard(app->clipboard())
{
    /* Turns out when embedding a qrc into static lib, this call is needed to load it. Not sure if this macro
     * must be used in code called directly by the application, or if it can be in purely library code, but
     * it works in this case where the base class of classes derived in application code calls it.
     */
    Q_INIT_RESOURCE(frontend_framework);

    // Prepare app (these also making using QSettings easier)
    mApp->setApplicationName(PROJECT_APP_NAME);
    mApp->setApplicationVersion(PROJECT_VERSION_STR);
    mApp->setOrganizationName(PROJECT_ORGANIZATION_NAME);
    mApp->setOrganizationDomain(PROJECT_ORGANIZATION_NAME + u".com"_s);

    /* Init settings accessor AFTER setting app details
     *
     * This settings accessor is for super basic settings (such as remembering a Window's position) that can
     * easily be discarded (e.g. during a re-install) without issue. If more serious settings are ever needed
     * that should likely be handled in the backend.
     */
    mSettings = new Qx::Settings(this);
    mSettings->setForceConsistentCasing(true);
    mFrontendSettings = mSettings->createGroup(u"frontend/"_s + mFrontendName);

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
QString FrontendFramework::frontendName() const { return mFrontendName; }

/* This implementation needs to be moved to FrontendGui if this ever drops the QGui dependency */
void FrontendFramework::handleDirective(const DClipboardUpdate& d) { mSystemClipboard->setText(d.text); }

void FrontendFramework::handleDirective(const DAssistedMode&, DAssistedMode::Action* response)
{
    // By default, just triggers help
    *response = DAssistedMode::SpecialAction::Help;
}

bool FrontendFramework::aboutToExit()
{
    /* Derivatives can override this to do things last minute, and return true,
     * or they can return false in order to delay exit until they manually call exit()
     */
    return true;
}

/* TODO: There are some lifetime concerns with these functions, particularly with mDriver.
 * Evaulate the timing when they are used and add guards where necessary. There is also an argument
 * that once driver's main function (drive) exits, that it should be considered 100% dead and the signals
 * between here and it should be disconnected to ensure no more communication occurs, especially since it will
 * be basically tore down anyway and not able to do anything of use. Ideally communication shouldn't happen at that
 * point regardless but it's a good default to have.
 *
 * On this same line of thinking we should see if delete later for driver and the thread is "too slow" and if there is
 * a safe way to destory them more immediately; largely, with the goal of ensuring that any remaining pending events that
 * came in after driver emitted "finish()" but before it receives the quit signal.
 *
 * IIRC it's possible for events to come in during that window becuase the connection of 'finished' to 'quit'
 * here is queued, so Driver has to actually finish processing anything that came already before it sees the
 * command to stop its event loop.
 *
 * If we care that much, we may need to see if theres a way to have "quit" be sent instantly and jump the queue, so that
 * the next event immediately after the one that driver was in the middle of handling is the quit signal.
 */
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
