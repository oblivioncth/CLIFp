// Unit Include
#include "controller.h"

// Qt Include
#include <QApplication>
#include <QWindow>

// Project Includes
#include "kernel/driver.h"

//===============================================================================================================
// CONTROLLER
//===============================================================================================================

//-Constructor-------------------------------------------------------------
Controller::Controller(QObject* parent) :
    QObject(parent),
    mStatusRelay(this),
    mExitCode(ErrorCode::NO_ERR)
{
    // Create driver
    Driver* driver = new Driver(QApplication::arguments());
    driver->moveToThread(&mWorkerThread);

    // Connect driver - Operation
    connect(&mWorkerThread, &QThread::started, driver, &Driver::drive); // Thread start causes driver start
    connect(driver, &Driver::finished, this, &Controller::driverFinishedHandler); // Result handling
    connect(driver, &Driver::finished, driver, &QObject::deleteLater); // Have driver clean up itself
    connect(driver, &Driver::finished, &mWorkerThread, &QThread::quit); // Have driver finish cause thread finish
    connect(&mWorkerThread, &QThread::finished, this, &Controller::finisher); // Finish execution when thread quits

    // Connect driver - Status
    connect(driver, &Driver::statusChanged, &mStatusRelay, &StatusRelay::statusChangeHandler);
    connect(driver, &Driver::errorOccured, &mStatusRelay, &StatusRelay::errorHandler);
    connect(driver, &Driver::message, &mStatusRelay, &StatusRelay::messageHandler);
    connect(driver, &Driver::longTaskProgressChanged, &mStatusRelay, &StatusRelay::longTaskProgressHandler);
    connect(driver, &Driver::longTaskTotalChanged, &mStatusRelay, &StatusRelay::longTaskTotalHandler);
    connect(driver, &Driver::longTaskStarted, &mStatusRelay, &StatusRelay::longTaskStartedHandler);
    connect(driver, &Driver::longTaskFinished, &mStatusRelay, &StatusRelay::longTaskFinishedHandler);
    connect(&mStatusRelay, &StatusRelay::longTaskCanceled, driver, &Driver::cancelActiveLongTask);

    // Connect driver - Response Requested  (BlockingQueuedConnection since response must be waited for)
    connect(driver, &Driver::blockingErrorOccured, &mStatusRelay, &StatusRelay::blockingErrorHandler, Qt::BlockingQueuedConnection);
    connect(driver, &Driver::saveFileRequested, &mStatusRelay, &StatusRelay::saveFileRequestHandler, Qt::BlockingQueuedConnection);
    connect(driver, &Driver::itemSelectionRequested, &mStatusRelay, &StatusRelay::itemSelectionRequestHandler, Qt::BlockingQueuedConnection);

    // Connect quit handler
    connect(&mStatusRelay, &StatusRelay::quitRequested, this, &Controller::quitRequestHandler);
    connect(this, &Controller::quit, driver, &Driver::quitNow);
}

//-Destructor-------------------------------------------------------------
Controller::~Controller()
{
    // Just to be safe, but never should be the case
    if(mWorkerThread.isRunning())
    {
        mWorkerThread.quit();
        mWorkerThread.wait();
    }
}

//-Instance Functions-------------------------------------------------------------
//Private:
bool Controller::windowsAreOpen()
{
    // Based on Qt's own check here: https://code.qt.io/cgit/qt/qtbase.git/tree/src/gui/kernel/qwindow.cpp?h=5.15.2#n2710
    // and here: https://code.qt.io/cgit/qt/qtbase.git/tree/src/gui/kernel/qguiapplication.cpp?h=5.15.2#n3629
    QWindowList topWindows = QApplication::topLevelWindows();
    for(const QWindow* window : qAsConst(topWindows))
    {
        if (window->isVisible() && !window->transientParent() && window->type() != Qt::ToolTip)
            return true;
    }
    return false;
}


//Public:
void Controller::run() { mWorkerThread.start(); }

//-Slots--------------------------------------------------------------------------------
//Private:
void Controller::driverFinishedHandler(ErrorCode code) { mExitCode = code; }

void Controller::quitRequestHandler()
{
    // Notify driver to quit if it still exists
    emit quit();

    // Close all top-level windows
    qApp->closeAllWindows();
}

void Controller::finisher()
{
    // Quit once no windows remain
    if(windowsAreOpen())
        connect(qApp, &QGuiApplication::lastWindowClosed, this, [this]() { QApplication::exit(mExitCode); });
    else
        QApplication::exit(mExitCode);
}
