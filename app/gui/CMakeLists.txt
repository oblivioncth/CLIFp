#================= Common Build =========================

set(PRETTY_NAME "${PROJECT_NAME}")

# Add via ob standard executable
include(OB/Executable)
ob_add_standard_executable(${APP_GUI_TARGET_NAME}
    ALIAS "${APP_GUI_ALIAS_NAME}"
    OUTPUT_NAME "${PROJECT_NAME}"
    SOURCE
        frontend/assisted/action/action.h
        frontend/assisted/action/a-help.cpp
        frontend/assisted/action/a-play.cpp
        frontend/assisted/action/a-share.cpp
        frontend/assisted/action/a-shortcut.cpp
        frontend/assisted/action/a-update.cpp
        frontend/assisted/action/a-version.cpp
        frontend/assisted/dialog/actionmenudialog.h
        frontend/assisted/dialog/actionmenudialog.cpp
        frontend/assisted/dialog/titleentrydialog.h
        frontend/assisted/dialog/titleentrydialog.cpp
        frontend/gui.h
        frontend/gui.cpp
        main.cpp
    RESOURCE "resources.qrc"
    LINKS
        PRIVATE
            CLIFp::FrontendFramework
            ${Qt}::Widgets
            Qx::Widgets
            magic_enum::magic_enum
    CONFIG STANDARD
    WIN32
)

# Add exe details on Windows
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    include(FrontendFramework)
    set_clip_exe_details(${APP_GUI_TARGET_NAME} ${PRETTY_NAME})
endif()
