# Add via ob standard object library
include(OB/Library)

# Originally was gonna do this with an OBJECT library, for a lightly faster compilation,
# but because this includes a header it causes some wack with MOC and redefinitions,
# as MOC compiles the header file twice: here, and by consumers

ob_add_standard_library(${FRONTEND_FRAMEWORK_TARGET_NAME}
    NAMESPACE "${PROJECT_NAMESPACE}"
    ALIAS "${FRONTEND_FRAMEWORK_ALIAS_NAME}"
    TYPE "STATIC"
    HEADERS_API
        FILES
            frontend/framework.h
    IMPLEMENTATION
        frontend/framework.cpp
        frontend/framework_linux.cpp
    RESOURCE "frontend_framework.qrc"
    LINKS
        PUBLIC
            CLIFp::Backend
            ${Qt}::Gui # Needed for QIcon, silly, but technically not an issue
)

# Include ICO hash in the following on Linux
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    include(FrontendFramework)
    app_ico_hash(ico_hash)
    set(COND_ICO_HASH "APP_ICO_HASH" "\"${ico_hash}\"")
else()
    set(COND_ICO_HASH "")
endif()


## Forward select project variables to C++ code
include(OB/CppVars)
ob_add_cpp_vars(${FRONTEND_FRAMEWORK_TARGET_NAME}
    NAME "_frontend_project_vars"
    PREFIX "PROJECT_"
    VARS
        VERSION_STR "\"${PROJECT_VERSION_VERBOSE}\""
        APP_NAME "\"${PROJECT_FORMAL_NAME}\""
        SHORT_NAME "\"${PROJECT_NAME}\""
        ${COND_ICO_HASH}
)
