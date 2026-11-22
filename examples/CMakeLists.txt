# One executable per example; each one focuses on a part of the public API.
set(AW_TPL_EXAMPLES
    01_tasks_basics
    02_task_run
    03_when_all
    04_when_any
    05_cancellation
    06_thread_pool
    07_timeout
    08_co_spawn
)

set(_aw_tpl_example_options)
if(AW_TPL_WARNINGS_AS_ERRORS)
    list(APPEND _aw_tpl_example_options STRICT_WARNINGS)
endif()

foreach(example IN LISTS AW_TPL_EXAMPLES)
    aw_add_executable(TARGET aw_tpl_example_${example}
        SOURCES ${example}.cpp
        LINK_LIBRARIES aw::tpl
        ${_aw_tpl_example_options})
endforeach()

add_custom_target(aw_tpl_examples)
foreach(example IN LISTS AW_TPL_EXAMPLES)
    add_dependencies(aw_tpl_examples aw_tpl_example_${example})
endforeach()
