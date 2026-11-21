# aw_aggregate_lib()
#
# Facade INTERFACE library that aggregates other INTERFACE libraries.
#
# Usage:
#   include(cmake/Library/AggregateLib)
#   aw_aggregate_lib(NAME cheats COMPONENTS aimbot esp godmode)
#
# Sets: cheats (INTERFACE), ns::cheats (ALIAS)

function(aw_aggregate_lib)
    set(_oneValue NAME)
    set(_multiValue COMPONENTS)
    cmake_parse_arguments(_AGG "${_multiValue}" "${_oneValue}" "${_multiValue}" ${ARGN})

    if(NOT _AGG_NAME)
        message(FATAL_ERROR "aw_aggregate_lib() requires NAME argument")
    endif()
    if(NOT _AGG_COMPONENTS)
        message(FATAL_ERROR "aw_aggregate_lib() requires COMPONENTS argument")
    endif()

    add_library(${_AGG_NAME} INTERFACE)
    if(DEFINED _NS)
        add_library(${_NS}::${_AGG_NAME} ALIAS ${_AGG_NAME})
    else()
        add_library(${_AGG_NAME}::${_AGG_NAME} ALIAS ${_AGG_NAME})
    endif()

    target_link_libraries(${_AGG_NAME} INTERFACE ${_AGG_COMPONENTS})
endfunction()
