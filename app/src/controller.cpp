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
    mExitCode(0),
    mReadyToExit(false)
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

    // Connect driver - Status/Non-blocking
    connect(driver, &Driver::statusChanged, &mStatusRelay, &StatusRelay::statusChangeHandler);
    connect(driver, &Driver::errorOccurred, &mStatusRelay, &StatusRelay::errorHandler);
    connect(driver, &Driver::longTaskProgressChanged, &mStatusRelay, &StatusRelay::longTaskProgressHandler);
    connect(driver, &Driver::longTaskTotalChanged, &mStatusRelay, &StatusRelay::longTaskTotalHandler);
    connect(driver, &Driver::longTaskStarted, &mStatusRelay, &StatusRelay::longTaskStartedHandler);
    connect(driver, &Driver::longTaskFinished, &mStatusRelay, &StatusRelay::longTaskFinishedHandler);
    connect(&mStatusRelay, &StatusRelay::longTaskCanceled, driver, &Driver::cancelActiveLongTask);
    connect(&mStatusRelay, &StatusRelay::longTaskCanceled, this, &Controller::longTaskCanceledHandler);
    connect(driver, &Driver::clipboardUpdateRequested, &mStatusRelay, &StatusRelay::clipboardUpdateRequestHandler);

    // Connect driver - Response Requests/Blocking (BlockingQueuedConnection since response must be waited for)
    connect(driver, &Driver::message, &mStatusRelay, &StatusRelay::messageHandler, Qt::BlockingQueuedConnection); // Allows optional blocking
    connect(driver, &Driver::blockingErrorOccurred, &mStatusRelay, &StatusRelay::blockingErrorHandler, Qt::BlockingQueuedConnection);
    connect(driver, &Driver::saveFileRequested, &mStatusRelay, &StatusRelay::saveFileRequestHandler, Qt::BlockingQueuedConnection);
    connect(driver, &Driver::existingDirRequested, &mStatusRelay, &StatusRelay::existingDirectoryRequestHandler, Qt::BlockingQueuedConnection);
    connect(driver, &Driver::itemSelectionRequested, &mStatusRelay, &StatusRelay::itemSelectionRequestHandler, Qt::BlockingQueuedConnection);
    connect(driver, &Driver::questionAnswerRequested, &mStatusRelay, &StatusRelay::questionAnswerRequestHandler, Qt::BlockingQueuedConnection);

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

void Controller::longTaskCanceledHandler()
{
    /* A bit of a bodge. Pressing the Cancel button on a progress dialog
     * doesn't count as closing it (it doesn't fire a close event) by the strict definition of
     * QWidget, so here when the progress bar is closed we manually check to see if it was
     * the last window if the application is ready to exit.
     *
     * Normally the progress bar should never still be open by that point, but this is here as
     * a fail-safe as otherwise the application would deadlock when the progress bar is closed
     * via the Cancel button.
     */
    if(mReadyToExit && !windowsAreOpen())
        exit();
}

void Controller::finisher()
{
    // Quit once no windows remain
    if(windowsAreOpen())
    {
        mReadyToExit = true;
        connect(qApp, &QGuiApplication::lastWindowClosed, this, &Controller::exit);
    }
    else
        exit();
}

void Controller::exit() { QApplication::exit(mExitCode); }
