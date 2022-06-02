#================= Project Setup ==========================

# CMake
cmake_minimum_required(VERSION 3.21.1)

# Project
project(CLIFp VERSION 0.8 LANGUAGES CXX)
set(TARGET_FP_VERSION 10.1)
set(FORMAL_NAME "CLI Flashpoint")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LC)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UC)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Check if top level
if(${PROJECT_IS_TOP_LEVEL})
    message(STATUS "NOTE: ${PROJECT_NAME} is being configured as a top-level project")
else()
    set(SUB_PROJ_EXCLUDE_FROM_ALL "EXCLUDE_FROM_ALL")
    message(STATUS "NOTE: ${PROJECT_NAME} is being configured as a sub-project")
endif()

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

# General Variables
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TARGET_ARCH x64)
else()
  set(TARGET_ARCH x86)
endif()

#--------------------Setup Paths-------------------

# Cmake scripts
set(FILE_TEMPLATES_PATH "${PROJ_SCRIPTS_PATH}/file_templates")

# Install
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/out/install")

# Package
set(PACKAGE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/out/dist")

#------------Set Global Build Rules----------------

# Clean install when clean target is ran
set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${CMAKE_INSTALL_PREFIX}")

# Versioning
include(VerboseVersioning)
setup_verbose_versioning(PROJECT_VERSION_VERBOSE)

#================= Build =========================

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
fetch_qx(828ad58c37c84f48f2617faac6e8f464de848aa9) # dev commit

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

# Name here needs to be as unique as possible for when this project is inlcuded
# in another via FetchContent or add_subdirectory (prevent target clashes)
set(TARGET_NAME ${PROJECT_NAME_LC}_${PROJECT_NAME_LC})

# Add executable target
qt_add_executable(${TARGET_NAME} WIN32)

# Make alias target so target can be referred to with its friendly
# export name both internally and when part of another build tree
add_executable(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${TARGET_NAME})

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
        Qx::Network
        Qx::Widgets
        Qx::Windows
)

# Configure target properties
set_target_properties(${COMPONENT_TARGET_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME "${PROJECT_NAME}"
)

## Forward select project variables to C++ code
include(ProjectVars)
set_cxx_project_vars(${TARGET_NAME}
    VERSION_STR "\"${PROJECT_VERSION}\""
    SHORT_NAME "\"${PROJECT_NAME}\""
    TARGET_FP_VER_STR "\"${TARGET_FP_VERSION}\""
    EXECUTABLE_NAME "\"CLIFp.exe\""
    APP_NAME "\"${FORMAL_NAME}\""
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

#--------------------Package Config-----------------------

# Create config file
configure_file("${FILE_TEMPLATES_PATH}/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake"
    @ONLY
)

# Create version config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

#================= Install ==========================

# Install executable
install(TARGETS ${TARGET_NAME}
    CONFIGURATIONS Release
    COMPONENT ${TARGET_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ${SUB_PROJ_EXCLUDE_FROM_ALL} # "EXCLUDE_FROM_ALL" if project is not top-level
    RUNTIME DESTINATION bin   
)

# Install package target export
install(EXPORT ${PROJECT_NAME}Targets
    CONFIGURATIONS Release
    COMPONENT ${TARGET_NAME}
    FILE "${PROJECT_NAME}Targets.cmake"
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION cmake
    ${SUB_PROJ_EXCLUDE_FROM_ALL} # "EXCLUDE_FROM_ALL" if project is not top-level
)

# Install package config
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake"
    CONFIGURATIONS Release
    COMPONENT ${TARGET_NAME}
    DESTINATION cmake
    ${SUB_PROJ_EXCLUDE_FROM_ALL} # "EXCLUDE_FROM_ALL" if project is not top-level
)

#========Export For In-tree Builds =================
# For in source builds
export(EXPORT ${PROJECT_NAME}Targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake"
)

#====================== CPack ==============================

set(CPACK_PACKAGE_VENDOR "oblivioncth")
set(CPACK_PACKAGE_DIRECTORY "${PACKAGE_PREFIX}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}_${PROJECT_VERSION_VERBOSE}_${CMAKE_SYSTEM_NAME}_${TARGET_ARCH}")
set(CPACK_GENERATOR "ZIP")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
include(CPack)

