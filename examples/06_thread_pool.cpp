// ThreadPool: the global pool, default thread count, and a private pool of your own.
#include <aw/tpl/tpl.hpp>

#include <print>
#include <thread>

using namespace std::chrono_literals;
using namespace aw::tpl;
using namespace aw::tpl::threading;
using namespace aw::tpl::threading::tasks;

Task<int> work(int v) {
    co_await async_sleep(20ms);
    co_return v + 1;
}

int main() {
    // Must be called before the global pool is first used; afterwards it is ignored.
    ThreadPool::set_default_threads(4);
    std::print("global pool threads: {}\n", ThreadPool::global().thread_count());

    // A coroutine started from a non-pool thread runs on the global pool.
    std::print("work(1) = {}\n", work(1).result());

    // Own pool: coroutines started *from a worker of this pool* (and their continuations,
    // timers and cancellations) stay on it. Task<>::run is the easy way to start such a coroutine.
    {
        ThreadPool pool(2);
        std::print("private pool threads: {}\n", pool.thread_count());
    } // destruction resumes pending timers, drains queued work and joins every thread

    // Blocking on a pool worker is a bug and throws std::logic_error instead of deadlocking.
    auto bad = []() -> Task<void> {
        try {
            (void)work(1).result();
        } catch (const std::logic_error& ex) {
            std::print("blocking on a worker: {}\n", ex.what());
        }
        co_return;
    };
    bad().wait();
    return 0;
}
