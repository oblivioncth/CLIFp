// Unit Include
#include "controller.h"

// Qt Include
#include <QApplication>
#include <QWindow>

// Project Includes
#include "driver.h"

// Windows Includes
#include <qx_windows.h>
#include <processenv.h>

//===============================================================================================================
// CONTROLLER
//===============================================================================================================

//-Constructor-------------------------------------------------------------
Controller::Controller(QObject* parent)
    : QObject(parent),
    mStatusRelay(this)
{
    // Create driver
    Driver* driver = new Driver(QApplication::arguments(), QString::fromStdWString(std::wstring(GetCommandLineW())));
    driver->moveToThread(&mWorkerThread);

    // Connect driver - Operation
    connect(&mWorkerThread, &QThread::finished, driver, &QObject::deleteLater);
    connect(this, &Controller::operate, driver, &Driver::drive);
    connect(driver, &Driver::finished, this, &Controller::finisher, Qt::QueuedConnection);
    // This is in main thread so let event loop finish its current cycle before finishing with QueuedConnection

    // Connect driver - Status (QueuedConnection is implicit)
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
    connect(driver, &Driver::authenticationRequired, &mStatusRelay, &StatusRelay::authenticationHandler, Qt::BlockingQueuedConnection);

    // Connect quit handler
    connect(&mStatusRelay, &StatusRelay::quitRequested, this, &Controller::quitRequestHandler);
    connect(this, &Controller::quit, driver, &Driver::quitNow);

    // Start thread
    mWorkerThread.start();
}

//-Desctructor-------------------------------------------------------------
Controller::~Controller()
{
    // Stop thread
    mWorkerThread.quit();
    mWorkerThread.wait();
}

//-Instance Functions-------------------------------------------------------------
//Private:
bool Controller::windowsAreOpen()
{
    // Based on Qt's down check here: https://code.qt.io/cgit/qt/qtbase.git/tree/src/gui/kernel/qwindow.cpp?h=5.15.2#n2710
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
void Controller::run() { emit operate(); }

//-Slots--------------------------------------------------------------------------------
//Private:
void Controller::quitRequestHandler()
{
    // Notify driver to quit if it still exists
    emit quit();

    // Close all top-level windows
    qApp->closeAllWindows();
}

void Controller::finisher(ErrorCode errorCode)
{
    // Quit once no windows remain
    if(windowsAreOpen())
        connect(qApp, &QGuiApplication::lastWindowClosed, [errorCode]() { QApplication::exit(errorCode); });
    else
        QApplication::exit(errorCode);
}
