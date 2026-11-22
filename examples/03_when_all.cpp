// when_all: wait for several tasks (variadic with mixed types, or a vector of tasks).
#include <aw/tpl/tpl.hpp>

#include <format>
#include <memory>
#include <print>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <variant>

using namespace std::chrono_literals;
using namespace aw::tpl;
using namespace aw::tpl::threading;
using namespace aw::tpl::threading::tasks;

Task<int> number(int v, std::chrono::milliseconds delay) {
    co_await async_sleep(delay);
    co_return v;
}

Task<std::string> text(std::string s, std::chrono::milliseconds delay) {
    co_await async_sleep(delay);
    co_return s;
}

Task<void> nothing(std::chrono::milliseconds delay) {
    co_await async_sleep(delay);
}

Task<int> failing() {
    co_await async_sleep(10ms);
    throw std::runtime_error("child failed");
    co_return 0;
}

// Helper: normalises bool to lowercase "true"/"false".
inline std::string bool_str(bool v) { return std::format("{}", v ? "true" : "false"); }

Task<void> app() {
    // 1. Heterogeneous: a tuple of results; Task<void> children yield std::monostate.
    auto [n, s, unit] = co_await when_all(number(1, 120ms), text("two", 60ms), nothing(30ms));
    std::print("tuple: {}, {}, monostate? {}\n", n, s, bool_str(std::is_same_v<decltype(unit), std::monostate>));

    // 2. Only Task<void> children: the result is a Task<void>.
    co_await when_all(nothing(20ms), nothing(40ms));
    std::print("all void tasks done\n");

    // 3. Vector of Task<T>: a vector of results in the original order.
    std::vector<Task<int>> many{
        number(10, 80ms),
        number(20, 60ms),
        number(30, 40ms),
        number(40, 20ms),
    };
    std::vector<int> results = co_await when_all(std::move(many));
    std::print("vector:");
    std::ranges::for_each(results, [](int r) { std::print(" {}", r); });
    std::print("\n");

    // 4. Vector of Task<void>.
    std::vector<Task<void>> voids{nothing(10ms), nothing(20ms)};
    co_await when_all(std::move(voids));

    // 5. Failures: every child is awaited first, then the first failure (in argument order) is rethrown.
    try {
        (void)co_await when_all(number(1, 100ms), failing());
    } catch (const std::runtime_error& ex) {
        std::print("when_all rethrew: {}\n", ex.what());
    }
}

int main() {
    app().wait();
    return 0;
}
