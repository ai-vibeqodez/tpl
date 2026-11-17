#pragma once

#include <coroutine>

namespace aw::tpl::threading::tasks {

class TaskGroup;

} // namespace aw::tpl::threading::tasks

namespace aw::tpl::threading::tasks::detail {

/**
 * @struct TaskGroupAwaiter
 * @brief Awaiter returned by @c TaskGroup::wait_async(): suspends until the group's counter reaches zero,
 *        without blocking a pool worker. The continuation is resumed on the thread pool.
 */
struct TaskGroupAwaiter {
    TaskGroup& group;

    [[nodiscard]] bool await_ready() const;
    [[nodiscard]] bool await_suspend(std::coroutine_handle<> awaiting) const;
    void await_resume() const noexcept {}
};

} // namespace aw::tpl::threading::tasks::detail
