#include <aw/tpl/threading/tasks/detail/task_state_base.hpp>
#include <aw/tpl/threading/tasks/detail/task_stateless_exception.hpp>

#include <aw/tpl/threading/detail/pool_scheduling.hpp>

#include <algorithm>
#include <ranges>

namespace aw::tpl::threading::tasks::detail {

void throw_task_has_no_state() {
    throw task_stateless_exception{};
}

TaskStateBase::~TaskStateBase() {
    if (frame) {
        frame.destroy();
    }
}

bool TaskStateBase::is_ready() const {
    return ready.load(std::memory_order_acquire);
}

bool TaskStateBase::try_attach_continuation(std::coroutine_handle<> awaiting) {
    std::lock_guard lock(mtx);
    if (ready.load(std::memory_order_relaxed)) {
        return false;
    }
    continuations.push_back(awaiting);
    return true;
}

bool TaskStateBase::try_attach_completion_callback(aw::tpl::threading::detail::CallbackFunction<void()> callback) {
    std::lock_guard lock(mtx);
    if (ready.load(std::memory_order_relaxed)) {
        return false;
    }
    completion_callbacks.push_back(std::move(callback));
    return true;
}

std::coroutine_handle<> TaskStateBase::suspend_or_resume(std::coroutine_handle<> awaiting) {
    if (!try_attach_continuation(awaiting)) {
        return awaiting;
    }
    return std::noop_coroutine();
}

std::coroutine_handle<> TaskStateBase::finish() {
    std::vector<std::coroutine_handle<>> to_resume;
    std::vector<aw::tpl::threading::detail::CallbackFunction<void()>> callbacks_to_run;
    {
        std::lock_guard lock(mtx);
        ready.store(true, std::memory_order_release);
        to_resume = std::move(continuations);
        continuations.clear();
        callbacks_to_run = std::move(completion_callbacks);
        completion_callbacks.clear();
    }
    done_sem.release();

    // Invoke all completion callbacks outside the lock; they must not throw.
    for (auto& cb : callbacks_to_run) {
        cb();
    }

    if (to_resume.empty()) {
        return std::noop_coroutine();
    }

    // The first continuation is resumed by symmetric transfer, the rest are scheduled on the pool.
    std::coroutine_handle<> first = to_resume.front();
    std::ranges::for_each(to_resume.begin() + 1, to_resume.end(),
                          [](auto h) { threading::detail::schedule_on_pool(h); });
    return first;
}

void TaskStateBase::complete() {
    std::coroutine_handle<> first = finish();
    first.resume();
}

void TaskStateBase::wait_blocking() {
    if (!is_ready()) {
        // A pool worker blocking here can starve the pool (guaranteed deadlock on a single worker).
        if (threading::detail::current_pool() != nullptr) {
            throw std::logic_error("Blocking wait on a thread-pool worker would deadlock; use co_await instead.");
        }
        done_sem.acquire();
        done_sem.release();
    }
}

} // namespace aw::tpl::threading::tasks::detail
