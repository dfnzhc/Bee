/**
 * @File WorkPoolTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/17
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include "Concurrency/Thread/WorkPool.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <numeric>
#include <thread>
#include <vector>

namespace
{

using namespace bee;

// =============================================================================
// 基础功能
// =============================================================================

TEST(WorkPoolTest, PostSingleTaskExecutes)
{
    WorkPool         pool(2);
    std::atomic<int> counter{0};

    pool.post([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    pool.wait_for_tasks();

    EXPECT_EQ(counter.load(), 1);
}

TEST(WorkPoolTest, PostBatchTasksAllExecute)
{
    WorkPool         pool(4);
    constexpr int    kCount = 10000;
    std::atomic<int> counter{0};

    for (int i = 0; i < kCount; ++i) {
        pool.post([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }

    pool.wait_for_tasks();
    EXPECT_EQ(counter.load(), kCount);
}

TEST(WorkPoolTest, SubmitReturnsCorrectValue)
{
    WorkPool pool(2);
    auto     future = pool.submit([] { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST(WorkPoolTest, SubmitPropagatesException)
{
    WorkPool pool(2);
    auto     future = pool.submit([]() -> int { throw std::runtime_error("oops"); });
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(WorkPoolTest, ThreadCountReturnsConfigured)
{
    WorkPool pool(3);
    EXPECT_EQ(pool.thread_count(), 3u);
}

TEST(WorkPoolTest, ThreadCountAtLeastOne)
{
    WorkPoolConfig cfg;
    cfg.thread_count = 0;
    WorkPool pool(cfg);
    EXPECT_GE(pool.thread_count(), 1u);
}

TEST(WorkPoolTest, PendingTasksReachesZeroAfterWait)
{
    WorkPool pool(2);
    for (int i = 0; i < 100; ++i) {
        pool.post([] { std::this_thread::yield(); });
    }
    pool.wait_for_tasks();
    EXPECT_EQ(pool.pending_tasks(), 0u);
}

TEST(WorkPoolTest, StatsCompletedCountAccurate)
{
    WorkPool      pool(2);
    constexpr int kCount = 500;

    for (int i = 0; i < kCount; ++i) {
        pool.post([] {});
    }
    pool.wait_for_tasks();

    auto s = pool.stats();
    EXPECT_EQ(s.completed_tasks, static_cast<std::uint64_t>(kCount));
}

// =============================================================================
// 优先级调度
// =============================================================================

TEST(WorkPoolTest, HighPriorityExecutesBeforeLow)
{
    // 使用 1 个工作线程，确保从全局队列严格顺序消费
    WorkPool pool(1);

    // 使用 promise/future 精确同步，取代不可靠的 sleep
    std::promise<void> gate_entered;
    std::promise<void> gate_release;
    auto               entered_future = gate_entered.get_future();
    auto               release_future = gate_release.get_future();

    // 门控任务：进入后通知主线程，然后阻塞等待放行
    pool.post([&] {
        gate_entered.set_value();
        release_future.wait();
    });

    // 等待工作线程确实已进入门控任务
    entered_future.wait();

    std::vector<int> execution_order;
    std::mutex       order_mutex;

    auto record = [&](int id) {
        return [&execution_order, &order_mutex, id] {
            std::lock_guard lock(order_mutex);
            execution_order.push_back(id);
        };
    };

    // 先提交 Low，再提交 High —— 期望 High 先执行
    pool.post(record(1), TaskPriority::Low);
    pool.post(record(2), TaskPriority::Low);
    pool.post(record(3), TaskPriority::High);
    pool.post(record(4), TaskPriority::High);

    // 放开门控
    gate_release.set_value();

    pool.wait_for_tasks();

    // High(3,4) 应在 Low(1,2) 之前执行
    ASSERT_EQ(execution_order.size(), 4u);

    auto first_high = std::find_if(execution_order.begin(), execution_order.end(), [](int v) { return v >= 3; });
    auto first_low  = std::find_if(execution_order.begin(), execution_order.end(), [](int v) { return v <= 2; });

    EXPECT_LT(std::distance(execution_order.begin(), first_high), std::distance(execution_order.begin(), first_low))
        << "High 优先级任务应在 Low 优先级任务之前执行";
}

TEST(WorkPoolTest, DefaultPriorityIsNormal)
{
    WorkPool         pool(2);
    std::atomic<int> counter{0};

    // 不传优先级参数，默认应为 Normal
    pool.post([&counter] { counter.fetch_add(1); });
    pool.wait_for_tasks();

    EXPECT_EQ(counter.load(), 1);
}

// =============================================================================
// try_post 与关停后行为
// =============================================================================

TEST(WorkPoolTest, TryPostReturnsFalseAfterShutdown)
{
    WorkPool pool(2);
    pool.shutdown();
    EXPECT_FALSE(pool.try_post([] {}));
}

TEST(WorkPoolTest, PostThrowsAfterShutdown)
{
    WorkPool pool(2);
    pool.shutdown();
    EXPECT_THROW(pool.post([] {}), std::runtime_error);
}

TEST(WorkPoolTest, SubmitThrowsAfterShutdown)
{
    WorkPool pool(2);
    pool.shutdown();
    EXPECT_THROW((void)pool.submit([] { return 1; }), std::runtime_error);
}

// =============================================================================
// 关停模式
// =============================================================================

TEST(WorkPoolTest, ShutdownDrainWaitsForAllTasks)
{
    WorkPool         pool(2);
    std::atomic<int> counter{0};

    for (int i = 0; i < 1000; ++i) {
        pool.post([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }

    pool.shutdown(ShutdownMode::Drain);
    EXPECT_EQ(counter.load(), 1000);
}

TEST(WorkPoolTest, ShutdownImmediateExitsPromptly)
{
    WorkPool pool(2);

    // 提交长时间运行的任务
    std::atomic<bool> stop{false};
    for (int i = 0; i < 100; ++i) {
        (void)pool.try_post([&stop] {
            while (!stop.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto start = std::chrono::steady_clock::now();
    stop.store(true, std::memory_order_release);
    pool.shutdown(ShutdownMode::Immediate);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 5);
}

TEST(WorkPoolTest, DestructorImplicitlyDrains)
{
    std::atomic<int> counter{0};
    {
        WorkPool pool(2);
        for (int i = 0; i < 500; ++i) {
            pool.post([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
        }
        // 析构函数自动调用 shutdown(Drain)
    }
    EXPECT_EQ(counter.load(), 500);
}

TEST(WorkPoolTest, DoubleShutdownIsIdempotent)
{
    WorkPool pool(2);
    pool.post([] {});
    pool.shutdown(ShutdownMode::Drain);
    pool.shutdown(ShutdownMode::Drain); // 不应挂起或崩溃
}

// =============================================================================
// 等待超时
// =============================================================================

TEST(WorkPoolTest, WaitForTasksForCanTimeout)
{
    WorkPool pool(1);

    std::mutex              gate_mutex;
    std::condition_variable gate_cv;
    bool                    allow_exit = false;

    pool.post([&] {
        std::unique_lock lock(gate_mutex);
        gate_cv.wait(lock, [&] { return allow_exit; });
    });

    // 任务还被阻塞，超时应返回 false
    EXPECT_FALSE(pool.wait_for_tasks_for(std::chrono::milliseconds(10)));

    // 放行后应在合理时间内完成
    {
        std::lock_guard lock(gate_mutex);
        allow_exit = true;
    }
    gate_cv.notify_one();

    EXPECT_TRUE(pool.wait_for_tasks_for(std::chrono::seconds(2)));
}

// =============================================================================
// 多线程并发提交
// =============================================================================

TEST(WorkPoolTest, ConcurrentSubmitFromMultipleThreads)
{
    WorkPool         pool(4);
    constexpr int    kProducers   = 4;
    constexpr int    kPerProducer = 5000;
    std::atomic<int> counter{0};

    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&] {
            for (int i = 0; i < kPerProducer; ++i) {
                pool.post([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    pool.wait_for_tasks();

    EXPECT_EQ(counter.load(), kProducers * kPerProducer);
}

// =============================================================================
// 嵌套提交
// =============================================================================

TEST(WorkPoolTest, NestedPostFromWorkerDoesNotDeadlock)
{
    WorkPool         pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool.post([&] { pool.post([&counter] { counter.fetch_add(1, std::memory_order_relaxed); }); });
    }

    pool.wait_for_tasks();
    EXPECT_EQ(counter.load(), 100);
}

TEST(WorkPoolTest, SingleWorkerNestedSubmitGetDoesNotDeadlock)
{
    // 单工作线程下 submit().get() 内联执行，不应自死锁
    WorkPool pool(1);

    auto future = pool.submit([&pool] {
        auto inner = pool.submit([] { return 99; });
        return inner.get();
    });

    EXPECT_EQ(future.get(), 99);
}

// =============================================================================
// 工作窃取
// =============================================================================

TEST(WorkPoolTest, WorkStealingOccursUnderUnevenLoad)
{
    WorkPool pool(4);

    // 从单个工作线程内部批量提交，制造负载不均
    auto future = pool.submit([&pool] {
        for (int i = 0; i < 10000; ++i) {
            pool.post([] { /* 轻量工作 */ });
        }
    });

    future.get();
    pool.wait_for_tasks();

    auto s = pool.stats();
    EXPECT_GT(s.stolen_tasks, 0u) << "4 个工作线程 + 不均匀负载，应发生窃取";
}

// =============================================================================
// is_worker_thread
// =============================================================================

TEST(WorkPoolTest, IsWorkerThreadReturnsTrueInsideWorker)
{
    WorkPool          pool(2);
    std::atomic<bool> was_worker{false};

    pool.post([&was_worker] { was_worker.store(WorkPool::is_worker_thread(), std::memory_order_release); });
    pool.wait_for_tasks();

    EXPECT_TRUE(was_worker.load());
}

TEST(WorkPoolTest, IsWorkerThreadReturnsFalseOutsideWorker)
{
    EXPECT_FALSE(WorkPool::is_worker_thread());
}

// =============================================================================
// 压力测试：多种线程数 × 多种关停模式，确认无死锁
// =============================================================================

TEST(WorkPoolTest, SoakMultiModeNoDeadlock)
{
    constexpr std::array<std::size_t, 3>  kThreadCounts{1, 2, 4};
    constexpr std::array<ShutdownMode, 2> kModes{ShutdownMode::Drain, ShutdownMode::Immediate};
    constexpr int                         kRounds = 4;

    for (auto mode : kModes) {
        for (auto threads : kThreadCounts) {
            for (int round = 0; round < kRounds; ++round) {
                WorkPool          pool(threads);
                std::atomic<bool> stop_flag{false};

                // 持续生产任务的外部线程
                std::vector<std::thread> producers;
                producers.reserve(3);
                for (int p = 0; p < 3; ++p) {
                    producers.emplace_back([&pool, &stop_flag] {
                        while (!stop_flag.load(std::memory_order_acquire)) {
                            (void)pool.try_post([] {});
                        }
                    });
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(8));

                auto shutdown_future = std::async(std::launch::async, [&pool, mode] { pool.shutdown(mode); });

                stop_flag.store(true, std::memory_order_release);
                for (auto& t : producers) {
                    t.join();
                }

                ASSERT_EQ(shutdown_future.wait_for(std::chrono::seconds(5)), std::future_status::ready)
                    << "mode=" << static_cast<int>(mode) << " threads=" << threads << " round=" << round;
            }
        }
    }
}

// =============================================================================
// 补充测试：TLS 隔离、空等待、环形回绕
// =============================================================================

TEST(WorkPoolTest, MultiplePoolInstancesDoNotInterfere)
{
    // 两个独立工作池，互相不影响
    WorkPool         pool_a(2);
    WorkPool         pool_b(2);
    std::atomic<int> counter_a{0};
    std::atomic<int> counter_b{0};

    for (int i = 0; i < 500; ++i) {
        pool_a.post([&counter_a] { counter_a.fetch_add(1, std::memory_order_relaxed); });
        pool_b.post([&counter_b] { counter_b.fetch_add(1, std::memory_order_relaxed); });
    }

    pool_a.wait_for_tasks();
    pool_b.wait_for_tasks();

    EXPECT_EQ(counter_a.load(), 500);
    EXPECT_EQ(counter_b.load(), 500);
}

TEST(WorkPoolTest, WaitForTasksReturnsImmediatelyWhenNoPending)
{
    // 没有任何待处理任务时 wait_for_tasks 应立即返回
    WorkPool pool(2);
    pool.wait_for_tasks(); // 不应挂起

    auto ok = pool.wait_for_tasks_for(std::chrono::milliseconds(1));
    EXPECT_TRUE(ok);
}

TEST(WorkPoolTest, LocalDequeWraparoundStressTest)
{
    // 对本地 deque 进行大量入队出队，验证环形缓冲区槽位复用的安全性
    WorkPoolConfig cfg;
    cfg.thread_count         = 4;
    cfg.local_queue_capacity = 64; // 较小容量以加速回绕

    WorkPool         pool(cfg);
    constexpr int    kBatches  = 50;
    constexpr int    kPerBatch = 200;
    std::atomic<int> counter{0};

    for (int b = 0; b < kBatches; ++b) {
        for (int i = 0; i < kPerBatch; ++i) {
            pool.post([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
        }
        pool.wait_for_tasks();
    }

    EXPECT_EQ(counter.load(), kBatches * kPerBatch);
}

} // namespace
