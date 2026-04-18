/**
 * @File WhenAllTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief when_all() 组合子测试。
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "Task/Core/WhenAll.hpp"
#include "Task/Core/Scheduler.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// ── Variadic when_all ──

TEST(WhenAllTests, VariadicTwoInts)
{
    WorkPool pool(2);

    auto t1 = spawn_task(pool, [] { return 10; });
    auto t2 = spawn_task(pool, [] { return 20; });

    auto combined = when_all(std::move(t1), std::move(t2));
    auto [a, b]   = combined.get();

    EXPECT_EQ(a, 10);
    EXPECT_EQ(b, 20);
    pool.shutdown();
}

TEST(WhenAllTests, VariadicHeterogeneous)
{
    WorkPool pool(2);

    auto t1 = spawn_task(pool, [] { return 42; });
    auto t2 = spawn_task(pool, [] { return std::string("hello"); });
    auto t3 = spawn_task(pool, [] { return 3.14; });

    auto combined  = when_all(std::move(t1), std::move(t2), std::move(t3));
    auto [i, s, d] = combined.get();

    EXPECT_EQ(i, 42);
    EXPECT_EQ(s, "hello");
    EXPECT_DOUBLE_EQ(d, 3.14);
    pool.shutdown();
}

TEST(WhenAllTests, VariadicWithVoid)
{
    WorkPool pool(2);

    std::atomic<int> counter{0};
    auto t1 = spawn_task(pool, [&] { counter.fetch_add(1); });
    auto t2 = spawn_task(pool, [&] { counter.fetch_add(1); });

    auto combined = when_all(std::move(t1), std::move(t2));
    combined.get();

    EXPECT_EQ(counter.load(), 2);
    pool.shutdown();
}

TEST(WhenAllTests, VariadicSingleTask)
{
    auto t = []() -> Task<int> { co_return 7; }();

    auto combined = when_all(std::move(t));
    auto [v]      = combined.get();

    EXPECT_EQ(v, 7);
}

TEST(WhenAllTests, VariadicExceptionPropagation)
{
    WorkPool pool(2);

    auto t1 = spawn_task(pool, [] { return 1; });
    auto t2 = spawn_task(pool, []() -> int { throw std::runtime_error("fail"); });

    auto combined = when_all(std::move(t1), std::move(t2));
    EXPECT_THROW(combined.get(), std::runtime_error);

    pool.shutdown();
}

// ── Vector when_all ──

TEST(WhenAllTests, VectorBasic)
{
    WorkPool pool(4);

    std::vector<Task<int>> tasks;
    for (int i = 0; i < 10; ++i) {
        tasks.push_back(spawn_task(pool, [i] { return i * i; }));
    }

    auto combined = when_all(std::move(tasks));
    auto results  = combined.get();

    ASSERT_EQ(results.size(), 10u);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(results[i], i * i);
    }
    pool.shutdown();
}

TEST(WhenAllTests, VectorEmpty)
{
    std::vector<Task<int>> tasks;
    auto combined = when_all(std::move(tasks));
    auto results  = combined.get();

    EXPECT_TRUE(results.empty());
}

TEST(WhenAllTests, VectorSingleElement)
{
    auto t = []() -> Task<int> { co_return 42; }();

    std::vector<Task<int>> tasks;
    tasks.push_back(std::move(t));

    auto combined = when_all(std::move(tasks));
    auto results  = combined.get();

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], 42);
}

TEST(WhenAllTests, VectorExceptionPropagation)
{
    WorkPool pool(4);

    std::vector<Task<int>> tasks;
    tasks.push_back(spawn_task(pool, [] { return 1; }));
    tasks.push_back(spawn_task(pool, []() -> int { throw std::logic_error("vec fail"); }));
    tasks.push_back(spawn_task(pool, [] { return 3; }));

    auto combined = when_all(std::move(tasks));
    EXPECT_THROW(combined.get(), std::logic_error);

    pool.shutdown();
}

// ── 大规模并发 ──

TEST(WhenAllTests, VectorLargeConcurrency)
{
    WorkPool pool(8);
    constexpr int N = 1000;

    std::vector<Task<int>> tasks;
    tasks.reserve(N);
    for (int i = 0; i < N; ++i) {
        tasks.push_back(spawn_task(pool, [i] { return i; }));
    }

    auto combined = when_all(std::move(tasks));
    auto results  = combined.get();

    ASSERT_EQ(results.size(), static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(results[i], i);
    }
    pool.shutdown();
}

// ── 混合类型链 ──

TEST(WhenAllTests, VariadicMixedValueAndVoid)
{
    WorkPool pool(2);

    std::atomic<int> side{0};
    auto t1 = spawn_task(pool, [] { return 100; });
    auto t2 = spawn_task(pool, [&side] { side.store(1); });

    auto combined = when_all(std::move(t1), std::move(t2));
    auto [val, _] = combined.get();

    EXPECT_EQ(val, 100);
    EXPECT_EQ(side.load(), 1);
    pool.shutdown();
}
