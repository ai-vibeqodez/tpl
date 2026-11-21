#ifndef AW_TPL_THREADING_DETAIL_CANCELLATION_STATE_HPP
#define AW_TPL_THREADING_DETAIL_CANCELLATION_STATE_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

#include <aw/tpl/threading/detail/callback_function.hpp>

namespace aw::tpl::threading::detail {

/**
 * @struct CancellationState
 * @brief Shared thread-safe state backing a @c CancellationTokenSource and its tokens.
 */
struct CancellationState {
    std::atomic<bool> canceled{false};
    std::mutex mtx;
    std::vector<std::pair<std::uint64_t, CallbackFunction<void()>>> callbacks;
    std::uint64_t next_id = 1;

    /**
     * @brief Transitions the state to canceled and invokes all registered callbacks once.
     *        Every callback runs even if one throws; the first exception is rethrown afterwards.
     */
    void cancel();

    /**
     * @brief Registers a callback to run upon cancellation, or executes it immediately if already canceled.
     * @param[in] cb Callable invoked on cancellation.
     * @return Registration id (> 0), or 0 if the callback was executed immediately.
     */
    std::uint64_t register_callback(CallbackFunction<void()> cb);

    /**
     * @brief Removes a previously registered callback.
     * @return True if the callback was still registered.
     */
    bool unregister_callback(std::uint64_t id);

    /**
     * @brief Number of callbacks currently registered (diagnostic).
     */
    [[nodiscard]] std::size_t callback_count();
};

} // namespace aw::tpl::threading::detail

#endif  // AW_TPL_THREADING_DETAIL_CANCELLATION_STATE_HPP
