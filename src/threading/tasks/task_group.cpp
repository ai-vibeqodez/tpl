#include <aw/tpl/threading/tasks/task_group.hpp>

#include <aw/tpl/threading/detail/pool_scheduling.hpp>

#include <stdexcept>
#include <utility>

namespace aw::tpl::threading::tasks {

void TaskGroup::add(std::size_t count) {
    std::lock_guard lock(mtx_);
    count_ += count;
}

void TaskGroup::done() {
    std::vector<std::coroutine_handle<>> to_resume;
    {
        std::lock_guard lock(mtx_);
        if (count_ == 0) {
            throw std::logic_error("TaskGroup::done() called without a matching add().");
        }
        if (--count_ == 0) {
            to_resume = std::move(waiters_);
            waiters_.clear();
            // Notified under the lock: a blocked wait() can only return (and destroy the group)
            // after this function released the mutex.
            cv_.notify_all();
        }
    }
    // `this` is not touched below: the group may already be gone.
    for (auto h : to_resume) {
        threading::detail::schedule_on_pool(h);
    }
}

std::size_t TaskGroup::pending() const {
    std::lock_guard lock(mtx_);
    return count_;
}

void TaskGroup::wait() {
    std::unique_lock lock(mtx_);
    if (count_ == 0) {
        return;
    }
    if (threading::detail::current_pool() != nullptr) {
        throw std::logic_error("Blocking TaskGroup::wait() on a thread-pool worker would deadlock; use wait_async().");
    }
    cv_.wait(lock, [this] { return count_ == 0; });
}

namespace detail {

bool TaskGroupAwaiter::await_ready() const {
    return group.pending() == 0;
}

bool TaskGroupAwaiter::await_suspend(std::coroutine_handle<> awaiting) const {
    std::lock_guard lock(group.mtx_);
    if (group.count_ == 0) {
        return false;
    }
    group.waiters_.push_back(awaiting);
    return true;
}

} // namespace detail

} // namespace aw::tpl::threading::tasks
