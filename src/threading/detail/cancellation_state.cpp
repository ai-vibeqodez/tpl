#include <aw/tpl/threading/detail/cancellation_state.hpp>

#include <algorithm>
#include <exception>
#include <ranges>

namespace aw::tpl::threading::detail {

void CancellationState::cancel() {
    std::vector<std::pair<std::uint64_t, CallbackFunction<void()>>> to_run;
    {
        std::lock_guard lock(mtx);
        if (canceled.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        to_run.swap(callbacks);
    }
    // The lock is not held while callbacks run. A throwing callback must not prevent the others
    // from running (they may resume sleeping coroutines), so every failure is collected; the first
    // one is rethrown afterwards, matching the documented first-exception-wins contract.
    std::vector<std::exception_ptr> errors;
    std::ranges::for_each(to_run, [&errors](auto& entry) {
        if (!entry.second) return;
        try {
            entry.second();
        } catch (...) {
            errors.push_back(std::current_exception());
        }
    });
    if (!errors.empty()) {
        std::rethrow_exception(errors.front());
    }
}

std::uint64_t CancellationState::register_callback(CallbackFunction<void()> cb) {
    std::uint64_t id = 0;
    CallbackFunction<void()> immediate_cb;
    {
        std::lock_guard lock(mtx);
        if (canceled.load(std::memory_order_acquire)) {
            immediate_cb = std::move(cb);
        } else {
            id = next_id++;
            callbacks.emplace_back(id, std::move(cb));
        }
    }
    // An exception from an immediately executed callback propagates to the caller.
    if (immediate_cb) {
        immediate_cb();
    }
    return id;
}

bool CancellationState::unregister_callback(std::uint64_t id) {
    if (id == 0) return false;
    std::lock_guard lock(mtx);
    if (auto it = std::ranges::find_if(callbacks, [id](const auto& e) { return e.first == id; });
        it != callbacks.end()) {
        callbacks.erase(it);
        return true;
    }
    return false;
}

std::size_t CancellationState::callback_count() {
    std::lock_guard lock(mtx);
    return callbacks.size();
}

} // namespace aw::tpl::threading::detail
