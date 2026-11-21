# Tests use remote Tests.cmake from FetchContent (added to CMAKE_MODULE_PATH in root CMakeLists.txt)
include(Tests)

set(AW_TPL_TEST_SOURCES
    test_thread_pool.cpp
    test_task.cpp
    test_when_all.cpp
    test_when_any.cpp
    test_when_any_callbacks.cpp
    test_cancellation.cpp
    test_task_group.cpp
    test_spawn.cpp
)

aw_test_target(NAME tpl SOURCES ${AW_TPL_TEST_SOURCES} LINK_TARGET aw::tpl)
