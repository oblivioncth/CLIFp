### verbose_versioning.cmake ###

# Include this script as part of a CMakeList.txt used for configuration
# and call the function "setup_verbose_versioning" to get the project's git-based
# verbose version and setup the build scripts to reconfigure if the verbose version is out
# of date.

# Version acquisition function
function(__get_verbose_version repo fallback return)
    find_package(Git)

    if(Git_FOUND)
        # Describe repo
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --match v*.* --dirty --always
            WORKING_DIRECTORY "${repo}"
            COMMAND_ERROR_IS_FATAL ANY
            RESULT_VARIABLE res
            OUTPUT_VARIABLE GIT_DESC
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Command result
        if(NOT ("${GIT_DESC}" STREQUAL ""))
            set(${return} "${GIT_DESC}" PARENT_SCOPE)
        else()
            message(FATAL_ERROR "Git returned a null description!")
        endif()

    elseif(NOT (${fallback} STREQUAL ""))
        set(${return} "${fallback}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Git could not be found! You can define NO_GIT to acknowledge building without verbose versioning.")
    endif()
endfunction()

# Build script usage
if(CMAKE_SCRIPT_MODE_FILE)
    # Get cached verbose version from disk
    if(EXISTS "${VERBOSE_VER_CACHE}")
        file(READ ${VERBOSE_VER_CACHE} CACHED_VERBOSE_VER)
    else()
        set(CACHED_VERBOSE_VER "")
    endif()

    # Get fresh verbose version
    __get_verbose_version("${GIT_REPO}" "${VERSION_FALLBACK}" VERBOSE_VER)

    # Compare values, and update if necessary
    if(NOT ("${CACHED_VERBOSE_VER}" STREQUAL "${VERBOSE_VER}"))
        message(STATUS "Verbose version is out of date")
        # This will update verbose_ver.txt, causing cmake to reconfigure
        file(WRITE ${VERBOSE_VER_CACHE} ${VERBOSE_VER})
    endif()
# Configure usage (make function available)
else()
    function(setup_verbose_versioning return)
        # Handle fallback value
        if(NO_GIT)
            set(VERSION_FALLBACK "v${PROJECT_VERSION}")
        else()
            set(VERSION_FALLBACK "")
        endif()

        # Get verbose version
        __get_verbose_version("${CMAKE_CURRENT_SOURCE_DIR}" "${VERSION_FALLBACK}" VERBOSE_VER)

        # Write to "cache" file
        set(VERBOSE_VER_CACHE ${CMAKE_CURRENT_BINARY_DIR}/verbose_ver.txt)
        file(WRITE ${VERBOSE_VER_CACHE} ${VERBOSE_VER})

        # Add custom target to allow for build time re-check (byproduct important!)
        add_custom_target(
            ${PROJECT_NAME}_verbose_ver_check
            BYPRODUCTS
                ${VERBOSE_VER_CACHE}
            COMMAND
                ${CMAKE_COMMAND}
                "-DVERBOSE_VER_CACHE=${VERBOSE_VER_CACHE}"
                "-DGIT_REPO=${CMAKE_CURRENT_SOURCE_DIR}"
                "-DVERSION_FALLBACK=${VERSION_FALLBACK}"
                "-P" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
            COMMENT
                "Re-checking verbose version..."
            VERBATIM
            USES_TERMINAL
        )

        # This configure_file makes cmake reconfigure dependent on verbose_ver.txt
        configure_file(${VERBOSE_VER_CACHE} ${VERBOSE_VER_CACHE}.old COPYONLY)

        message(STATUS "Verbose Version: ${VERBOSE_VER}")
        set(${return} "${VERBOSE_VER}" PARENT_SCOPE)
    endfunction()
endif()

