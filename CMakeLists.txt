#================= Project Setup ==========================

# CMake
cmake_minimum_required(VERSION 3.24.0...3.26.0)

# Project
# NOTE: DON'T USE TRAILING ZEROS IN VERSIONS
project(CLIFp
    VERSION 0.9.12
    LANGUAGES CXX
    DESCRIPTION "Command-line Interface for Flashpoint Archive"
)

# Get helper scripts
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchOBCMake.cmake)
fetch_ob_cmake("v0.3.7.1")

# Initialize project according to standard rules
include(OB/Project)
ob_standard_project_setup()

# Additional Project Variables
set(PROJECT_FORMAL_NAME "CLI Flashpoint")
set(TARGET_FP_VERSION_PREFIX 13.0)

# Configuration options
# Handled by fetched libs, but set this here formally since they aren't part of the main project
option(BUILD_SHARED_LIBS "Build CLIFp with shared libraries" OFF)

# C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build augmentation
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    enable_language("RC")
endif()

#================= Build =========================

# Import Qt
set(CLIFP_QT_COMPONENTS
    Core
    Core5Compat
    Gui
    Widgets
    Sql
    Network
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
     list(APPEND CLIFP_QT_COMPONENTS WaylandClient) # To enable wayland support
endif()

# Find Qt package
include(OB/Qt)
ob_find_package_qt(REQUIRED COMPONENTS ${CLIFP_QT_COMPONENTS})

# Fetch Qx (build and import from source)
set(CLIFP_QX_COMPONENTS
    Core
    Io
    Network
    Widgets
)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    list(APPEND CLIFP_QX_COMPONENTS Windows)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    list(APPEND CLIFP_QX_COMPONENTS Linux)
endif()

include(OB/FetchQx)
ob_fetch_qx(
    REF "0715db2b21a0cd02a71786184b3fe40bf9e38183"
    COMPONENTS
        ${CLIFP_QX_COMPONENTS}
)

# Fetch libfp (build and import from source)
include(OB/Fetchlibfp)
ob_fetch_libfp("183a479d00235d332aa1046a9b5ba98f62699752")

# Fetch QI-QMP (build and import from source)
include(OB/FetchQI-QMP)
ob_fetch_qi_qmp("5d7d30b061cdcf5f5159f20d7f6e6be81b1b0170")

# Fetch QuaZip (build and import from source)
include(OB/FetchQuaZip)
ob_fetch_quazip(
    REF "v1.3"
    QT_MAJOR_VER "6"
)

# Fetch Neargye's Magic Enum
include(OB/FetchMagicEnum)
ob_fetch_magicenum("v0.9.6")

# Process Targets
set(APP_TARGET_NAME ${PROJECT_NAMESPACE_LC}_${PROJECT_NAMESPACE_LC})
set(APP_ALIAS_NAME ${PROJECT_NAMESPACE})
add_subdirectory(app)

#--------------------Package Config-----------------------

ob_standard_project_package_config(
    COMPATIBILITY "SameMinorVersion"
    CONFIG STANDARD
        TARGET_CONFIGS
            TARGET "${PROJECT_NAMESPACE}::${APP_ALIAS_NAME}" COMPONENT "${APP_ALIAS_NAME}" DEFAULT
)

#================= Install ==========================

ob_standard_project_misc_install()

#====================== CPack ==============================

include(OB/Packaging)
ob_standard_project_package(VENDOR "oblivioncth")
