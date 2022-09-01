function(fetch_quazip git_ref)
    include(FetchContent)

    # Make sure static libs are used
    set(BUILD_SHARED_LIBS OFF)

    # ----- ZLIB ----------------------------------------------------------------------------------------------

    # First, fetch zlib since QuaZip relies on it.
    FetchContent_Declare(
        ZLIB
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.2.11
        OVERRIDE_FIND_PACKAGE # Allows this to be used when QuaZip calls `find_package(ZLIB)`
    )

    # Sadly, zlib's CMake's comparability with FetchContent is (understandably given its age) garbage, as it
    # was written before modern CMake practices emerged, and isn't being updated anymore; therefore, some work
    # must be done to fetch it in a way so that it's usable in the same manner as it would be when find_package()
    # is used (which is done by QuaZip). Because of this, FetchContent_MakeAvailable cannot be used and instead
    # FetchContent_Populate must be used in addition to some manual configuration.

    # Handle old "project" style used by zlib script. This won't affect CMake scripts using a minimum version
    # that already defines this policy's value (i.e. QuaZip) so it doesn't need to be unset later
    set(CMAKE_POLICY_DEFAULT_CMP0048 OLD)

    # Populate zlib build
    FetchContent_GetProperties(ZLIB)
    if(NOT zlib_POPULATED)
        FetchContent_Populate(ZLIB)

        # EXCLUDE_FROM_ALL so that only main zlib library gets built since it's a dependency, ignore examples, etc.
        add_subdirectory(${zlib_SOURCE_DIR} ${zlib_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    # Create find_package redirect config files for zlib. FetchContent_MakeAvailable does this automatically but
    # since FetchContent_Populate is being used instead, this needs to be done here manually
    if(NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/zlib-config.cmake AND
       NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/ZLIBConfig.cmake)
        file(WRITE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/ZLIBConfig.cmake
        [=[
                # Include extras if they exist
                include("${CMAKE_CURRENT_LIST_DIR}/zlib-extra.cmake" OPTIONAL)
                include("${CMAKE_CURRENT_LIST_DIR}/ZLIBExtra.cmake" OPTIONAL)
        ]=])
    endif()

    if(NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/zlib-config-version.cmake AND
       NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/ZLIBConfigVersion.cmake)
        file(WRITE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/ZLIBConfigVersion.cmake
        [=[
                # Version not available, assuming it is compatible
                set(PACKAGE_VERSION_COMPATIBLE TRUE)
        ]=])
    endif()

    # Provide zlib headers to targets that consume this target (have to do this since normally its done by
    # zlibs cmake's install package, which again isn't used here)
    target_include_directories(zlibstatic INTERFACE "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")

    # Create an alias for ZLIB so that it can be referred to by the same name as when it imported via
    # find_package()
    add_library(ZLIB::ZLIB ALIAS zlibstatic)

    # ----- QuaZip --------------------------------------------------------------------------------------------

    # QuaZip's install targets try to forward zlib's install targets, which aren't available since zlib is being
    # fetched and therefore won't have its cmake package scripts, so QuaZip's install configuration must be
    # skipped. This is fine since it won't be used anyway given it is being fetched as well.
    set(QUAZIP_INSTALL OFF)

    FetchContent_Declare(
        QuaZip
        GIT_REPOSITORY https://github.com/stachenov/quazip.git
        GIT_TAG ${git_ref}
    )
    FetchContent_MakeAvailable(QuaZip)
endfunction()
