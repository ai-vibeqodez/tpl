include(Tests)

set(AW_TPL_TEST_SOURCES
    test_thread_pool.cpp
    test_task.cpp
    test_when_all.cpp
    test_when_any.cpp
    test_when_any_callbacks.cpp
    test_cancellation.cpp
    test_task_group.cpp
    test_spawn.cpp)

if(AW_TPL_WARNINGS_AS_ERRORS)
    aw_test_target(NAME tpl SOURCES ${AW_TPL_TEST_SOURCES}
        LINK_TARGET aw::tpl STRICT_WARNINGS)
else()
    aw_test_target(NAME tpl SOURCES ${AW_TPL_TEST_SOURCES}
        LINK_TARGET aw::tpl)
endif()
