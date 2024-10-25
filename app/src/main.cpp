// Qt Includes
#include <QApplication>

// Project Includes
#include "kernel/directive.h"
#include "controller.h"
#ifdef __linux__
    #include "utility.h"
#endif
#include "project_vars.h"

int main(int argc, char *argv[])
{
    //-Basic Application Setup-------------------------------------------------------------

    // QApplication Object
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // Set application name
    app.setApplicationName(PROJECT_APP_NAME);
    app.setApplicationVersion(PROJECT_VERSION_STR);

#ifdef __linux__
    // Set application icon
    app.setWindowIcon(Utility::appIconFromResources());
#endif

    // Register metatypes
    qRegisterMetaType<AsyncDirective>();
    qRegisterMetaType<SyncDirective>();

    // Create application controller
    Controller appController(&app);

    // Start driver
    appController.run();

    // Start event loop
    return app.exec();
}




