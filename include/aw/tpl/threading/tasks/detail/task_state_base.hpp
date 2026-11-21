#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <functional>
#include <mutex>
#include <semaphore>
#include <vector>

// std::move_only_function is C++26; libc++ in clang-18 does not provide it.
#if __has_include(<version>) && defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
#define AW_TPL_HAS_MOVE_ONLY_FUNCTION 1
#endif

namespace aw::tpl::threading::tasks::detail {

/**
 * @brief Throws @c detail::task_stateless_exception: the task has no shared state (default-constructed or moved-from).
 */
[[noreturn]] void throw_task_has_no_state();

/**
 * @struct TaskStateBase
 * @brief Shared synchronization state between a running coroutine and its @c Task handle(s).
 */
struct TaskStateBase {
    std::mutex mtx;
    /**
     * @brief Completion flag. Atomic so awaiters can poll it without acquiring @c mtx
     *        (a false is always safe: they fall back to the locked suspend path).
     *        Writers publish it with release inside @c finish() so that an acquire load
     *        that observes true also observes @c exception and (in derived states) @c value.
     */
    std::atomic<bool> ready{false};
    std::vector<std::coroutine_handle<>> continuations;
    /// Fire-and-forget callbacks invoked once on completion; used by when_any optimization.
#ifdef AW_TPL_HAS_MOVE_ONLY_FUNCTION
    std::vector<std::move_only_function<void()>> completion_callbacks;
#else
    std::vector<std::function<void()>> completion_callbacks;
#endif
    std::exception_ptr exception = nullptr;
    std::binary_semaphore done_sem{0};

    /// Coroutine frame owned by this state once the coroutine reached its final suspend point.
    std::coroutine_handle<> frame = nullptr;

    TaskStateBase() = default;
    TaskStateBase(const TaskStateBase&) = delete;
    TaskStateBase& operator=(const TaskStateBase&) = delete;

    /**
     * @brief Destroys the finished coroutine frame (and with it any by-value parameters).
     *        Runs when the last Task/awaiter reference is dropped, i.e. never while the ramp function
     *        of the coroutine can still be running.
     */
    ~TaskStateBase();

    /**
     * @brief Checks whether the task has completed in a thread-safe manner.
     * @return True if result or exception is ready.
     */
    [[nodiscard]] bool is_ready();

    /**
     * @brief Registers an awaiting coroutine if the task is not yet complete.
     * @param[in] awaiting Handle of the caller coroutine.
     * @return True if suspended and @p awaiting will be resumed later; false if already ready.
     */
    bool try_attach_continuation(std::coroutine_handle<> awaiting);

    /**
     * @brief Registers a completion callback if the task is not yet complete.
     *        The callback is invoked exactly once when the task finishes, outside any locks.
     *        Used by when_any to attach direct notifications without spawning watcher coroutines.
     * @param[in] callback Invoked with no arguments; must not block or throw.
     * @return True if registered (callback will fire later); false if already ready (caller must handle).
     */
    bool try_attach_completion_callback(
#ifdef AW_TPL_HAS_MOVE_ONLY_FUNCTION
        std::move_only_function<void()> callback
#else
        std::function<void()> callback
#endif
    );

    /**
     * @brief @c await_suspend helper: attaches @p awaiting or hands it straight back when already ready.
     * @return @c std::noop_coroutine() if suspended; @p awaiting if it must continue immediately.
     */
    std::coroutine_handle<> suspend_or_resume(std::coroutine_handle<> awaiting);

    /**
     * @brief Finishes the task state, releasing waiters, and returning the first continuation handle.
     * @return The first continuation handle, or std::noop_coroutine() if no continuations were attached.
     */
    std::coroutine_handle<> finish();

    /**
     * @brief Marks the task as finished, releases blocking waiters, and resumes the first continuation.
     */
    void complete();

    /**
     * @brief Blocks the calling thread until the task is finished.
     * @throws std::logic_error When called on a pool worker for an unfinished task (would deadlock).
     */
    void wait_blocking();
};

} // namespace aw::tpl::threading::tasks::detail
