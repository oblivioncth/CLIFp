function(set_clip_exe_details tgt name)
    include(OB/WinExecutableDetails)
    ob_set_win_executable_details(${tgt}
        ICON "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../res/app/CLIFp.ico"
        FILE_VER ${PROJECT_VERSION}
        PRODUCT_VER ${TARGET_FP_VERSION_PREFIX}
        COMPANY_NAME "oblivioncth"
        FILE_DESC "CLI for Flashpoint Archive"
        INTERNAL_NAME "${name}"
        COPYRIGHT "Open Source @ 2024 oblivioncth"
        TRADEMARKS_ONE "All Rights Reserved"
        TRADEMARKS_TWO "GNU AGPL V3"
        ORIG_FILENAME "${name}.exe"
        PRODUCT_NAME "${PROJECT_FORMAL_NAME}"
    )
endfunction()

function(app_ico_hash return)
    file(SHA1 "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../res/app/CLIFp.ico" hash)
    set(${return} ${hash} PARENT_SCOPE)
endfunction()
