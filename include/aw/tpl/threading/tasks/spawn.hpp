#pragma once

/**
 * @file spawn.hpp
 * @brief @c co_spawn: explicit fire-and-forget for tasks, optionally with error reporting and drain tracking.
 */

#include <aw/tpl/threading/tasks/task.hpp>
#include <aw/tpl/threading/tasks/task_group.hpp>
#include <aw/tpl/threading/tasks/detail/spawn_guard.hpp>

#include <exception>
#include <functional>
#include <utility>

namespace aw::tpl::threading::tasks {

/**
 * @brief Receives the exception of a spawned task that failed.
 *        Runs on a pool worker; an exception thrown by the handler itself is swallowed.
 */
using ErrorHandler = std::function<void(std::exception_ptr)>;

/**
 * @brief Detaches @p task: the caller does not observe it. Failures are silently discarded.
 *        Tasks are eager, so this only documents intent (and drops the @c [[nodiscard]] handle);
 *        the coroutine keeps running and its frame is released when it finishes.
 * @tparam T Result type of the task (ignored).
 * @param[in] task Task to detach.
 */
template <typename T>
void co_spawn(Task<T> task) {
    (void)task;
}

/**
 * @brief Detaches @p task and reports a failure to @p on_error instead of dropping it silently.
 * @tparam T Result type of the task (ignored).
 * @param[in] task Task to detach.
 * @param[in] on_error Called on a pool worker with the task's exception if it failed.
 */
template <typename T>
void co_spawn(Task<T> task, ErrorHandler on_error) {
    (void)detail::spawn_guard<T>(std::move(task), std::move(on_error), nullptr);
}

/**
 * @brief Detaches @p task and tracks it in @p group: the group's counter is incremented before this
 *        function returns and decremented after the task (and @p on_error, if any) finished.
 *        Use @c group.wait() / @c group.wait_async() to drain.
 * @tparam T Result type of the task (ignored).
 * @param[in,out] group Group tracking the task; must outlive it.
 * @param[in] task Task to detach.
 * @param[in] on_error Optional failure handler, called before the group is released.
 */
template <typename T>
void co_spawn(TaskGroup& group, Task<T> task, ErrorHandler on_error = {}) {
    group.add();
    (void)detail::spawn_guard<T>(std::move(task), std::move(on_error), &group);
}

} // namespace aw::tpl::threading::tasks
