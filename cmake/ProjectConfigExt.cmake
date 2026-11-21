# ProjectConfigExt.cmake — aw::tpl extensions to remote ProjectConfig.cmake

include(CheckCXXSourceCompiles)

#[[
Check C++23 coroutine/format runtime support and fail early with a readable message.
Must be called after project() and find_package(Threads).
Sets AW_HAS_CXX20_RUNTIME to indicate the toolchain provides the required C++20/23 runtime library.
]]
function(aw_check_cxx23_runtime)
    set(CMAKE_REQUIRED_FLAGS "")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_REQUIRED_FLAGS "-std=c++23")
    endif()
    set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)

    check_cxx_source_compiles("
        #include <coroutine>
        #include <format>
        int main() {
            std::format(\"{}\", 42);
            return 0;
        }
    " AW_HAS_CXX20_RUNTIME)

    if(NOT AW_HAS_CXX20_RUNTIME)
        message(FATAL_ERROR
            "The selected C++ toolchain lacks the C++23 standard library features "
            "(coroutine, format) required by aw::tpl. "
            "Try a newer GCC (≥13), Clang (≥17), or MSVC (≥19.35)."
        )
    endif()
endfunction()
