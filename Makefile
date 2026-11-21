# ── aw::tpl ───────────────────────────────────────────────────────────────
# Everyday build targets.
#
#   make build | test | clean   (BUILD_TYPE=Debug|Release, default Debug)
#
# Examples:
#   make build BUILD_TYPE=Release
#   make test BUILD_TYPE=Release

BUILD_TYPE ?= Debug
PRESET     := $(if $(filter Release,$(BUILD_TYPE)),release,debug)

.PHONY: build test lint format clean

build:
	cmake -B build/$(PRESET) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build/$(PRESET)

test: build
	@if [ -f tests/CMakeLists.txt ]; then ctest --test-dir build/$(PRESET) --output-on-failure --parallel 4; \
	else echo "No tests/ directory."; fi

lint: build
	@if command -v clang-tidy >/dev/null 2>&1; then \
	    find include src -name '*.hpp' -o -name '*.cpp' | xargs clang-tidy -p build/$(PRESET) --warnings-as-errors='*'; \
	else echo "clang-tidy not found"; fi

format:
	@if command -v clang-format >/dev/null 2>&1; then \
	    find include src examples tests -name '*.hpp' -o -name '*.cpp' 2>/dev/null | xargs clang-format -i; \
	else echo "clang-format not found (>= 15 required)"; fi

clean:
	rm -rf build
