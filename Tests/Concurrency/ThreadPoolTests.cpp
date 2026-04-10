/**
* @File ThreadPoolTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "Concurrency/Thread/ThreadPool.hpp"

namespace
{

using bee::BackpressurePolicy;
using bee::LifecyclePhase;
using bee::ShutdownMode;
using bee::ThreadPool;
using bee::ThreadPoolConfig;

// =============================================================================
// Basic functionality
// =============================================================================

TEST(ThreadPoolTest, SubmitReturnsCorrectResult)
{
    ThreadPool pool(4);

    auto future = pool.submit([](int a, int b) {
        return a + b;
    }, 20, 22);

    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, ExceptionCanPropagateToFuture)
{
    ThreadPool pool(2);

    auto future = pool.submit([]() -> int {
        throw std::runtime_error("expected failure");
    });

    EXPECT_THROW((void)future.get(), std::runtime_error);
}

TEST(ThreadPoolTest, PostCanProcessLargeAmountOfTasks)
{
    ThreadPool pool(std::max(2u, std::thread::hardware_concurrency()));

    constexpr std::size_t kTaskCount = 100000;
    std::atomic<std::size_t> counter{0};

    for (std::size_t i = 0; i < kTaskCount; ++i) {
        pool.post([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.wait_for_tasks();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), kTaskCount);
}

TEST(ThreadPoolTest, NestedSubmitWorksInsideWorkerThread)
{
    ThreadPool pool(4);

    auto outer = pool.submit([&pool] {
        auto inner = pool.submit([] {
            return 41;
        });
        return inner.get() + 1;
    });

    EXPECT_EQ(outer.get(), 42);
}

// =============================================================================
// Shutdown behavior
// =============================================================================

TEST(ThreadPoolTest, ShutdownRejectsNewTaskSubmission)
{
    ThreadPool pool(2);
    pool.shutdown();

    EXPECT_THROW(pool.submit([] {}), std::runtime_error);
    EXPECT_THROW(pool.post([] {}), std::runtime_error);
}

TEST(ThreadPoolTest, WaitForTasksBlocksUntilTaskCompletion)
{
    ThreadPool pool(2);

    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};

    pool.post([&] {
        started.store(true, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        finished.store(true, std::memory_order_release);
    });

    pool.wait_for_tasks();

    EXPECT_TRUE(started.load(std::memory_order_acquire));
    EXPECT_TRUE(finished.load(std::memory_order_acquire));
    EXPECT_EQ(pool.pending_tasks(), 0u);
}

TEST(ThreadPoolTest, ShutdownImmediateDropsQueuedTasks)
{
    ThreadPool pool(1);

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool allow_exit = false;
    std::atomic<std::size_t> executed{0};

    pool.post([&] {
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&] {
            return allow_exit;
        });
    });

    constexpr std::size_t kAttemptMax = 100000;
    std::size_t queued                = 0;
    std::size_t consecutive_failures  = 0;
    for (std::size_t i = 0; i < kAttemptMax; ++i) {
        if (pool.try_post([&executed] {
            executed.fetch_add(1, std::memory_order_relaxed);
        })) {
            ++queued;
            consecutive_failures = 0;
        } else {
            if (++consecutive_failures >= 10)
                break;
        }
    }
    ASSERT_GT(queued, 0u);

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        allow_exit = true;
    }
    gate_cv.notify_one();

    pool.shutdown(ShutdownMode::Immediate);

    EXPECT_LT(executed.load(std::memory_order_relaxed), queued);
}

// =============================================================================
// Multi-producer
// =============================================================================

TEST(ThreadPoolTest, MultiProducerPostDoesNotLoseTasks)
{
    ThreadPool pool(std::max(2u, std::thread::hardware_concurrency()));

    constexpr std::size_t kProducerCount    = 8;
    constexpr std::size_t kTasksPerProducer = 20000;
    std::atomic<std::size_t> counter{0};

    std::vector<std::thread> producers;
    producers.reserve(kProducerCount);

    for (std::size_t p = 0; p < kProducerCount; ++p) {
        producers.emplace_back([&pool, &counter] {
            for (std::size_t i = 0; i < kTasksPerProducer; ++i) {
                pool.post([&counter] {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    pool.wait_for_tasks();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), kProducerCount * kTasksPerProducer);
}

// =============================================================================
// Stats
// =============================================================================

TEST(ThreadPoolTest, StatsTracksSubmittedAndFinalized)
{
    ThreadPool pool(4);

    constexpr std::size_t kTaskCount = 5000;
    for (std::size_t i = 0; i < kTaskCount; ++i) {
        pool.post([] {
        });
    }

    pool.wait_for_tasks();
    const auto stats = pool.stats();

    EXPECT_EQ(stats.pending_tasks, 0u);
    EXPECT_EQ(stats.active_tasks, 0u);
    EXPECT_GE(stats.submitted_tasks, kTaskCount);
    EXPECT_GE(stats.finalized_tasks, kTaskCount);
}

TEST(ThreadPoolTest, StatsConcurrentAccessDoesNotCrash)
{
    ThreadPool pool(4);

    constexpr std::size_t kTaskCount = 10000;
    for (std::size_t i = 0; i < kTaskCount; ++i) {
        pool.post([] {
        });
    }

    std::vector<std::thread> readers;
    readers.reserve(4);
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&pool] {
            for (int j = 0; j < 1000; ++j) {
                auto s = pool.stats();
                (void)s.worker_count;
            }
        });
    }

    pool.wait_for_tasks();
    for (auto& t : readers) {
        t.join();
    }

    const auto final_stats = pool.stats();
    EXPECT_EQ(final_stats.pending_tasks, 0u);
    EXPECT_EQ(final_stats.finalized_tasks, kTaskCount);
}

// =============================================================================
// try_post / try_submit
// =============================================================================

TEST(ThreadPoolTest, TryPostCanFailFastWhenQueueIsSaturated)
{
    ThreadPool pool(1);

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool allow_exit = false;

    pool.post([&] {
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&] {
            return allow_exit;
        });
    });

    bool saw_failure = false;
    for (std::size_t i = 0; i < 20000; ++i) {
        if (!pool.try_post([] {
        })) {
            saw_failure = true;
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        allow_exit = true;
    }
    gate_cv.notify_one();

    pool.wait_for_tasks();
    EXPECT_TRUE(saw_failure);
}

TEST(ThreadPoolTest, TrySubmitReturnsFutureOnSuccess)
{
    ThreadPool pool(2);

    auto maybe_future = pool.try_submit([](int x) {
        return x * 2;
    }, 21);

    ASSERT_TRUE(maybe_future.has_value());
    EXPECT_EQ(maybe_future->get(), 42);
}

// =============================================================================
// Cancellation
// =============================================================================

TEST(ThreadPoolTest, CancellableSubmitCanObserveRequestedStop)
{
    ThreadPool pool(2);

    auto source = ThreadPool::make_cancellation_source();
    source.request_stop();

    auto future = pool.submit_cancellable(source.token(), [](ThreadPool::CancellationToken token) -> int {
        if (token.stop_requested()) {
            throw std::runtime_error("cancelled");
        }
        return 7;
    });

    EXPECT_THROW((void)future.get(), std::runtime_error);
}

TEST(ThreadPoolTest, TryPostCancellableSkipsWhenTokenAlreadyStopped)
{
    ThreadPool pool(2);

    auto source = ThreadPool::make_cancellation_source();
    source.request_stop();

    std::atomic<int> executed{0};
    ASSERT_TRUE(pool.try_post_cancellable(source.token(), [&executed](ThreadPool::CancellationToken) {
        executed.fetch_add(1, std::memory_order_relaxed);
        }));

    pool.wait_for_tasks();
    EXPECT_EQ(executed.load(std::memory_order_relaxed), 0);
}

TEST(ThreadPoolTest, TryPostCancellableExecutesWhenTokenNotStopped)
{
    ThreadPool pool(2);

    auto source = ThreadPool::make_cancellation_source();
    std::atomic<int> executed{0};

    ASSERT_TRUE(pool.try_post_cancellable(source.token(), [&executed](ThreadPool::CancellationToken) {
        executed.fetch_add(1, std::memory_order_relaxed);
        }));

    pool.wait_for_tasks();
    EXPECT_EQ(executed.load(std::memory_order_relaxed), 1);
}

TEST(ThreadPoolTest, TryPostCancellableWithNonTokenFunction)
{
    ThreadPool pool(2);

    auto source = ThreadPool::make_cancellation_source();
    std::atomic<int> executed{0};

    ASSERT_TRUE(pool.try_post_cancellable(source.token(), [&executed](int x) {
        executed.store(x, std::memory_order_relaxed);
        }, 42));

    pool.wait_for_tasks();
    EXPECT_EQ(executed.load(std::memory_order_relaxed), 42);
}

// =============================================================================
// Worker thread identity
// =============================================================================

TEST(ThreadPoolTest, WorkerThreadIdentity)
{
    ThreadPool pool(4);

    EXPECT_FALSE(ThreadPool::is_worker_thread());
    EXPECT_EQ(ThreadPool::current_worker_index(), static_cast<std::size_t>(-1));

    std::atomic<std::size_t> remaining{pool.thread_count()};
    for (std::size_t i = 0; i < pool.thread_count(); ++i) {
        pool.post([&remaining] {
            EXPECT_TRUE(ThreadPool::is_worker_thread());
            auto idx = ThreadPool::current_worker_index();
            EXPECT_NE(idx, static_cast<std::size_t>(-1));
            remaining.fetch_sub(1, std::memory_order_acq_rel);
        });
    }

    pool.wait_for_tasks();
    EXPECT_EQ(remaining.load(std::memory_order_acquire), 0u);
}

// =============================================================================
// Work stealing verification
// =============================================================================

TEST(ThreadPoolTest, WorkStealingEffect)
{
    ThreadPool pool(1);

    constexpr std::size_t kTaskCount = 1000;
    std::atomic<std::size_t> counter{0};

    for (std::size_t i = 0; i < kTaskCount; ++i) {
        pool.post([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.wait_for_tasks();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), kTaskCount);

    const auto stats = pool.stats();
    EXPECT_GT(stats.global_pop_hits, 0u);
}

// =============================================================================
// RAII and lifecycle
// =============================================================================

TEST(ThreadPoolTest, RAIIDestructorShutdown)
{
    {
        ThreadPool pool(2);
        for (std::size_t i = 0; i < 100; ++i) {
            pool.post([] {
            });
        }
    }
}

TEST(ThreadPoolTest, LifecyclePhaseReachesStoppedAfterShutdown)
{
    ThreadPool pool(2);
    pool.post([] {
    });
    pool.shutdown(ShutdownMode::Drain);

    const auto s = pool.stats();
    EXPECT_EQ(s.phase, LifecyclePhase::Stopped);
    EXPECT_EQ(s.pending_tasks, 0u);
}

TEST(ThreadPoolTest, NestedSubmitExceptionPropagation)
{
    ThreadPool pool(2);

    auto outer = pool.submit([&pool]() -> int {
        auto inner = pool.submit([]() -> int {
            throw std::runtime_error("inner error");
        });
        return inner.get();
    });

    EXPECT_THROW((void)outer.get(), std::runtime_error);
}

// =============================================================================
// Concurrency safety / deadlock prevention
// =============================================================================

TEST(ThreadPoolTest, ConcurrentShutdownIsIdempotent)
{
    ThreadPool pool(4);

    for (std::size_t i = 0; i < 100; ++i) {
        pool.post([] {
        });
    }

    std::vector<std::thread> threads;
    threads.reserve(4);
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&pool] {
            pool.shutdown(ShutdownMode::Drain);
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

TEST(ThreadPoolTest, ShutdownImmediateWithConcurrentPosts)
{
    ThreadPool pool(4);

    std::atomic<bool> stop_flag{false};
    constexpr std::size_t kProducers = 4;

    std::vector<std::thread> producers;
    producers.reserve(kProducers);

    for (std::size_t p = 0; p < kProducers; ++p) {
        producers.emplace_back([&pool, &stop_flag] {
            while (!stop_flag.load(std::memory_order_acquire)) {
                try {
                    pool.post([] {
                    });
                } catch (...) {
                    break;
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    stop_flag.store(true, std::memory_order_release);
    pool.shutdown(ShutdownMode::Immediate);

    for (auto& t : producers) {
        t.join();
    }
}

TEST(ThreadPoolTest, RepeatedSmallBurstsDoNotStall)
{
    ThreadPool pool(2);

    constexpr std::size_t kRounds = 5000;
    std::atomic<std::size_t> completed{0};

    for (std::size_t i = 0; i < kRounds; ++i) {
        std::atomic<bool> task_done{false};
        pool.post([&task_done, &completed] {
            completed.fetch_add(1, std::memory_order_relaxed);
            task_done.store(true, std::memory_order_release);
        });

        while (!task_done.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        pool.wait_for_tasks();
    }

    EXPECT_EQ(completed.load(std::memory_order_relaxed), kRounds);
}

TEST(ThreadPoolTest, BurstWorkloadMaintainsTaskAccountingInvariants)
{
    ThreadPool pool(2);

    constexpr std::size_t kRounds        = 2000;
    constexpr std::size_t kTasksPerRound = 3;

    for (std::size_t i = 0; i < kRounds; ++i) {
        for (std::size_t j = 0; j < kTasksPerRound; ++j) {
            pool.post([] {
            });
        }
        pool.wait_for_tasks();
    }

    const auto s = pool.stats();
    EXPECT_EQ(s.pending_tasks, 0u);
    EXPECT_EQ(s.active_tasks, 0u);
    EXPECT_EQ(s.submitted_tasks, s.finalized_tasks);
}

TEST(ThreadPoolTest, WaitForTasksWithConcurrentTryPostFailuresDoesNotStall)
{
    ThreadPool pool(1);

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool allow_exit = false;

    pool.post([&] {
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&] {
            return allow_exit;
        });
    });

    auto waiter = std::async(std::launch::async, [&pool] {
        pool.wait_for_tasks();
    });

    std::atomic<bool> stop_submitter{false};
    std::thread submitter([&] {
        while (!stop_submitter.load(std::memory_order_acquire)) {
            (void)pool.try_post([] {
            });
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        allow_exit = true;
    }
    gate_cv.notify_one();

    stop_submitter.store(true, std::memory_order_release);
    submitter.join();

    EXPECT_EQ(waiter.wait_for(std::chrono::seconds(5)), std::future_status::ready);
}

TEST(ThreadPoolTest, RepeatedConcurrentShutdownAndPostingDoesNotDeadlock)
{
    constexpr int kRounds            = 24;
    constexpr std::size_t kProducers = 4;

    for (int round = 0; round < kRounds; ++round) {
        ThreadPool pool(4);
        std::atomic<bool> stop_flag{false};

        std::vector<std::thread> producers;
        producers.reserve(kProducers);
        for (std::size_t p = 0; p < kProducers; ++p) {
            producers.emplace_back([&pool, &stop_flag] {
                while (!stop_flag.load(std::memory_order_acquire)) {
                    try {
                        pool.post([] {
                        });
                    } catch (...) {
                        break;
                    }
                }
            });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto shutdown_future =
                std::async(std::launch::async, [&pool] {
                    pool.shutdown(ShutdownMode::Immediate);
                });

        stop_flag.store(true, std::memory_order_release);

        for (auto& t : producers) {
            t.join();
        }

        ASSERT_EQ(shutdown_future.wait_for(std::chrono::seconds(5)), std::future_status::ready)
            << "round=" << round;
    }
}

// =============================================================================
// Timed wait
// =============================================================================

TEST(ThreadPoolTest, WaitForTasksForCanTimeoutAndThenComplete)
{
    ThreadPool pool(1);

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool allow_exit = false;

    pool.post([&] {
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&] {
            return allow_exit;
        });
    });

    EXPECT_FALSE(pool.wait_for_tasks_for(std::chrono::milliseconds(10)));

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        allow_exit = true;
    }
    gate_cv.notify_one();

    EXPECT_TRUE(pool.wait_for_tasks_for(std::chrono::seconds(2)));
}

// =============================================================================
// Backpressure policies
// =============================================================================

TEST(ThreadPoolTest, CallerRunsBackpressureCanExecuteInlineWhenSaturated)
{
    ThreadPoolConfig cfg;
    cfg.thread_count          = 1;
    cfg.backpressure_policy   = BackpressurePolicy::CallerRuns;
    cfg.enqueue_block_timeout = std::chrono::milliseconds(5);

    ThreadPool pool(cfg);

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool allow_exit = false;

    pool.post([&] {
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&] {
            return allow_exit;
        });
    });

    std::atomic<int> inline_executed{0};
    bool saw_inline = false;
    for (int i = 0; i < 50000; ++i) {
        (void)pool.try_post([&inline_executed] {
            inline_executed.fetch_add(1, std::memory_order_relaxed);
        });
        if (inline_executed.load(std::memory_order_acquire) > 0) {
            saw_inline = true;
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        allow_exit = true;
    }
    gate_cv.notify_one();

    pool.wait_for_tasks();
    EXPECT_TRUE(saw_inline);
}

TEST(ThreadPoolTest, BlockBackpressureRespectsTimeoutUnderSaturation)
{
    ThreadPoolConfig cfg;
    cfg.thread_count          = 1;
    cfg.backpressure_policy   = BackpressurePolicy::Block;
    cfg.enqueue_block_timeout = std::chrono::milliseconds(15);

    ThreadPool pool(cfg);

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool allow_exit = false;

    pool.post([&] {
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_cv.wait(lock, [&] {
            return allow_exit;
        });
    });

    for (int i = 0; i < 12000; ++i) {
        if (!pool.try_post([] {
        })) {
            break;
        }
    }

    const auto begin = std::chrono::steady_clock::now();
    const bool ok    = pool.try_post([] {
    });
    const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin);

    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        allow_exit = true;
    }
    gate_cv.notify_one();

    pool.wait_for_tasks();

    EXPECT_FALSE(ok);
    EXPECT_GE(elapsed.count(), 10);
}

// =============================================================================
// Soak test
// =============================================================================

TEST(ThreadPoolTest, SoakMatrixModesAndThreadCountsNoDeadlock)
{
    constexpr std::array<std::size_t, 3> kThreadCounts{1, 2, 4};
    constexpr std::array<ShutdownMode, 2> kModes{ShutdownMode::Drain, ShutdownMode::Immediate};
    constexpr int kRounds = 6;

    for (auto mode : kModes) {
        for (auto threads : kThreadCounts) {
            for (int round = 0; round < kRounds; ++round) {
                ThreadPool pool(threads);
                std::atomic<bool> stop_flag{false};

                std::vector<std::thread> producers;
                producers.reserve(3);
                for (int p = 0; p < 3; ++p) {
                    producers.emplace_back([&pool, &stop_flag] {
                        while (!stop_flag.load(std::memory_order_acquire)) {
                            try {
                                pool.post([] {
                                });
                            } catch (...) {
                                break;
                            }
                        }
                    });
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(8));

                auto shutdown_future =
                        std::async(std::launch::async, [&pool, mode] {
                            pool.shutdown(mode);
                        });

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

TEST(ThreadPoolTest, AdaptiveGlobalProbeBudgetAlwaysInValidRange)
{
    ThreadPool pool(4);

    for (std::size_t i = 0; i < 20000; ++i) {
        pool.post([] {
        });
    }
    pool.wait_for_tasks();

    const auto s = pool.stats();
    EXPECT_GE(s.global_probe_budget, 1u);
    EXPECT_LE(s.global_probe_budget, 16u);
}

} // namespace
