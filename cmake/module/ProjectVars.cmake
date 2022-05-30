function(set_cxx_project_vars target)
    # Const variables
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set(GENERATED_NAME "project_vars.h")
    set(GENERATED_PATH "${GENERATED_DIR}/${GENERATED_NAME}")
    set(TEMPLATE_FILE "__project_vars.h.in")


    # Additional Function inputs
    set(oneValueArgs
        FULL_NAME
        SHORT_NAME
        TARGET_FP_VER
    )

    # Parse arguments
    cmake_parse_arguments(PV "" "${oneValueArgs}" "" ${ARGN})

    # Validate input
    foreach(unk_val ${WIN_ED_UNPARSED_ARGUMENTS})
        message(WARNING "Ignoring unrecognized parameter: ${unk_val}")
    endforeach()

    if(${WIN_ED_KEYWORDS_MISSING_VALUES})
        foreach(missing_val ${WIN_ED_UNPARSED_ARGUMENTS})
            message(ERROR "A value for '${missing_val}' must be provided")
        endforeach()
        message(FATAL_ERROR "Not all required values were present!")
    endif()

    # All values are string based and the cmake_parse_argument generated variable
    # names are already correct for generation

    # Generate project_vars.h
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${TEMPLATE_FILE}"
        "${GENERATED_PATH}"
        @ONLY
        NEWLINE_STYLE UNIX
    )

    # Add file to target
    target_sources(${target} PRIVATE "${GENERATED_PATH}")
endfunction()
