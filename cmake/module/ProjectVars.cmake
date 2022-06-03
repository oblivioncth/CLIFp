# Generates the header file 'project_vars.h' and adds it to the given target via
# 'target_include_directories'. The header will contain macro defintions for each
# key/value pair passed to the function after the target argument; therefore, the
# variable arguments must be even. All keys will be prefixed with "PROJECT_", so
# using uppercase key names is recommended.

function(set_cxx_project_vars target)
    # Const variables
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set(GENERATED_NAME "project_vars.h")
    set(GENERATED_PATH "${GENERATED_DIR}/${GENERATED_NAME}")
    set(TEMPLATE_FILE "__project_vars.h.in")
    set(VAR_PREFIX "PROJECT_")

    # Validate input
    set(__SKIP_ARGS 1) # Skip named arguments ('target')
    math(EXPR __ARGNC "${ARGC} - ${__SKIP_ARGS}")
    math(EXPR __ARGNC_DIV2_REMAINDER "${__ARGNC} % 2")
    if(${__ARGNC_DIV2_REMAINDER} GREATER 0)
        message(FATAL_ERROR "set_cxx_project_vars() requires an even number of arguments following target!")
    endif()

    # Set guard name
    string(TOUPPER ${PROJECT_NAME} __GUARD_NAME)

    # Generate defines
    math(EXPR __LAST_PAIR_INDEX "(${__ARGNC} / 2) - 1")
    foreach(pair RANGE ${__LAST_PAIR_INDEX})
        math(EXPR __KEY_IDX "(${pair} * 2) + ${__SKIP_ARGS}")
        math(EXPR __VALUE_IDX "(${pair} * 2) + 1 + ${__SKIP_ARGS}")
        set(__KEY ${ARGV${__KEY_IDX}})
        set(__VALUE ${ARGV${__VALUE_IDX}})

        if("${__KEY}" MATCHES "[ ]")
            message(FATAL_ERROR "set_cxx_project_vars() a key cannot contain spaces!")
        endif()

        set(__PROJECT_VARIABLES "${__PROJECT_VARIABLES}#define ${VAR_PREFIX}${__KEY} ${__VALUE}\n")
    endforeach()

    # Generate project_vars.h
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${TEMPLATE_FILE}"
        "${GENERATED_PATH}"
        @ONLY
        NEWLINE_STYLE UNIX
    )

    # Add file to target
    target_sources(${target} PRIVATE "${GENERATED_PATH}")
endfunction()
