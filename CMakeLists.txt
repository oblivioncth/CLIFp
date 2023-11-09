#================= Project Setup ==========================

# CMake
cmake_minimum_required(VERSION 3.24.0...3.26.0)

# Project
# NOTE: DON'T USE TRAILING ZEROS IN VERSIONS
project(CLIFp
    VERSION 0.9.9.1
    LANGUAGES CXX
    DESCRIPTION "Command-line Interface for Flashpoint Archive"
)

# Get helper scripts
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchOBCMake.cmake)
fetch_ob_cmake("v0.3.3")

# Initialize project according to standard rules
include(OB/Project)
ob_standard_project_setup()

# Additional Project Variables
set(PROJECT_FORMAL_NAME "CLI Flashpoint")
set(TARGET_FP_VERSION_PREFIX 12.1)

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
    Gui
    Widgets
    Sql
    Network
)

# Find Qt package
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)
find_package(Qt6 REQUIRED COMPONENTS ${CLIFP_QT_COMPONENTS})

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
    REF "7acb9d0f3e65de2f0a6e91c979b1216e6be7de7d"
    COMPONENTS
        ${CLIFP_QX_COMPONENTS}
)

# Fetch libfp (build and import from source)
include(OB/Fetchlibfp)
ob_fetch_libfp("d27276305defe2fbd98946b23ec9d135ce3ae1bd")

# Fetch QI-QMP (build and import from source)
include(OB/FetchQI-QMP)
ob_fetch_qi_qmp("v0.2.2")

# Fetch QuaZip (build and import from source)
include(OB/FetchQuaZip)
ob_fetch_quazip(
    REF "v1.3"
    QT_MAJOR_VER "6"
)

# Fetch Neargye's Magic Enum
include(OB/FetchMagicEnum)
ob_fetch_magicenum("v0.9.3")

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
