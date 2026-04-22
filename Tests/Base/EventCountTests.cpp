/**
 * @File EventCountTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/20
 * @Brief This file is part of Bee.
 */

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "Base/Sync/EventCount.hpp"

using namespace bee;
using namespace std::chrono_literals;

namespace
{

TEST(EventCountTests, NotifyNoWaitersIsNoOp)
{
    EventCount ec;
    // 无等待者时 notify 不应崩溃，epoch 仍会递增（但无感知）
    ec.notify();
    ec.notify_all();
    SUCCEED();
}

TEST(EventCountTests, CancelWaitDoesNotDeadlock)
{
    EventCount ec;
    auto       key = ec.prepare_wait();
    ec.cancel_wait();
    // 如果 waiter 计数泄漏，后续 notify 会尝试 syscall，但不会卡住
    ec.notify();
    SUCCEED();
}

TEST(EventCountTests, SingleWaiterAwakenedByNotify)
{
    EventCount        ec;
    std::atomic<bool> ready{false};
    std::atomic<bool> awoke{false};

    std::thread waiter([&] {
        while (!ready.load(std::memory_order_acquire)) {
            auto key = ec.prepare_wait();
            if (ready.load(std::memory_order_acquire)) {
                ec.cancel_wait();
                break;
            }
            ec.wait(key);
        }
        awoke.store(true, std::memory_order_release);
    });

    std::this_thread::sleep_for(20ms);
    EXPECT_FALSE(awoke.load());

    ready.store(true, std::memory_order_release);
    ec.notify();

    waiter.join();
    EXPECT_TRUE(awoke.load());
}

TEST(EventCountTests, AwaitHelperMatchesManualPattern)
{
    EventCount       ec;
    std::atomic<int> value{0};

    std::thread waiter([&] { ec.await([&] { return value.load(std::memory_order_acquire) == 42; }); });

    std::this_thread::sleep_for(20ms);
    value.store(42, std::memory_order_release);
    ec.notify();

    waiter.join();
    EXPECT_EQ(value.load(), 42);
}

TEST(EventCountTests, AwaitReturnsImmediatelyIfPredicateTrue)
{
    EventCount       ec;
    std::atomic<int> calls{0};
    ec.await([&] {
        ++calls;
        return true;
    });
    // 预检为 true，应该只调用 pred 一次即返回
    EXPECT_EQ(calls.load(), 1);
}

TEST(EventCountTests, NotifyAllWakesEveryWaiter)
{
    EventCount        ec;
    constexpr int     kN = 8;
    std::atomic<int>  ready{0};
    std::atomic<int>  awoke{0};
    std::atomic<bool> go{false};
    std::latch        all_in(kN);

    std::vector<std::thread> ts;
    for (int i = 0; i < kN; ++i) {
        ts.emplace_back([&] {
            all_in.count_down();
            ec.await([&] { return go.load(std::memory_order_acquire); });
            awoke.fetch_add(1, std::memory_order_relaxed);
        });
    }
    all_in.wait();
    std::this_thread::sleep_for(20ms);
    go.store(true, std::memory_order_release);
    ec.notify_all();

    for (auto& t : ts)
        t.join();
    EXPECT_EQ(awoke.load(), kN);
}

TEST(EventCountTests, ProducerConsumerStress)
{
    EventCount    ec;
    constexpr int kProducers = 2;
    constexpr int kConsumers = 2;
    constexpr int kItems     = 2000;

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    std::vector<std::thread> consumers;
    for (int i = 0; i < kConsumers; ++i) {
        consumers.emplace_back([&] {
            while (true) {
                ec.await([&] {
                    return consumed.load(std::memory_order_acquire) < produced.load(std::memory_order_acquire) ||
                           produced.load(std::memory_order_acquire) >= kItems * kProducers;
                });
                // 原子 CAS：只在 consumed < produced 时才 +1
                int c = consumed.load(std::memory_order_acquire);
                int p = produced.load(std::memory_order_acquire);
                if (c < p) {
                    if (consumed.compare_exchange_weak(c, c + 1, std::memory_order_acq_rel)) {
                        // 成功消费一个
                    }
                    continue;
                }
                if (p >= kItems * kProducers)
                    break;
            }
        });
    }

    std::vector<std::thread> producers;
    for (int i = 0; i < kProducers; ++i) {
        producers.emplace_back([&] {
            for (int k = 0; k < kItems; ++k) {
                produced.fetch_add(1, std::memory_order_acq_rel);
                ec.notify();
            }
        });
    }

    for (auto& t : producers)
        t.join();
    // 所有生产完成后，唤醒可能卡在 wait 的消费者（边界：p == c）
    ec.notify_all();
    for (auto& t : consumers)
        t.join();

    EXPECT_EQ(consumed.load(), kItems * kProducers);
    EXPECT_EQ(produced.load(), kItems * kProducers);
}

} // namespace
