#================= Common Build =========================

# Allow includes relative to source root
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Get path to resources
set(RESOURCE_PATH "${PROJECT_SOURCE_DIR}/res")

# Add executable target
qt_add_executable(${MAIN_TARGET_NAME} WIN32)

# Make alias target so target can be referred to with its friendly
# export name both internally and when part of another build tree
add_executable(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${MAIN_TARGET_NAME})

set(QT_RESOURCES
    ${RESOURCE_PATH}/resources.qrc
)

# Create source group for QT resources
source_group(TREE "${RESOURCE_PATH}"
    PREFIX "Resource Files"
    FILES ${QT_RESOURCES}
)

# Set app sources
set(CXX_SOURCES
    kernel/core.h
    kernel/core.cpp
    kernel/driver.h
    kernel/driver.cpp
    kernel/errorcode.h
    command/command.h
    command/command.cpp
    command/c-link.h
    command/c-link.cpp
    command/c-play.h
    command/c-play.cpp
    command/c-prepare.h
    command/c-prepare.cpp
    command/c-run.h
    command/c-run.cpp
    command/c-show.cpp
    command/c-show.h
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
    tools/mounter.h
    tools/mounter.cpp
    frontend/statusrelay.h
    frontend/statusrelay.cpp
    controller.h
    controller.cpp
    utility.h
    utility.cpp
    main.cpp
)

# Add sources to target
target_sources(${MAIN_TARGET_NAME}
    PRIVATE
        ${CXX_SOURCES}
        ${QT_RESOURCES}
)

# Link target to libraries
target_link_libraries(${MAIN_TARGET_NAME}
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
        Fp::Flashpoint
        QI-QMP::Qmpi
        QuaZip::QuaZip
        magic_enum::magic_enum
)

# Configure target properties
set_target_properties(${MAIN_TARGET_NAME} PROPERTIES
    EXPORT_NAME "${PROJECT_NAME}"
)

## Forward select project variables to C++ code
include(OB/ProjectVars)
set_cxx_project_vars(${MAIN_TARGET_NAME}
    VERSION_STR "\"${PROJECT_VERSION}\""
    SHORT_NAME "\"${PROJECT_NAME}\""
    TARGET_FP_VER_PFX_STR "\"${TARGET_FP_VERSION_PREFIX}\""
    APP_NAME "\"${FORMAL_NAME}\""
)

################# Windows Build #################
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    # Sources
    target_sources(${MAIN_TARGET_NAME}
        PRIVATE
            command/c-link_win.cpp
            task/t-exec_win.cpp
            task/t-bideprocess.h
            task/t-bideprocess.cpp
            tools/processbider.h
            tools/processbider.cpp
    )

    # Links
    target_link_libraries(${MAIN_TARGET_NAME}
        PRIVATE
            Qx::Windows
    )

    # Target properties
    # Only set the target version on Windows because on Linux it
    # causes the executable to have the version number as a suffix
    # (doesn't seem like this can be disabled). The number is set
    # on the target primarily for posterity anyway as it only tags
    # the binary with the version in a place that doesn't seem to
    # be user visible or really used in any significant way
    set_target_properties(${MAIN_TARGET_NAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
    )

    # Configure target properties
    set_target_properties(${MAIN_TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}"
    )

    # Set target exe details
    include(OB/WinExecutableDetails)
    set_win_executable_details(${MAIN_TARGET_NAME}
        ICON "${RESOURCE_PATH}/app/CLIFp.ico"
        FILE_VER ${PROJECT_VERSION}
        PRODUCT_VER ${TARGET_FP_VERSION_PREFIX}
        COMPANY_NAME "oblivioncth"
        FILE_DESC "CLI for BlueMaxima's Flashpoint"
        INTERNAL_NAME "CLIFp"
        COPYRIGHT "Open Source @ 2022 oblivioncth"
        TRADEMARKS_ONE "All Rights Reserved"
        TRADEMARKS_TWO "GNU AGPL V3"
        ORIG_FILENAME "CLIFp.exe"
        PRODUCT_NAME "${FORMAL_NAME}"
    )
endif()


################# Linux Build #################
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    # Sources
    target_sources(${MAIN_TARGET_NAME}
        PRIVATE
            command/c-link_linux.cpp
            task/t-awaitdocker.h
            task/t-awaitdocker.cpp
            task/t-exec_linux.cpp
    )

    # Links
    target_link_libraries(${MAIN_TARGET_NAME}
        PRIVATE
            Qx::Linux
    )

    # Configure target properties
    set_target_properties(${MAIN_TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME_LC}"
    )
endif()


#================= Install ==========================

# Install executable
install(TARGETS ${MAIN_TARGET_NAME}
    CONFIGURATIONS Release
    COMPONENT ${MAIN_TARGET_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ${SUB_PROJ_EXCLUDE_FROM_ALL} # "EXCLUDE_FROM_ALL" if project is not top-level
    RUNTIME DESTINATION bin
)
