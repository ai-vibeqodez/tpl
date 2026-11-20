#include <aw/tpl/threading/detail/cancellation_state.hpp>

#include <algorithm>
#include <exception>
#include <ranges>

namespace aw::tpl::threading::detail {

void CancellationState::cancel() {
    std::vector<std::pair<std::uint64_t, std::move_only_function<void()>>> to_run;
    {
        std::lock_guard lock(mtx);
        if (canceled.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        to_run.swap(callbacks);
    }
    // The lock is not held while callbacks run. A throwing callback must not prevent the others
    // from running (they may resume sleeping coroutines), so remember the first error and rethrow it.
    std::exception_ptr first_error;
    std::ranges::for_each(to_run, [&first_error](auto& entry) {
        if (!entry.second) return;
        try {
            entry.second();
        } catch (...) {
            if (!first_error) first_error = std::current_exception();
        }
    });
    if (first_error) std::rethrow_exception(first_error);
}

std::uint64_t CancellationState::register_callback(std::move_only_function<void()> cb) {
    std::uint64_t id = 0;
    std::move_only_function<void()> immediate_cb;
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
