QT += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = CLIFp

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/command/c-link.cpp \
    src/command/c-play.cpp \
    src/command/c-prepare.cpp \
    src/command/c-run.cpp \
    src/command/c-show.cpp \
    src/command.cpp \
    src/controller.cpp \
    src/core.cpp \
    src/driver.cpp \
    src/flashpoint-db.cpp \
    src/flashpoint-install.cpp \
    src/flashpoint-json.cpp \
    src/flashpoint-macro.cpp \
    src/logger.cpp \
    src/main.cpp \
    src/statusrelay.cpp

HEADERS += \
    src/command/c-link.h \
    src/command/c-play.h \
    src/command/c-prepare.h \
    src/command/c-run.h \
    src/command/c-show.h \
    src/command.h \
    src/controller.h \
    src/core.h \
    src/driver.h \
    src/flashpoint-db.h \
    src/flashpoint-install.h \
    src/flashpoint-json.h \
    src/flashpoint-macro.h \
    src/logger.h \
    src/statusrelay.h \
    src/version.h

RC_FILE = resources.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

contains(QT_ARCH, i386) {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static32_0-0-7-9_Qt_5-15-2
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static32_0-0-7-9_Qt_5-15-2d
} else {
    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static64_0-0-7-9_Qt_5-15-2
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lQxW_static64_0-0-7-9_Qt_5-15-2d
}

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

contains(QT_ARCH, i386) {
    win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static32_0-0-7-9_Qt_5-15-2.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static32_0-0-7-9_Qt_5-15-2d.lib
} else {
    win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static64_0-0-7-9_Qt_5-15-2.lib
    else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/QxW_static64_0-0-7-9_Qt_5-15-2d.lib
}

RESOURCES += \
    resources.qrc
