// Task<>::run: offload callables (lambdas, functors, move-only callables) to the thread pool.
#include <aw/tpl/tpl.hpp>

#include <format>
#include <memory>
#include <numeric>
#include <print>
#include <string>
#include <vector>

using namespace aw::tpl;
using namespace aw::tpl::threading;
using namespace aw::tpl::threading::tasks;

struct MoveOnlyJob {
    std::unique_ptr<int> value;
    int operator()() { return *value * 2; }
};

Task<int> parallel_sum(std::vector<int> numbers) {
    // run() stores the callable inside the coroutine frame, so capturing by value is always safe.
    int total = co_await Task<>::run(
        [numbers = std::move(numbers)] { return std::accumulate(numbers.begin(), numbers.end(), 0); });
    co_return total;
}

int main() {
    // 1. Void callable.
    Task<>::run([] { std::print("running on a pool thread\n"); }).wait();

    // 2. Callable with a result; the captured text outlives its original scope.
    Task<std::string> greeting;
    {
        std::string name = "tpl";
        greeting = Task<>::run([name] { return "hello, " + name; });
    }
    std::print("{}\n", greeting.result());

    // 3. Move-only callable.
    std::print("move-only job = {}\n", Task<>::run(MoveOnlyJob{std::make_unique<int>(21)}).result());

    // 4. Awaiting a run() from inside a coroutine.
    std::print("sum = {}\n", parallel_sum({1, 2, 3, 4, 5}).result());
    return 0;
}
