#================= Common Build =========================

# Pre-configure target
set(CLIFP_SOURCE
    kernel/buildinfo.h
    kernel/core.h
    kernel/core.cpp
    kernel/driver.h
    kernel/driver.cpp
    kernel/errorstatus.h
    kernel/errorstatus.cpp
    command/command.h
    command/command.cpp
    command/c-download.h
    command/c-download.cpp
    command/c-link.h
    command/c-link.cpp
    command/c-play.h
    command/c-play.cpp
    command/c-prepare.h
    command/c-prepare.cpp
    command/c-run.h
    command/c-run.cpp
    command/c-share.h
    command/c-share.cpp
    command/c-show.cpp
    command/c-show.h
    command/c-update.h
    command/c-update.cpp
    command/title-command.h
    command/title-command.cpp
    task/task.h
    task/task.cpp
    task/t-download.h
    task/t-download.cpp
    task/t-exec.h
    task/t-exec.cpp
    task/t-extra.h
    task/t-extra.cpp
    task/t-extract.h
    task/t-extract.cpp
    task/t-generic.h
    task/t-generic.cpp
    task/t-message.h
    task/t-message.cpp
    task/t-mount.h
    task/t-mount.cpp
    task/t-sleep.h
    task/t-sleep.cpp
    tools/blockingprocessmanager.h
    tools/blockingprocessmanager.cpp
    tools/deferredprocessmanager.h
    tools/deferredprocessmanager.cpp
    tools/mounter_proxy.h
    tools/mounter_proxy.cpp
    tools/mounter_qmp.h
    tools/mounter_qmp.cpp
    tools/mounter_router.h
    tools/mounter_router.cpp
    frontend/message.h
    frontend/statusrelay.h
    frontend/statusrelay.cpp
    controller.h
    controller.cpp
    utility.h
    utility.cpp
    main.cpp
)

set(CLIFP_LINKS
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        Qx::Core
        Qx::Io
        Qx::Network
        Qx::Widgets
        Fp::Fp
        QuaZip::QuaZip
        magic_enum::magic_enum
        QI-QMP::Qmpi
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    list(APPEND CLIFP_SOURCE
        command/c-link_win.cpp
        task/t-exec_win.cpp
        task/t-bideprocess.h
        task/t-bideprocess.cpp
    )

    list(APPEND CLIFP_LINKS
        PRIVATE
            Qx::Windows
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    list(APPEND CLIFP_SOURCE
        command/c-link_linux.cpp
        task/t-awaitdocker.h
        task/t-awaitdocker.cpp
        task/t-exec_linux.cpp
    )
    list(APPEND CLIFP_LINKS
        PRIVATE
            Qx::Linux
    )
endif()

# Add via ob standard executable
include(OB/Executable)
ob_add_standard_executable(${APP_TARGET_NAME}
    NAMESPACE "${PROJECT_NAMESPACE}"
    ALIAS "${APP_ALIAS_NAME}"
    SOURCE  ${CLIFP_SOURCE}
    RESOURCE "resources.qrc"
    LINKS ${CLIFP_LINKS}
    CONFIG STANDARD
    WIN32
)

## Forward select project variables to C++ code
include(OB/CppVars)
ob_add_cpp_vars(${APP_TARGET_NAME}
    NAME "project_vars"
    PREFIX "PROJECT_"
    VARS
        VERSION_STR "\"${PROJECT_VERSION_VERBOSE}\""
        SHORT_NAME "\"${APP_ALIAS_NAME}\""
        TARGET_FP_VER_PFX_STR "\"${TARGET_FP_VERSION_PREFIX}\""
        APP_NAME "\"${PROJECT_FORMAL_NAME}\""
)

## Add build info
if(BUILD_SHARED_LIBS)
    set(link_str "Shared")
else()
    set(link_str "Static")
endif()

ob_add_cpp_vars(${APP_TARGET_NAME}
    NAME "_buildinfo"
    PREFIX "BUILDINFO_"
    VARS
        SYSTEM "\"${CMAKE_SYSTEM_NAME}\""
        LINKAGE "\"${link_str}\""
        COMPILER "u\"${CMAKE_CXX_COMPILER_ID}\"_s"
        COMPILER_VER_STR "u\"${CMAKE_CXX_COMPILER_VERSION}\"_s"
)

## Add exe details on Windows
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    # Set target exe details
    include(OB/WinExecutableDetails)
    ob_set_win_executable_details(${APP_TARGET_NAME}
        ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/app/CLIFp.ico"
        FILE_VER ${PROJECT_VERSION}
        PRODUCT_VER ${TARGET_FP_VERSION_PREFIX}
        COMPANY_NAME "oblivioncth"
        FILE_DESC "CLI for Flashpoint Archive"
        INTERNAL_NAME "CLIFp"
        COPYRIGHT "Open Source @ 2022 oblivioncth"
        TRADEMARKS_ONE "All Rights Reserved"
        TRADEMARKS_TWO "GNU AGPL V3"
        ORIG_FILENAME "CLIFp.exe"
        PRODUCT_NAME "${PROJECT_FORMAL_NAME}"
    )
endif()
