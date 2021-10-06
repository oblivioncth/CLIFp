#include <QApplication>

#include "version.h"
#include "core.h"
#include "controller.h"

ErrorCode main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // QApplication Object
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // Set application name
    QCoreApplication::setApplicationName(VER_PRODUCTNAME_STR);
    QCoreApplication::setApplicationVersion(VER_FILEVERSION_STR);

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




