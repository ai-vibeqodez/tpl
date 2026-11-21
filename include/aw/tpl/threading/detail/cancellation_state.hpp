#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

// std::move_only_function is C++26; libc++ in clang-18 does not provide it.
#if __has_include(<version>) && defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
#define AW_TPL_HAS_MOVE_ONLY_FUNCTION 1
#endif

namespace aw::tpl::threading::detail {

/**
 * @struct CancellationState
 * @brief Shared thread-safe state backing a @c CancellationTokenSource and its tokens.
 */
struct CancellationState {
    std::atomic<bool> canceled{false};
    std::mutex mtx;
#ifdef AW_TPL_HAS_MOVE_ONLY_FUNCTION
    std::vector<std::pair<std::uint64_t, std::move_only_function<void()>>> callbacks;
#else
    std::vector<std::pair<std::uint64_t, std::function<void()>>> callbacks;
#endif
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
    std::uint64_t register_callback(
#ifdef AW_TPL_HAS_MOVE_ONLY_FUNCTION
        std::move_only_function<void()> cb
#else
        std::function<void()> cb
#endif
    );

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
