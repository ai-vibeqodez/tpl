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

.PHONY: test demo lint format clean release release-dry-run changelog changelog-dry-run update pr-fast weekly-deep

# Configure, build, and run tests only
build:
	cmake --preset $(PRESET)
	cmake --build --preset $(PRESET)

test: build
	ctest --preset $(PRESET)

# ────────────────────────────────────────────────────────────────────────────
# CI Contract targets (for GitHub Actions reusable workflows)
# ────────────────────────────────────────────────────────────────────────────

# pr-fast: quick lint + build + unit tests (called by reusable-pr-fast.yml)
pr-fast: lint test
	@echo "✓ PR checks passed"

# weekly-deep: sanitizers, stress tests, coverage (called by reusable-weekly-deep.yml)
weekly-deep: clean
	@# ASan + UBSan build
	cmake --preset debug -DAW_TPL_SANITIZE=address
	cmake --build --preset debug --parallel
	ASAN_OPTIONS=alloc_dealloc_mismatch=0:detect_leaks=1 ctest --preset debug --output-on-failure
	@# TSan build (separate from ASan)
	rm -rf build/debug
	cmake --preset debug -DAW_TPL_SANITIZE=thread
	cmake --build --preset debug --parallel
	ctest --preset debug --output-on-failure
	@# Coverage build
	rm -rf build/debug
	cmake --preset debug -DAW_TPL_ENABLE_COVERAGE=ON
	cmake --build --preset debug --parallel
	ctest --preset debug --output-on-failure
	@echo "✓ Deep checks passed"

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

# Cut a SemVer release from CHANGELOG.md [Unreleased]: bump, commit, tag. `make release PUSH=1` also pushes.
# Detect POSIX shell: MSYS2/MinGW/Cygwin/WSL all have /usr/bin.
# Only pure Windows (no /usr/bin) falls back to PowerShell.
IS_POSIX := $(strip $(if $(wildcard /usr/bin),1,))

release:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/release.ps1 $(if $(PUSH),-Push)
else
	bash scripts/release.sh $(if $(PUSH),--push)
endif

release-dry-run:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/release.ps1 -DryRun
else
	bash scripts/release.sh --dry-run
endif

lint: build
	@if command -v clang-tidy >/dev/null 2>&1; then \
	    find include src -name '*.hpp' -o -name '*.cpp' | xargs clang-tidy -p build/$(PRESET); \
	else echo "clang-tidy not found"; fi

format:
	@if command -v clang-format >/dev/null 2>&1; then \
	    find include src examples tests -name '*.hpp' -o -name '*.cpp' 2>/dev/null | xargs clang-format -i; \
	else echo "clang-format not found (>= 15 required)"; fi

# Cut changelog from git log (conventional commits). `make changelog FORCE=1` forces even if [Unreleased] is empty.
changelog:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/changelog-update.ps1 $(if $(FORCE),-Force)
else
	bash scripts/changelog-update.sh $(if $(FORCE),--force)
endif

changelog-dry-run:
	ifneq ($(IS_POSIX),1)
		powershell -NoProfile -ExecutionPolicy Bypass -File scripts/changelog-update.ps1 -DryRun
	else
		bash scripts/changelog-update.sh --dry-run
	endif

# Update all scripts from ai-warevo/.github.
update:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/updater.ps1
else
	bash scripts/updater.sh
endif
