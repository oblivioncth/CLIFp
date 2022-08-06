function(set_win_executable_details target)
    # Const variables
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/rc_gen")
    set(GENERATED_NAME "resources.rc")
    set(GENERATED_PATH "${GENERATED_DIR}/${GENERATED_NAME}")
    set(TEMPLATE_FILE "__resources.rc.in")

    # Additional Function inputs
    set(oneValueArgs
        ICON
        FILE_VER
        PRODUCT_VER
        COMPANY_NAME
        FILE_DESC
        INTERNAL_NAME
        COPYRIGHT
        TRADEMARKS_ONE
        TRADEMARKS_TWO
        ORIG_FILENAME
        PRODUCT_NAME
    )

    # Parse arguments
    cmake_parse_arguments(WIN_ED "" "${oneValueArgs}" "" ${ARGN})

    # Validate input
    foreach(unk_val ${WIN_ED_UNPARSED_ARGUMENTS})
        message(WARNING "Ignoring unrecognized parameter: ${unk_val}")
    endforeach()

    if(${WIN_ED_KEYWORDS_MISSING_VALUES})
        foreach(missing_val ${WIN_ED_KEYWORDS_MISSING_VALUES})
            message(ERROR "A value for '${missing_val}' must be provided")
        endforeach()
        message(FATAL_ERROR "Not all required values were present!")
    endif()

    # Determine absolute icon path (relative to caller)
    cmake_path(ABSOLUTE_PATH WIN_ED_ICON
        BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        NORMALIZE
        OUTPUT_VARIABLE __ABS_ICON_PATH
    )

    # Determine relative icon path (relative to generated rc file)
    cmake_path(RELATIVE_PATH __ABS_ICON_PATH
        BASE_DIRECTORY "${GENERATED_DIR}"
        OUTPUT_VARIABLE EXE_ICON
    )

    # Set binary file and product versions
    string(REPLACE "." "," VER_FILEVERSION ${WIN_ED_FILE_VER})
    string(REPLACE "." "," VER_PRODUCTVERSION ${WIN_ED_PRODUCT_VER})

    # Set string based values
    set(VER_COMPANYNAME_STR "${WIN_ED_COMPANY_NAME}")
    set(VER_FILEDESCRIPTION_STR "${WIN_ED_FILE_DESC}")
    set(VER_FILEVERSION_STR "${WIN_ED_FILE_VER}")
    set(VER_INTERNALNAME_STR "${WIN_ED_INTERNAL_NAME}")
    set(VER_LEGALCOPYRIGHT_STR "${WIN_ED_COPYRIGHT}")
    set(VER_LEGALTRADEMARKS1_STR "${WIN_ED_TRADEMARKS_ONE}")
    set(VER_LEGALTRADEMARKS2_STR "${WIN_ED_TRADEMARKS_TWO}")
    set(VER_ORIGINALFILENAME_STR "${WIN_ED_ORIG_FILENAME}")
    set(VER_PRODUCTNAME_STR "${WIN_ED_PRODUCT_NAME}")
    set(VER_PRODUCTVERSION_STR "${WIN_ED_PRODUCT_VER}")

    # Generate resources.rc
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${TEMPLATE_FILE}"
        "${GENERATED_PATH}"
        @ONLY
        NEWLINE_STYLE UNIX
    )

    # Add file to target
    target_sources(${target} PRIVATE "${GENERATED_PATH}")
endfunction()
