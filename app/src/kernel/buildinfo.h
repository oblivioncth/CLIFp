#ifndef BUILDINFO_H
#define BUILDINFO_H

// Qt Includes
#include <QString>
#include <QVersionNumber>

struct BuildInfo
{
    enum System{Windows, Linux};
    enum Linkage{Static, Shared};

    System system;
    Linkage linkage;
    QString compiler;
    QVersionNumber compilerVersion;
};

#endif // BUILDINFO_H
