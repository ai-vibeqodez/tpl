#pragma once

#include <aw/tpl/threading/tasks/task.hpp>
#include <aw/tpl/threading/tasks/task_group.hpp>

#include <exception>
#include <functional>

namespace aw::tpl::threading::tasks::detail {

/**
 * @brief Coroutine behind @c co_spawn: awaits @p task, forwards a failure to @p handler and finally
 *        releases @p group. Parameters live in the coroutine frame, so everything stays alive until
 *        the awaited task finished. Exceptions thrown by @p handler are swallowed.
 */
template <typename T>
Task<void> spawn_guard(Task<T> task, std::function<void(std::exception_ptr)> handler, TaskGroup* group) {
    std::exception_ptr error;
    try {
        (void)co_await task; // for Task<void> the cast is a no-op
    } catch (...) {
        error = std::current_exception();
    }
    task = Task<T>{};
    if (error && handler) {
        try {
            handler(error);
        } catch (...) {
            // Nobody observes this guard task; a throwing handler must not escape into the pool.
        }
    }
    handler = nullptr; // release the handler's captures before the group reports completion
    if (group) {
        group->done();
    }
}

} // namespace aw::tpl::threading::tasks::detail
