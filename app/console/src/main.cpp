// Qt Includes
#include <QGuiApplication> // Seems odd, but that's just what we need for this

// Project Includes
#include "frontend/console.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    FrontendConsole frontend(&app);
    return frontend.exec();
}

