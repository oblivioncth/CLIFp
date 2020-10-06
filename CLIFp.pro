QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = CLIFp


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/flashpointinstall.cpp \
    src/logger.cpp \
    src/main.cpp

HEADERS += \
    src/flashpointinstall.h \
    src/logger.h \
    src/version.h

RC_FILE = resources.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

contains(QT_ARCH, i386) {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQx_static32_0-0-2-9_Qt_5-15-0
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQx_static32_0-0-2-9_Qt_5-15-0d
} else {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQx_static64_0-0-2-9_Qt_5-15-0
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQx_static64_0-0-2-9_Qt_5-15-0d
}

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

contains(QT_ARCH, i386) {
    win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static32_0-0-2-9_Qt_5-15-0.a
    else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static32_0-0-2-9_Qt_5-15-0d.a
    else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static32_0-0-2-9_Qt_5-15-0.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static32_0-0-2-9_Qt_5-15-0d.lib
} else {
    win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static64_0-0-2-9_Qt_5-15-0.a
    else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static64_0-0-2-9_Qt_5-15-0d.a
    else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static64_0-0-2-9_Qt_5-15-0.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static64_0-0-2-9_Qt_5-15-0d.lib
}
