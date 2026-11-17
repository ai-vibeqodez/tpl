#pragma once

/**
 * @file task_group.hpp
 * @brief @c TaskGroup: a counter of in-flight operations that can be awaited (drained).
 */

#include <aw/tpl/threading/tasks/detail/task_group_awaiter.hpp>

#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <mutex>
#include <vector>

namespace aw::tpl::threading::tasks {

/**
 * @class TaskGroup
 * @brief Counts in-flight operations so that an owner can wait until all of them have finished
 *        (similar in spirit to C# @c CountdownEvent / Go's @c sync.WaitGroup).
 *
 * Call @c add() before starting an operation and @c done() when it ends; @c wait() / @c wait_async()
 * complete once the counter is zero. All members are thread-safe. The group must outlive every
 * operation that still has to call @c done(), but may be destroyed right after a wait returned.
 */
class TaskGroup {
public:
    TaskGroup() = default;
    TaskGroup(const TaskGroup&) = delete;
    TaskGroup& operator=(const TaskGroup&) = delete;

    /**
     * @brief Registers @p count more in-flight operations.
     * @param[in] count Number of operations to add.
     */
    void add(std::size_t count = 1);

    /**
     * @brief Marks one operation as finished; releases all waiters when the counter reaches zero.
     * @throws std::logic_error If there is no operation in flight (unbalanced @c done()).
     */
    void done();

    /**
     * @brief Number of operations currently in flight.
     */
    [[nodiscard]] std::size_t pending() const;

    /**
     * @brief Blocks the calling thread until no operation is in flight.
     * @throws std::logic_error When called on a pool worker while operations are in flight (would deadlock);
     *         use @c co_await @c wait_async() inside coroutines.
     */
    void wait();

    /**
     * @brief Awaitable that completes once no operation is in flight; does not block a worker.
     */
    [[nodiscard]] detail::TaskGroupAwaiter wait_async() noexcept { return detail::TaskGroupAwaiter{*this}; }

private:
    friend struct detail::TaskGroupAwaiter;

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::size_t count_ = 0;
    std::vector<std::coroutine_handle<>> waiters_;
};

} // namespace aw::tpl::threading::tasks
