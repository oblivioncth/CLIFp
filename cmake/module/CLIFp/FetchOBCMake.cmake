# Sets up OBCMake to be imported via git

# git_ref - Tag, branch name, or commit hash to retrieve. According to CMake docs,
#           a commit hash is preferred for speed and reliability

macro(fetch_ob_cmake git_ref)
    include(FetchContent)
    FetchContent_Declare(OBCMake
        GIT_REPOSITORY "https://github.com/oblivioncth/OBCMake"
        GIT_TAG ${git_ref}
    )
    FetchContent_MakeAvailable(OBCMake)
endmacro()
