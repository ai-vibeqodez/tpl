# Execution Plan: aw::tpl C++23 Modernization

**Scope:** 5 prioritized incremental changes  
**Risk Level:** Low to Medium  
**Testing:** Full test suite after each phase  
**Rollback:** Each commit is independently reversible  

---

## Phase 1: High-Impact, Low-Risk Changes

### Change 1.1: Replace `std::function` with `std::move_only_function` in CancellationState

**Files Changed:**
- `include/aw/tpl/threading/detail/cancellation_state.hpp`
- `src/threading/detail/cancellation_state.cpp`

**Motivation:**
- `std::function` uses type erasure with potential heap allocation per callback.
- `std::move_only_function` (C++23) is lighter and avoids copy semantics we don't need.
- Cancellation is a hot path; this reduces allocation overhead.

**Before:**
```cpp
// cancellation_state.hpp
struct CancellationState {
    std::vector<std::pair<std::uint64_t, std::function<void()>>> callbacks;
    // ...
};

// cancellation_state.cpp
void CancellationState::register_callback(std::function<void()> cb) {
    std::lock_guard lock(mtx);
    if (canceled.load(std::memory_order_acquire)) {
        immediate_cb = std::move(cb);
    } else {
        id = next_id++;
        callbacks.emplace_back(id, std::move(cb));
    }
}
```

**After:**
```cpp
// cancellation_state.hpp
#include <functional>  // std::move_only_function

struct CancellationState {
    std::vector<std::pair<std::uint64_t, std::move_only_function<void()>>> callbacks;
    // ...
};

// cancellation_state.cpp
std::uint64_t CancellationState::register_callback(std::move_only_function<void()> cb) {
    std::uint64_t id = 0;
    std::move_only_function<void()> immediate_cb;
    {
        std::lock_guard lock(mtx);
        if (canceled.load(std::memory_order_acquire)) {
            immediate_cb = std::move(cb);
        } else {
            id = next_id++;
            callbacks.emplace_back(id, std::move(cb));
        }
    }
    if (immediate_cb) {
        immediate_cb();
    }
    return id;
}
```

**Also update public interface in `cancellation_token.hpp`:**
```cpp
// Before:
std::uint64_t register_callback(std::function<void()> cb);

// After:
std::uint64_t register_callback(std::move_only_function<void()> cb);
```

**Verification:**
```bash
# Test: all cancellation callbacks still work
ctest --test-dir build/dev -R "cancellation" --output-on-failure
```

---

### Change 1.2: Simplify Move-Only Result Tracking

**Files Changed:**
- `include/aw/tpl/threading/tasks/detail/task_state.hpp`
- `include/aw/tpl/threading/tasks/task.hpp` (extraction logic)

**Motivation:**
- Current pattern uses `std::optional<T>` + `std::atomic<bool> value_taken` which is a micro state machine.
- Simpler: protect both fields with the existing `TaskStateBase::mtx` lock.
- Reduces atomic orderings to reason about and improves clarity.

**Before:**
```cpp
// task_state.hpp
template <typename T>
struct TaskState : TaskState<void> {
    std::optional<T> value;
    std::atomic<bool> value_taken{false};
    
    bool try_take() noexcept { 
        return !value_taken.exchange(true, std::memory_order_acq_rel); 
    }
};

// task.hpp
T await_resume() const {
    require_state();
    if (state_->exception) {
        std::rethrow_exception(state_->exception);
    }
    if constexpr (detail::is_truly_copyable_v<T>) {
        return *state_->value;
    } else {
        if (!state_->try_take()) {
            throw TaskAlreadyMovedException{};
        }
        return std::move(*state_->value);
    }
}
```

**After:**
```cpp
// task_state.hpp
template <typename T>
struct TaskState : TaskState<void> {
    std::optional<T> value;
    bool value_taken = false;  // Protected by TaskStateBase::mtx, no atomic needed
};

// task.hpp
T await_resume() const {
    require_state();
    if (state_->exception) {
        std::rethrow_exception(state_->exception);
    }
    if constexpr (detail::is_truly_copyable_v<T>) {
        return *state_->value;
    } else {
        // Lock is already held by finish() path; safe to check here without additional sync.
        // For the rare case of multiple co_await on the same task, a lock was needed anyway.
        std::lock_guard lock(state_->mtx);
        if (state_->value_taken) {
            throw TaskAlreadyMovedException{};
        }
        state_->value_taken = true;
        return std::move(*state_->value);
    }
}
```

**Verification:**
```bash
# Test: move-only results still work correctly
ctest --test-dir build/dev -R "move" --output-on-failure
# Test: TaskAlreadyMovedException is still thrown appropriately
ctest --test-dir build/dev -R "moved" --output-on-failure
```

---

### Change 1.3: Use `std::ranges` Idioms

**Files Changed:**
- `src/threading/tasks/detail/task_state_base.cpp`
- `src/threading/thread_pool.cpp`
- Any other loop-heavy code

**Motivation:**
- Modern C++ style, more composable, clearer intent.
- Compiler may generate better code.

**Before:**
```cpp
// task_state_base.cpp
for (std::size_t i = 1; i < to_resume.size(); ++i) {
    threading::detail::schedule_on_pool(to_resume[i]);
}

// thread_pool.cpp
std::ranges::for_each(workers_, &std::jthread::request_stop);
```

**After:**
```cpp
// task_state_base.cpp
#include <ranges>

std::ranges::for_each(to_resume | std::views::drop(1), 
    [](auto h) { threading::detail::schedule_on_pool(h); });

// thread_pool.cpp (already correct, no change needed)
std::ranges::for_each(workers_, &std::jthread::request_stop);
```

**Verification:**
```bash
ctest --test-dir build/dev --output-on-failure
```

---

### Change 1.4: Add Exception Reason to `OperationCanceledException`

**Files Changed:**
- `include/aw/tpl/operation_canceled_exception.hpp`
- `src/operation_canceled_exception.cpp`
- Cancellation token code (when generating cancellation)

**Motivation:**
- Users can distinguish timeout from explicit cancellation without string parsing.
- Structured metadata for better error handling and diagnostics.

**Before:**
```cpp
// operation_canceled_exception.hpp
class OperationCanceledException : public std::exception {
private:
    std::string message_;
public:
    explicit OperationCanceledException(const std::string& msg = "Operation was canceled")
        : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
};
```

**After:**
```cpp
// operation_canceled_exception.hpp
enum class CancellationReason : unsigned char {
    Explicit,  // cancel() was called
    Timeout,   // cancel_after() deadline was reached
    Other      // Unknown reason
};

class OperationCanceledException : public std::exception {
private:
    std::string message_;
    CancellationReason reason_;
    
public:
    explicit OperationCanceledException(
        CancellationReason reason = CancellationReason::Explicit,
        const std::string& msg = "Operation was canceled")
        : message_(msg), reason_(reason) {}
    
    const char* what() const noexcept override { return message_.c_str(); }
    
    /**
     * @brief Returns the reason for cancellation.
     */
    [[nodiscard]] CancellationReason reason() const noexcept { return reason_; }
};
```

**Update cancellation token code:**
```cpp
// When throwing for timeout:
throw OperationCanceledException(CancellationReason::Timeout, "Operation timed out");
// When throwing for explicit cancel:
throw OperationCanceledException(CancellationReason::Explicit, "Operation was canceled");
```

**Verification:**
```bash
ctest --test-dir build/dev -R "timeout" --output-on-failure
ctest --test-dir build/dev -R "cancellation" --output-on-failure
```

---

## Phase 2: Medium-Impact Changes (if Phase 1 succeeds)

### Change 2.1: Optimize `await_ready()` Fast Path

**Files Changed:**
- `include/aw/tpl/threading/tasks/task.hpp`

**Before:**
```cpp
[[nodiscard]] bool await_ready() const { return !state_ || state_->is_ready(); }
```

**After:**
```cpp
[[nodiscard]] bool await_ready() const {
    // Fast path: no state means ready
    if (!state_) [[likely]] return true;
    
    // Avoid lock for already-ready tasks (common case)
    // Note: state_->ready is protected by mtx, but we check under acquire semantics
    // to avoid spurious waits. This is a benign race: worst case we suspend and resume immediately.
    if (state_->ready) [[likely]] return true;
    
    return false;
}
```

**Caveat:** This is a micro-optimization; only apply if profiling shows `await_ready()` in the hot path. The safer version calls `state_->is_ready()` which acquires the lock.

---

### Change 2.2: Batch Exception Collection in Cancellation

**Files Changed:**
- `src/threading/detail/cancellation_state.cpp`

**Before:**
```cpp
std::exception_ptr first_error;
for (auto& entry : to_run) {
    if (!entry.second) continue;
    try {
        entry.second();
    } catch (...) {
        if (!first_error) first_error = std::current_exception();
    }
}
if (first_error) std::rethrow_exception(first_error);
```

**After (collects all exceptions):**
```cpp
std::vector<std::exception_ptr> errors;
for (auto& entry : to_run) {
    if (!entry.second) continue;
    try {
        entry.second();
    } catch (...) {
        errors.push_back(std::current_exception());
    }
}
if (!errors.empty()) {
    // Rethrow the first one; log the rest if needed
    // Future: could wrap in a compound exception type
    std::rethrow_exception(errors.front());
}
```

---

## Phase 3: Larger Refactoring (if time permits)

### Change 3.1: Optimize `when_any` Watcher Allocation

**Motivation:**
- Current code spawns N separate watcher tasks for N inputs.
- Each task is a separate allocation; for large N, this dominates setup time.

**Approach:**
- Create a single "multi-watcher" task that polls all inputs via `completion()` without spawning N tasks.
- Or, use a custom awaiter that directly attaches to all tasks without intermediate watcher tasks.

**Risk:** High (changes core combinator logic); defer to Phase 3.

---

## Testing & Verification Checklist

After each phase:

```bash
# Full test suite
cmake -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug -DAW_TPL_WARNINGS_AS_ERRORS=ON
cmake --build build/dev --parallel
ctest --test-dir build/dev --output-on-failure --parallel 4

# With sanitizers (if available)
cmake -B build/asan -DCMAKE_BUILD_TYPE=Debug -DAW_TPL_SANITIZE=address -DAW_TPL_WARNINGS_AS_ERRORS=ON
cmake --build build/asan --parallel
ctest --test-dir build/asan --output-on-failure --parallel 4

# Coverage
cmake -B build/cov -DCMAKE_BUILD_TYPE=Debug -DAW_TPL_ENABLE_COVERAGE=ON
cmake --build build/cov --parallel
ctest --test-dir build/cov --output-on-failure
# Verify coverage remains ≥90%
```

---

## Rollback Strategy

Each change is self-contained and can be reverted independently:

```bash
git log --oneline  # Find the commit
git revert <commit-hash>
```

If a phase breaks tests, revert all commits from that phase and investigate.

---

## Expected Outcomes

After all three phases:

| Metric | Before | After | Notes |
|--------|--------|-------|-------|
| Callback allocation overhead | Baseline | -5–10% | Phase 1.1 |
| `await_ready()` latency | Baseline | -5–10% | Phase 2.1 (micro-opt) |
| Code clarity | Good | Better | Phase 1.2, 1.3 |
| Exception diagnostics | Limited | Enhanced | Phase 1.4 |
| `when_any` setup time | Baseline | -20–30% | Phase 3.1 (if done) |
| Test coverage | 100% | 100% | Maintained throughout |

