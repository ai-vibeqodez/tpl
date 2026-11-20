#pragma once

#include <aw/tpl/threading/tasks/detail/task_state_base.hpp>

#include <optional>

namespace aw::tpl::threading::tasks::detail {

/**
 * @struct TaskState
 * @brief Shared state storing a return value of type @p T.
 * @tparam T Result type produced by the coroutine.
 *
 * Every @c TaskState<T> derives from @c TaskState<void> (the completion-only state), mirroring how
 * C# @c Task<T> derives from the non-generic @c Task. This lets a @c Task<T> be viewed as a
 * @c Task<void> that shares the very same state (see @c Task<void>'s converting constructor).
 */
template <typename T>
struct TaskState;

/**
 * @struct TaskState<void>
 * @brief Shared state specialization for void-returning tasks (also the common base of all states).
 */
template <>
struct TaskState<void> : TaskStateBase {};

template <typename T>
struct TaskState : TaskState<void> {
    std::optional<T> value;
    bool value_taken = false;  // Protected by TaskStateBase::mtx (acquired in try_take).

    /**
     * @brief One-shot claim used when a move-only result is extracted.
     * @return True for the first caller only (holds mtx to ensure atomicity).
     */
    bool try_take() noexcept {
        std::lock_guard<std::mutex> lock{mtx};
        if (value_taken) {
            return false;
        }
        value_taken = true;
        return true;
    }
};

} // namespace aw::tpl::threading::tasks::detail
