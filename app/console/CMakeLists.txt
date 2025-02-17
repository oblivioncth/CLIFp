#================= Common Build =========================

set(PRETTY_NAME "${PROJECT_NAME}-C")

# Add via ob standard executable
include(OB/Executable)
ob_add_standard_executable(${APP_CONSOLE_TARGET_NAME}
    ALIAS "${APP_CONSOLE_ALIAS_NAME}"
    OUTPUT_NAME "${PRETTY_NAME}"
    SOURCE
        frontend/console.h
        frontend/console.cpp
        frontend/input.h
        frontend/input.cpp
        frontend/progressprinter.h
        frontend/progressprinter.cpp
        main.cpp
    LINKS
        PRIVATE
            CLIFp::FrontendFramework
            magic_enum::magic_enum
    CONFIG STANDARD
)

# Add exe details on Windows
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    include(FrontendFramework)
    set_clip_exe_details(${APP_CONSOLE_TARGET_NAME} ${PRETTY_NAME})
endif()
