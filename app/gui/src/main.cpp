// Qt Includes
#include <QApplication>

// Project Includes
#include "frontend/gui.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    FrontendGui frontend(&app);
    return frontend.exec();
}

