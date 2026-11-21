# aw_header_only_lib()
#
# Header-only INTERFACE library with correct build/install include paths.
#
# Usage:
#   include(cmake/Library/HeaderOnlyLib)
#   aw_header_only_lib(NAME mylib)
#   aw_header_only_lib(NAME mylib INCLUDE_DIRS ${CMAKE_SOURCE_DIR})
#
# Sets: mylib (INTERFACE), ns::mylib (ALIAS)

function(aw_header_only_lib)
    set(_oneValue NAME)
    set(_multiValue INCLUDE_DIRS)
    cmake_parse_arguments(_HLIB "${_multiValue}" "${_oneValue}" "${_multiValue}" ${ARGN})

    if(NOT _HLIB_NAME)
        message(FATAL_ERROR "aw_header_only_lib() requires NAME argument")
    endif()

    add_library(${_HLIB_NAME} INTERFACE)
    if(DEFINED _NS)
        add_library(${_NS}::${_HLIB_NAME} ALIAS ${_HLIB_NAME})
    else()
        add_library(${_HLIB_NAME}::${_HLIB_NAME} ALIAS ${_HLIB_NAME})
    endif()

    # Default: inc/ relative to current source directory
    if(NOT _HLIB_INCLUDE_DIRS)
        set(_HLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/inc)
    endif()

    target_include_directories(${_HLIB_NAME} INTERFACE
        $<BUILD_INTERFACE:${_HLIB_INCLUDE_DIRS}>
    )
endfunction()
