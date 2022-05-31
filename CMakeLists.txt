cmake_minimum_required(VERSION 3.21.1)

# Project
project(CLIFp VERSION 0.8 LANGUAGES CXX)
set(TARGET_FP_VERSION 10.1)
set(FORMAL_NAME "CLI for BlueMaxima's Flashpoint")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build augmentation
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
enable_language("RC")

# Add local modules
set(PROJ_SCRIPTS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJ_SCRIPTS_PATH}/module")

# Import Qt
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Network
)

# Disable deprecated code
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)

# Fetch Qx (build and import from source)
include(FetchQx)
fetch_qx(ae48c53243bb1b2140196026bbc72aff3e56fbd9) # dev commit

# Set app sources
set(CXX_SOURCES
    src/command/c-link.h
    src/command/c-link.cpp
    src/command/c-play.h
    src/command/c-play.cpp
    src/command/c-prepare.h
    src/command/c-prepare.cpp
    src/command/c-run.h
    src/command/c-run.cpp
    src/command/c-show.cpp
    src/command/c-show.h
    src/command.h
    src/command.cpp
    src/controller.h
    src/controller.cpp
    src/core.h
    src/core.cpp
    src/driver.h
    src/driver.cpp
    src/flashpoint/fp-db.h
    src/flashpoint/fp-db.cpp
    src/flashpoint/fp-install.h
    src/flashpoint/fp-install.cpp
    src/flashpoint/fp-json.h
    src/flashpoint/fp-json.cpp
    src/flashpoint/fp-macro.h
    src/flashpoint/fp-macro.cpp
    src/logger.h
    src/logger.cpp
    src/statusrelay.h
    src/statusrelay.cpp
    src/main.cpp
)

set(QT_RESOURCES
    res/resources.qrc
)

set(CXX_INCLUDE_PATHS
    include
)

# Add executable target
set(TARGET_NAME ${PROJECT_NAME})
qt_add_executable(${TARGET_NAME} WIN32)

# Add sources to target
target_sources(${TARGET_NAME}
    PRIVATE
        ${CXX_SOURCES}
        ${QT_RESOURCES}
)

# Add external includes to target
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CXX_INCLUDE_PATHS}
)

## Forward select project variables to C++ code
include(ProjectVars)
set_cxx_project_vars(${TARGET_NAME}
    VERSION_STR "\"${PROJECT_VERSION}\""
)

# Set target exe details
include(WinExecutableDetails)
set_win_executable_details(${TARGET_NAME}
    ICON "res/icon/CLIFp.ico"
    FILE_VER ${PROJECT_VERSION}
    PRODUCT_VER ${TARGET_FP_VERSION}
    COMPANY_NAME "oblivioncth"
    FILE_DESC "CLI for BlueMaxima's Flashpoint"
    INTERNAL_NAME "CLIFp"
    COPYRIGHT "Open Source @ 2022 oblivioncth"
    TRADEMARKS_ONE "All Rights Reserved"
    TRADEMARKS_TWO "GNU AGPL V3"
    ORIG_FILENAME "CLIFp.exe"
    PRODUCT_NAME "${FORMAL_NAME}"
)

# Link target to libraries
target_link_libraries(${TARGET_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        Qx::Core
        Qx::Io
)
