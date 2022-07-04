// Qt Includes
#include <QApplication>

// Project Includes
#include "core.h"
#include "controller.h"
#include "project_vars.h"

ErrorCode main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------

    // QApplication Object
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // Set application name
    QCoreApplication::setApplicationName(PROJECT_APP_NAME);
    QCoreApplication::setApplicationVersion(PROJECT_VERSION_STR);

    // Register metatypes
    qRegisterMetaType<Core::Error>();
    qRegisterMetaType<Core::BlockingError>();

    // Create application controller
    Controller appController(qApp->instance());

    // Start driver
    appController.run();

    // Start event loop
    return app.exec();
}




