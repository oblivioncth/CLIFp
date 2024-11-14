#================= Common Build =========================

# Add via ob standard executable
include(OB/Executable)
ob_add_standard_executable(${APP_GUI_TARGET_NAME}
    NAMESPACE "${PROJECT_NAMESPACE}"
    ALIAS "${APP_GUI_ALIAS_NAME}"
    SOURCE
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
    set_clip_exe_details(${APP_GUI_TARGET_NAME} ${APP_GUI_ALIAS_NAME})
endif()
