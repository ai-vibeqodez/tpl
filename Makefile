# ── aw::tpl ──────────────────────────────────────────────────────────────
# Everyday build targets.
#
#   make build | test | demo | clean   (BUILD_TYPE=Debug|Release, default Debug)
#
# Examples:
#   make build BUILD_TYPE=Release
#   make test BUILD_TYPE=Release
#   make demo          # build and run all examples

BUILD_TYPE ?= Debug
PRESET     := $(if $(filter Release,$(BUILD_TYPE)),release,debug)

.PHONY: test demo lint format clean

# Configure, build, and run tests only
build:
	cmake --preset $(PRESET)
	cmake --build --preset $(PRESET)

test: build
	ctest --preset $(PRESET)

# Configure, build, and run every example in examples/
demo:
	cmake --preset $(PRESET) -DAW_TPL_BUILD_TESTS=OFF -DAW_TPL_BUILD_EXAMPLES=ON
	cmake --build --preset $(PRESET) --target aw_tpl_examples --parallel
	@if command -v clang-format >/dev/null 2>&1; then \
	    for example in $(PRESET)/examples/aw_tpl_example_*; do echo "=== $$example"; "$$example" || exit 1; done; \
	else \
	    for example in $(PRESET)/examples/aw_tpl_example_*; do echo "=== $$example"; "$$example" || exit 1; done; \
	fi

clean:
	rm -rf build

lint: build
	@if command -v clang-tidy >/dev/null 2>&1; then \
	    find include src -name '*.hpp' -o -name '*.cpp' | xargs clang-tidy -p build/$(PRESET); \
	else echo "clang-tidy not found"; fi

format:
	@if command -v clang-format >/dev/null 2>&1; then \
	    find include src examples tests -name '*.hpp' -o -name '*.cpp' 2>/dev/null | xargs clang-format -i; \
	else echo "clang-format not found (>= 15 required)"; fi


