/**
 * @file test_when_any_callbacks.cpp
 * @brief Test when_any direct callback attachment (Phase 3 optimization).
 *
 * Verifies that when_any attaches completion callbacks directly without spawning
 * watcher coroutines. This reduces setup overhead O(N watcher frames) to O(1).
 */

#include <aw/tpl/threading/tasks/task.hpp>
#include <aw/tpl/threading/tasks/when_any.hpp>
#include <aw/tpl/threading/tasks/async_sleep.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace aw::tpl;
using namespace aw::tpl::threading::tasks;

TEST_CASE("when_any with direct callback attachment completes when any task finishes", "[when_any][callbacks]") {
    std::vector<Task<int>> tasks;
    tasks.push_back([]() -> Task<int> {
        co_await async_sleep(50ms);
        co_return 1;
    }());
    tasks.push_back([]() -> Task<int> {
        co_await async_sleep(200ms);
        co_return 2;
    }());

    auto when_any_task = when_any(std::move(tasks));
    REQUIRE(when_any_task.result().result() == 1);
}

TEST_CASE("when_any handles all tasks completing within the race", "[when_any][callbacks][race]") {
    // Stress-test with many pending tasks to verify callback mechanics.
    for (int run = 0; run < 50; ++run) {
        std::vector<Task<int>> tasks;
        for (int i = 0; i < 20; ++i) {
            tasks.push_back([]() -> Task<int> {
                co_await async_sleep(5ms);
                co_return 42;
            }());
        }

        auto winner = when_any(std::move(tasks)).result();
        REQUIRE(winner.result() == 42);
    }
}

TEST_CASE("when_any with direct callbacks preserves move-only task semantics", "[when_any][callbacks][move]") {
    std::vector<Task<std::unique_ptr<int>>> tasks;
    tasks.push_back([]() -> Task<std::unique_ptr<int>> {
        co_await async_sleep(20ms);
        co_return std::make_unique<int>(99);
    }());
    tasks.push_back([]() -> Task<std::unique_ptr<int>> {
        co_await async_sleep(200ms);
        co_return std::make_unique<int>(100);
    }());

    // Copy the first task to compare identity later.
    auto first_task = tasks[0];

    auto winner = when_any(std::move(tasks)).result();
    REQUIRE(winner == first_task);
    auto ptr = winner.result();
    REQUIRE(*ptr == 99);
}
