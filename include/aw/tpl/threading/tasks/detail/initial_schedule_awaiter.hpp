#ifndef AW_TPL_THREADING_TASKS_DETAIL_INITIAL_SCHEDULE_AWAITER_HPP
#define AW_TPL_THREADING_TASKS_DETAIL_INITIAL_SCHEDULE_AWAITER_HPP

#include <coroutine>

namespace aw::tpl::threading::tasks::detail {

/**
 * @struct InitialScheduleAwaiter
 * @brief Immediately schedules a newly created coroutine onto the active or global @c ThreadPool.
 */
struct InitialScheduleAwaiter {
    [[nodiscard]] bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const;
    void await_resume() const noexcept {}
};

} // namespace aw::tpl::threading::tasks::detail

#endif  // AW_TPL_THREADING_TASKS_DETAIL_INITIAL_SCHEDULE_AWAITER_HPP
