QT       += core gui sql network

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
    src/c-play.cpp \
    src/c-prepare.cpp \
    src/c-run.cpp \
    src/c-shortcut.cpp \
    src/c-show.cpp \
    src/command.cpp \
    src/core.cpp \
    src/flashpoint-install.cpp \
    src/logger.cpp \
    src/main.cpp

HEADERS += \
    src/c-play.h \
    src/c-prepare.h \
    src/c-run.h \
    src/c-shortcut.h \
    src/c-show.h \
    src/command.h \
    src/core.h \
    src/flashpoint-install.h \
    src/logger.h \
    src/version.h

RC_FILE = resources.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

contains(QT_ARCH, i386) {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQx_static32_0-0-5-0_Qt_5-15-2
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQx_static32_0-0-5-0_Qt_5-15-2d
} else {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQx_static64_0-0-5-0_Qt_5-15-2
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQx_static64_0-0-5-0_Qt_5-15-2d
}

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

contains(QT_ARCH, i386) {
    win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static32_0-0-5-0_Qt_5-15-2.a
    else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static32_0-0-5-0_Qt_5-15-2d.a
    else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static32_0-0-5-0_Qt_5-15-2.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static32_0-0-5-0_Qt_5-15-2d.lib
} else {
    win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static64_0-0-5-0_Qt_5-15-2.a
    else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/libQx_static64_0-0-5-0_Qt_5-15-2d.a
    else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static64_0-0-5-0_Qt_5-15-2.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/Qx_static64_0-0-5-0_Qt_5-15-2d.lib
}
