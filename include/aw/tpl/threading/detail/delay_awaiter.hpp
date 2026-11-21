#ifndef AW_TPL_THREADING_DETAIL_DELAY_AWAITER_HPP
#define AW_TPL_THREADING_DETAIL_DELAY_AWAITER_HPP

#include <chrono>
#include <coroutine>

namespace aw::tpl::threading::detail {

/**
 * @struct DelayAwaiter
 * @brief Awaitable timer bound to the active or global @c ThreadPool (returned by @c async_sleep).
 */
struct DelayAwaiter {
    std::chrono::milliseconds duration;

    [[nodiscard]] bool await_ready() const noexcept { return duration.count() <= 0; }
    void await_suspend(std::coroutine_handle<> h) const;
    void await_resume() const noexcept {}
};

} // namespace aw::tpl::threading::detail

#endif  // AW_TPL_THREADING_DETAIL_DELAY_AWAITER_HPP
