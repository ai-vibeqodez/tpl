#ifndef AW_TPL_THREADING_TASKS_DETAIL_CANCELABLE_SLEEP_AWAITER_HPP
#define AW_TPL_THREADING_TASKS_DETAIL_CANCELABLE_SLEEP_AWAITER_HPP

#include <aw/tpl/threading/cancellation_token.hpp>

#include <chrono>
#include <coroutine>

namespace aw::tpl::threading::tasks::detail {

/**
 * @struct CancelableSleepAwaiter
 * @brief Awaitable timer object supporting cooperative cancellation via @c CancellationToken
 *        (returned by @c async_sleep(duration, token)).
 */
struct CancelableSleepAwaiter {
    std::chrono::milliseconds duration;
    CancellationToken token;

    [[nodiscard]] bool await_ready() const;
    void await_suspend(std::coroutine_handle<> h) const;
    void await_resume() const;
};

} // namespace aw::tpl::threading::tasks::detail

#endif  // AW_TPL_THREADING_TASKS_DETAIL_CANCELABLE_SLEEP_AWAITER_HPP
