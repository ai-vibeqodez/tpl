cmake_minimum_required(VERSION 3.23)

# --- Fetch reusable CMake modules from remote template repository ---
include(FetchContent)

FetchContent_Declare(
    aw_cmake_modules
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../template-cpp
)

FetchContent_MakeAvailable(aw_cmake_modules)

# Add remote cmake directory to module search path
list(APPEND CMAKE_MODULE_PATH "${aw_cmake_modules_SOURCE_DIR}/cmake")

# --- Project configuration (all from remote) ---
include(ProjectConfig)

aw_project_config(NAME tpl NS aw::)

project(${AW_PROJECT_NAME} VERSION ${AW_PROJECT_VERSION} LANGUAGES CXX)

# --- Find dependencies (before using them) ---
find_package(Threads REQUIRED)

# --- Additional cmake modules (must come after project() and after Threads is found) ---
include(CXX23Runtime)
include(Compiler)

# Check C++23 runtime support
aw_check_cxx23_runtime()

# --- Top-level detection and basic setup ---
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(TPL_IS_TOP_LEVEL ON)
else()
    set(TPL_IS_TOP_LEVEL OFF)
endif()

# --- Options ---
option(AW_TPL_BUILD_EXAMPLES "Build aw::tpl example applications" ${TPL_IS_TOP_LEVEL})
option(AW_TPL_BUILD_TESTS "Build aw::tpl unit tests" ${TPL_IS_TOP_LEVEL})
option(AW_TPL_INSTALL "Generate install rules and package config for aw::tpl" ${TPL_IS_TOP_LEVEL})
option(AW_TPL_WARNINGS_AS_ERRORS "Build with strict warnings treated as errors" OFF)
option(AW_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
set(AW_SANITIZE "none" CACHE STRING "Sanitizer to enable: none, address (ASan+UBSan) or thread (TSan)")
set_property(CACHE AW_SANITIZE PROPERTY STRINGS none address thread)

# --- Sanitizer/coverage validation ---
if(NOT AW_SANITIZE STREQUAL "none")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "AW_SANITIZE requires GCC or Clang")
    endif()
    if(AW_ENABLE_COVERAGE)
        message(FATAL_ERROR "AW_SANITIZE cannot be combined with AW_ENABLE_COVERAGE")
    endif()
endif()

# --- Core library ---
include(CoreLibrary)

set(AW_TPL_SOURCES
    src/operation_canceled_exception.cpp
    src/timeout_exception.cpp
    src/threading/cancellation_token.cpp
    src/threading/cancellation_token_source.cpp
    src/threading/thread_pool.cpp
    src/threading/detail/cancellation_state.cpp
    src/threading/detail/delay_awaiter.cpp
    src/threading/tasks/async_sleep.cpp
    src/threading/tasks/task_already_moved_exception.cpp
    src/threading/tasks/task_group.cpp
    src/threading/tasks/task_void.cpp
    src/threading/tasks/detail/cancelable_sleep_awaiter.cpp
    src/threading/tasks/detail/completion_awaiter.cpp
    src/threading/tasks/detail/final_awaiter.cpp
    src/threading/tasks/detail/initial_schedule_awaiter.cpp
    src/threading/tasks/detail/promise_void.cpp
    src/threading/tasks/detail/task_state_base.cpp
    src/threading/tasks/detail/when_any_awaiter.cpp
    src/threading/tasks/detail/when_any_state.cpp
    src/threading/tasks/detail/when_any_watcher.cpp
)

set(AW_TPL_HEADERS
    include/aw/tpl/operation_canceled_exception.hpp
    include/aw/tpl/timeout_exception.hpp
    include/aw/tpl/tpl.hpp
    include/aw/tpl/threading/cancellation_token.hpp
    include/aw/tpl/threading/cancellation_token_source.hpp
    include/aw/tpl/threading/thread_pool.hpp
    include/aw/tpl/threading/detail/callback_function.hpp
    include/aw/tpl/threading/detail/cancellation_state.hpp
    include/aw/tpl/threading/detail/delay_awaiter.hpp
    include/aw/tpl/threading/detail/detached.hpp
    include/aw/tpl/threading/detail/pool_scheduling.hpp
    include/aw/tpl/threading/detail/timer_entry.hpp
    include/aw/tpl/threading/tasks/async_sleep.hpp
    include/aw/tpl/threading/tasks/spawn.hpp
    include/aw/tpl/threading/tasks/task.hpp
    include/aw/tpl/threading/tasks/task_already_moved_exception.hpp
    include/aw/tpl/threading/tasks/task_fwd.hpp
    include/aw/tpl/threading/tasks/task_group.hpp
    include/aw/tpl/threading/tasks/task_void.hpp
    include/aw/tpl/threading/tasks/when_all.hpp
    include/aw/tpl/threading/tasks/when_any.hpp
    include/aw/tpl/threading/tasks/detail/cancelable_sleep_awaiter.hpp
    include/aw/tpl/threading/tasks/detail/completion_awaiter.hpp
    include/aw/tpl/threading/tasks/detail/final_awaiter.hpp
    include/aw/tpl/threading/tasks/detail/initial_schedule_awaiter.hpp
    include/aw/tpl/threading/tasks/detail/is_truly_copyable.hpp
    include/aw/tpl/threading/tasks/detail/promise.hpp
    include/aw/tpl/threading/tasks/detail/promise_base.hpp
    include/aw/tpl/threading/tasks/detail/promise_void.hpp
    include/aw/tpl/threading/tasks/detail/run_async.hpp
    include/aw/tpl/threading/tasks/detail/spawn_guard.hpp
    include/aw/tpl/threading/tasks/detail/task_state.hpp
    include/aw/tpl/threading/tasks/detail/task_group_awaiter.hpp
    include/aw/tpl/threading/tasks/detail/task_state_base.hpp
    include/aw/tpl/threading/tasks/detail/when_all_element.hpp
    include/aw/tpl/threading/tasks/detail/when_all_settled.hpp
    include/aw/tpl/threading/tasks/detail/when_any_awaiter.hpp
    include/aw/tpl/threading/tasks/detail/when_any_state.hpp
    include/aw/tpl/threading/tasks/detail/when_any_watcher.hpp
)

add_library(aw_tpl STATIC ${AW_TPL_SOURCES})
add_library(aw::tpl ALIAS aw_tpl)

target_compile_features(aw_tpl PUBLIC cxx_std_23)

target_include_directories(aw_tpl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(aw_tpl PUBLIC Threads::Threads)

# Register public headers with FILE_SET (CMake 3.23+)
target_sources(aw_tpl
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES ${AW_TPL_HEADERS}
)

# Apply compiler-specific configuration
if(AW_TPL_WARNINGS_AS_ERRORS)
    aw_configure_compiler(TARGET aw_tpl STRICT_WARNINGS)
else()
    aw_configure_compiler(TARGET aw_tpl)
endif()

# --- Install & Package Export ---
if(AW_TPL_INSTALL)
    include(Library/InstallExport)
    aw_install_library(TARGET aw_tpl NAME tpl NS aw::tpl DEPENDENCIES Threads)
endif()

# --- Examples ---
if(AW_TPL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# --- Tests ---
if(AW_TPL_BUILD_TESTS)
    include(Tests)  # From remote
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        include(CTest)
        if(BUILD_TESTING)
            add_subdirectory(tests)
        endif()
    endif()
endif()
