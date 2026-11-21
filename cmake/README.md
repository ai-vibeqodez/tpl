# CMake Modules for aw::tpl

This directory contains **project-specific CMake extensions** for `aw::tpl`. The project uses a **hybrid architecture**: base reusable modules are fetched from [ai-warevo/template_cpp](https://github.com/ai-warevo/template_cpp) via `FetchContent`, while project-specific customizations live here.

## Architecture

### Remote Modules (FetchContent from template_cpp)

- **`ProjectConfig.cmake`** — Project metadata, version management, namespace setup
- **`CoreLibrary.cmake`** — Standard library factory function
- **`Tests.cmake`** — Catch2 v3 test integration

These are declared in the root `CMakeLists.txt`:

```cmake
FetchContent_Declare(
    aw_cmake_modules
    GIT_REPOSITORY https://github.com/ai-warevo/template_cpp.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  non_existent_dir
)

FetchContent_MakeAvailable(aw_cmake_modules)
list(APPEND CMAKE_MODULE_PATH "${aw_cmake_modules_SOURCE_DIR}/cmake")
```

### Local Modules (This Directory)

- **`ProjectConfigExt.cmake`** — Extensions to `ProjectConfig.cmake`
  - `aw_check_cxx23_runtime()` — Runtime library validation for C++23 coroutines/format
- **`Compiler.cmake`** — Compiler-specific flags for aw::tpl
  - `aw_configure_compiler()` — Warnings, sanitizers, coverage, coroutine flags
- **`Library/InstallExport.cmake`** — Installation and package config generation
- **`Library/HeaderOnlyLib.cmake`** — Header-only library helper
- **`Library/AggregateLib.cmake`** — Library aggregation helper

## Usage Pattern

```cmake
# 1. Fetch remote modules (before project())
include(FetchContent)
FetchContent_Declare(aw_cmake_modules ...)
FetchContent_MakeAvailable(aw_cmake_modules)
list(APPEND CMAKE_MODULE_PATH "${aw_cmake_modules_SOURCE_DIR}/cmake")

# 2. Include remote base + local extensions
include(ProjectConfig)  # Remote
include(cmake/ProjectConfigExt.cmake)  # Local extension

aw_project_config(NAME tpl NS aw::)
project(${AW_PROJECT_NAME} VERSION ${AW_PROJECT_VERSION} LANGUAGES CXX)

# 3. Check C++23 runtime support (local extension)
find_package(Threads REQUIRED)
aw_check_cxx23_runtime()

# 4. Build library with remote helper
include(CoreLibrary)  # Remote
add_library(aw_tpl STATIC ${SOURCES})
# ...

# 5. Apply project-specific compiler config
include(cmake/Compiler.cmake)  # Local
aw_configure_compiler(TARGET aw_tpl STRICT_WARNINGS)

# 6. Tests (remote module)
include(Tests)  # Remote
add_subdirectory(tests)
```

## Module Reference

### `ProjectConfigExt.cmake` (Local)

Extends remote `ProjectConfig.cmake` with C++23-specific runtime checks.

#### `aw_check_cxx23_runtime()`

Check C++23 coroutine/format runtime support and fail early with a readable message. Must be called after `project()` and `find_package(Threads)`.

**Sets:** `AW_HAS_CXX20_RUNTIME` (indicates the toolchain provides required C++20/23 runtime)

**Throws:** `FATAL_ERROR` if coroutine or format support is missing

**Example:**
```cmake
project(MyProject LANGUAGES CXX)
find_package(Threads REQUIRED)
aw_check_cxx23_runtime()
```

### `Compiler.cmake` (Local)

Apply compiler-specific optimization flags for C++ coroutines, experimental library support, and sanitization.

#### `aw_configure_compiler(TARGET <target> [STRICT_WARNINGS])`

**Parameters:**
- `TARGET` (required) — Target library name
- `STRICT_WARNINGS` — Convert warnings to errors

**Applies:**
- STRICT_WARNINGS: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror` (GCC/Clang) or `/W4 /WX` (MSVC)
- `AW_SANITIZE=address`: `-fsanitize=address,undefined`
- `AW_SANITIZE=thread`: `-fsanitize=thread`
- `AW_ENABLE_COVERAGE=ON`: `--coverage -O0 -g`

**Example:**
```cmake
add_library(mylib STATIC src.cpp)
aw_configure_compiler(TARGET mylib STRICT_WARNINGS)
```

### `Library/InstallExport.cmake` (Local)

Generate CMake package config for find_package() and install rules.

#### `aw_install_library(TARGET <target> NAME <name> NS <namespace> [DEPENDENCIES ...])`

**Parameters:**
- `TARGET` — Library target
- `NAME` — Package name for `find_package()`
- `NS` — Install namespace (e.g., `aw::tpl`)
- `DEPENDENCIES` — List of dependencies (`Threads` → `find_dependency(Threads)`)

**Example:**
```cmake
aw_install_library(TARGET aw_tpl NAME tpl NS aw::tpl DEPENDENCIES Threads)
```

**Generates:**
- `lib/cmake/tpl/tplConfig.cmake`
- `lib/cmake/tpl/tplConfigVersion.cmake`
- `lib/cmake/tpl/tplTargets.cmake`

**Consumer usage:**
```cmake
find_package(tpl REQUIRED)
target_link_libraries(myapp PRIVATE aw::tpl)
```

### `Library/HeaderOnlyLib.cmake` (Local)

Create header-only INTERFACE library.

#### `aw_header_only_lib(TARGET <target> HEADERS <files...> [INCLUDE_DIR <dir>])`

**Example:**
```cmake
aw_header_only_lib(
    TARGET my_header_lib
    HEADERS include/my/lib.hpp
    INCLUDE_DIR include
)
```

### `Library/AggregateLib.cmake` (Local)

Combine multiple libraries into a single target.

#### `aw_aggregate_lib(TARGET <target> DEPENDS <libs...>)`

**Example:**
```cmake
aw_aggregate_lib(TARGET aw::all DEPENDS aw::tpl aw::net aw::io)
```

## Rationale: Hybrid vs. Full Remote

**Why hybrid?**

1. **Reusable across projects** — `ProjectConfig`, `CoreLibrary`, `Tests` are generic and shared
2. **Project-specific needs** — C++23 runtime checks, coroutine compiler flags, and aw::tpl sanitizer/coverage setup are unique to this project
3. **Stability** — Local modules can evolve independently while remote modules remain stable
4. **No duplication** — Only project-specific code lives locally

**Why not 100% remote?**

- C++23 runtime validation (`aw_check_cxx23_runtime`) is not needed by all projects
- Compiler flags for coroutines/sanitizers are aw::tpl-specific
- Installation logic depends on aw::tpl's public API surface

## Updating Remote Modules

Remote modules are fetched at **configure time** from the `main` branch. To lock to a specific version, change `GIT_TAG`:

```cmake
FetchContent_Declare(
    aw_cmake_modules
    GIT_REPOSITORY https://github.com/ai-warevo/template_cpp.git
    GIT_TAG        v1.2.3  # Pin to release tag or commit SHA
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  non_existent_dir
)
```

To update to the latest, delete the build directory and reconfigure:

```sh
rm -rf build && cmake -B build -G Ninja
```

## Testing Locally vs. Remote

To test changes to remote modules before pushing to `template_cpp`:

1. Modify the local copy in `/home/node/workspace/template-cpp/cmake/`
2. Change `GIT_REPOSITORY` to a local path temporarily:
   ```cmake
   FetchContent_Declare(
       aw_cmake_modules
       SOURCE_DIR /home/node/workspace/template-cpp/cmake
   )
   ```
3. Reconfigure and test
4. Push changes to `template_cpp` and revert to `GIT_REPOSITORY` URL

## See Also

- Root `CMakeLists.txt` — Main project configuration
- `tests/CMakeLists.txt` — Test suite setup
- [template_cpp/cmake](https://github.com/ai-warevo/template_cpp/tree/main/cmake) — Remote module source
- [AGENTS.md](../AGENTS.md) — Development guide for aw::tpl
