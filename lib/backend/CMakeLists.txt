#================= Common Build =========================

# Pre-configure target
set(BACKEND_API
    kernel/directive.h
    kernel/driver.h
    kernel/errorcode.h
)

set(BACKEND_IMPLEMENTATION
    kernel/buildinfo.h
    kernel/core.h
    kernel/core.cpp
    kernel/director.h
    kernel/director.cpp
    kernel/directorate.h
    kernel/directorate.cpp
    kernel/driver_p.h
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
    task/t-titleexec.h
    task/t-titleexec.cpp
    task/t-titleexec_win.cpp
    tools/archiveaccess.h
    tools/archiveaccess.cpp
    tools/blockingprocessmanager.h
    tools/blockingprocessmanager.cpp
    tools/deferredprocessmanager.h
    tools/deferredprocessmanager.cpp
    tools/mounter_game_server.h
    tools/mounter_game_server.cpp
    tools/mounter_qmp.h
    tools/mounter_qmp.cpp
    tools/mounter_router.h
    tools/mounter_router.cpp
    utility.h
)

set(BACKEND_LINKS
    PRIVATE
        Qt6::Sql
        Qt6::Network
        Qx::Io
        Qx::Network
        Fp::Fp
        QuaZip::QuaZip
        magic_enum::magic_enum
        QI-QMP::Qmpi
    PUBLIC
        Qt6::Core
        Qx::Core
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    list(APPEND BACKEND_IMPLEMENTATION
        command/c-link_win.cpp
        task/t-exec_win.cpp
    )

    list(APPEND BACKEND_LINKS
        PRIVATE
            Qx::Windows
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    list(APPEND BACKEND_IMPLEMENTATION
        command/c-link_linux.cpp
        task/t-awaitdocker.h
        task/t-awaitdocker.cpp
        task/t-exec_linux.cpp
    )
    list(APPEND BACKEND_LINKS
        PRIVATE
            Qx::Linux
    )
endif()

# Add via ob standard library
include(OB/Library)
ob_add_standard_library(${BACKEND_TARGET_NAME}
    NAMESPACE "${PROJECT_NAMESPACE}"
    ALIAS "${BACKEND_ALIAS_NAME}"
    EXPORT_HEADER
        PATH "${PROJECT_NAMESPACE_LC}_${BACKEND_ALIAS_NAME_LC}_export.h"
    HEADERS_API
        FILES ${BACKEND_API}
    IMPLEMENTATION
        ${BACKEND_IMPLEMENTATION}
    LINKS
        ${BACKEND_LINKS}
)

## Forward select project variables to C++ code
include(OB/CppVars)
ob_add_cpp_vars(${BACKEND_TARGET_NAME}
    NAME "_backend_project_vars"
    PREFIX "PROJECT_"
    VARS
        VERSION_STR "\"${PROJECT_VERSION_VERBOSE}\""
        SHORT_NAME "\"${PROJECT_NAME}\""
        TARGET_FP_VER_PFX_STR "\"${TARGET_FP_VERSION_PREFIX}\""
)

## Add build info
if(BUILD_SHARED_LIBS)
    set(link_str "Shared")
else()
    set(link_str "Static")
endif()

ob_add_cpp_vars(${BACKEND_TARGET_NAME}
    NAME "_buildinfo"
    PREFIX "BUILDINFO_"
    VARS
        SYSTEM "\"${CMAKE_SYSTEM_NAME}\""
        LINKAGE "\"${link_str}\""
        COMPILER "u\"${CMAKE_CXX_COMPILER_ID}\"_s"
        COMPILER_VER_STR "u\"${CMAKE_CXX_COMPILER_VERSION}\"_s"
)
