/**
 * @File ChaseLevDequeTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include "Concurrency/LockFree/ChaseLevDeque.hpp"

using namespace bee;

namespace
{

struct TrackedObject
{
    static inline std::atomic<int> live_count{0};

    int value = 0;

    explicit TrackedObject(int v = 0) noexcept
        : value(v)
    {
        live_count.fetch_add(1, std::memory_order_relaxed);
    }

    TrackedObject(const TrackedObject& other) noexcept
        : value(other.value)
    {
        live_count.fetch_add(1, std::memory_order_relaxed);
    }

    TrackedObject(TrackedObject&& other) noexcept
        : value(other.value)
    {
        live_count.fetch_add(1, std::memory_order_relaxed);
    }

    TrackedObject& operator=(const TrackedObject&) noexcept = default;
    TrackedObject& operator=(TrackedObject&&) noexcept      = default;

    ~TrackedObject()
    {
        live_count.fetch_sub(1, std::memory_order_relaxed);
    }
};

TEST(ChaseLevDequeTests, CapacityAtLeastOne)
{
    ChaseLevDeque<int> deque0(0);
    ChaseLevDeque<int> deque1(1);

    EXPECT_EQ(deque0.capacity(), 1u);
    EXPECT_EQ(deque1.capacity(), 1u);
}

TEST(ChaseLevDequeTests, OwnerPushAndPopIsLifo)
{
    ChaseLevDeque<int> deque(8);

    ASSERT_TRUE(deque.try_push(1));
    ASSERT_TRUE(deque.try_push(2));
    ASSERT_TRUE(deque.try_push(3));

    int value = 0;
    ASSERT_TRUE(deque.try_pop(value));
    EXPECT_EQ(value, 3);
    ASSERT_TRUE(deque.try_pop(value));
    EXPECT_EQ(value, 2);
    ASSERT_TRUE(deque.try_pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_FALSE(deque.try_pop(value));
    EXPECT_TRUE(deque.is_empty());
}

TEST(ChaseLevDequeTests, StealIsFifoFromTop)
{
    ChaseLevDeque<int> deque(8);

    ASSERT_TRUE(deque.try_push(10));
    ASSERT_TRUE(deque.try_push(20));
    ASSERT_TRUE(deque.try_push(30));

    int value = 0;
    ASSERT_TRUE(deque.try_steal(value));
    EXPECT_EQ(value, 10);
    ASSERT_TRUE(deque.try_steal(value));
    EXPECT_EQ(value, 20);

    ASSERT_TRUE(deque.try_pop(value));
    EXPECT_EQ(value, 30);
    EXPECT_TRUE(deque.is_empty());
}

TEST(ChaseLevDequeTests, FullConditionWithTryPush)
{
    ChaseLevDeque<int> deque(2);

    ASSERT_TRUE(deque.try_push(1));
    ASSERT_TRUE(deque.try_push(2));
    EXPECT_FALSE(deque.try_push(3));

    int value = 0;
    ASSERT_TRUE(deque.try_pop(value));
    EXPECT_EQ(value, 2);
}

TEST(ChaseLevDequeTests, SupportsMoveOnlyType)
{
    ChaseLevDeque<std::unique_ptr<int>> deque(4);
    ASSERT_TRUE(deque.try_push(std::make_unique<int>(42)));

    std::unique_ptr<int> out;
    ASSERT_TRUE(deque.try_steal(out));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(*out, 42);
}

TEST(ChaseLevDequeTests, BlockingPushMakesProgressAfterSteal)
{
    ChaseLevDeque<int> deque(2);
    deque.push(11);
    deque.push(22);

    std::atomic<bool> producer_entered{false};
    std::atomic<bool> producer_done{false};
    std::thread producer([&]() {
        producer_entered.store(true, std::memory_order_release);
        deque.push(33);
        producer_done.store(true, std::memory_order_release);
    });

    while (!producer_entered.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    for (int i = 0; i < 1024 && !producer_done.load(std::memory_order_acquire); ++i) {
        std::this_thread::yield();
    }
    EXPECT_FALSE(producer_done.load(std::memory_order_acquire));

    std::vector<int> stolen;
    stolen.reserve(3);
    int value = 0;

    deque.steal(value);
    stolen.push_back(value);

    producer.join();
    EXPECT_TRUE(producer_done.load(std::memory_order_acquire));

    deque.steal(value);
    stolen.push_back(value);
    deque.steal(value);
    stolen.push_back(value);

    ASSERT_EQ(stolen.size(), 3u);
    EXPECT_TRUE(deque.is_empty());
}

TEST(ChaseLevDequeTests, OwnerPopAndStealerRaceOnSingleElement)
{
    constexpr int kRounds = 20000;

    for (int i = 0; i < kRounds; ++i) {
        ChaseLevDeque<int> deque(2);
        ASSERT_TRUE(deque.try_push(i));

        std::atomic<int> success_count{0};
        std::atomic<int> popped_value{-1};
        std::atomic<int> stolen_value{-1};

        std::thread owner([&]() {
            int value = -1;
            if (deque.try_pop(value)) {
                popped_value.store(value, std::memory_order_relaxed);
                success_count.fetch_add(1, std::memory_order_relaxed);
            }
        });

        std::thread stealer([&]() {
            int value = -1;
            if (deque.try_steal(value)) {
                stolen_value.store(value, std::memory_order_relaxed);
                success_count.fetch_add(1, std::memory_order_relaxed);
            }
        });

        owner.join();
        stealer.join();

        EXPECT_EQ(success_count.load(std::memory_order_relaxed), 1);

        const int pop_v   = popped_value.load(std::memory_order_relaxed);
        const int steal_v = stolen_value.load(std::memory_order_relaxed);
        EXPECT_TRUE((pop_v == i && steal_v == -1) || (pop_v == -1 && steal_v == i));
        EXPECT_TRUE(deque.is_empty());
    }
}

TEST(ChaseLevDequeTests, ConcurrentOwnerAndStealersConsumeAllItems)
{
    constexpr int kTotalItems = 50000;
    constexpr int kStealers   = 4;

    ChaseLevDeque<int> deque(65536);

    for (int i = 0; i < kTotalItems; ++i) {
        ASSERT_TRUE(deque.try_push(i));
    }

    std::vector<std::atomic<std::uint8_t>> seen(static_cast<size_t>(kTotalItems));
    for (auto& flag : seen) {
        flag.store(0, std::memory_order_relaxed);
    }

    std::atomic<int> consumed{0};
    std::atomic<bool> valid{true};

    auto consume_value = [&](int value) {
        if (value < 0 || value >= kTotalItems) {
            valid.store(false, std::memory_order_relaxed);
            return;
        }

        auto& slot          = seen[static_cast<size_t>(value)];
        const auto previous = slot.exchange(1, std::memory_order_relaxed);
        if (previous != 0) {
            valid.store(false, std::memory_order_relaxed);
            return;
        }

        consumed.fetch_add(1, std::memory_order_relaxed);
    };

    std::thread owner([&]() {
        int value = 0;
        while (consumed.load(std::memory_order_relaxed) < kTotalItems) {
            if (!deque.try_pop(value)) {
                std::this_thread::yield();
                continue;
            }
            consume_value(value);
        }
    });

    std::vector<std::thread> stealers;
    stealers.reserve(kStealers);
    for (int i = 0; i < kStealers; ++i) {
        stealers.emplace_back([&]() {
            int value = 0;
            while (consumed.load(std::memory_order_relaxed) < kTotalItems) {
                if (!deque.try_steal(value)) {
                    std::this_thread::yield();
                    continue;
                }
                consume_value(value);
            }
        });
    }

    owner.join();
    for (auto& thread : stealers) {
        thread.join();
    }

    EXPECT_EQ(consumed.load(std::memory_order_relaxed), kTotalItems);
    EXPECT_TRUE(valid.load(std::memory_order_relaxed));
    EXPECT_TRUE(deque.is_empty());

    for (const auto& flag : seen) {
        EXPECT_EQ(flag.load(std::memory_order_relaxed), 1);
    }
}

TEST(ChaseLevDequeTests, DestructorReleasesRemainingNonTrivialElements)
{
    TrackedObject::live_count.store(0, std::memory_order_relaxed);

    {
        ChaseLevDeque<TrackedObject> deque(8);
        ASSERT_TRUE(deque.try_emplace(1));
        ASSERT_TRUE(deque.try_emplace(2));
        ASSERT_TRUE(deque.try_emplace(3));
    }

    EXPECT_EQ(TrackedObject::live_count.load(std::memory_order_relaxed), 0);
}

} // namespace

// =============================================================================
// Additional ChaseLev Deque Tests
// =============================================================================

TEST(ChaseLevDequeTests, CapacityOneWithMultipleStealers)
{
    ChaseLevDeque<int> deque(1);
    constexpr int kRounds = 5000;
    std::atomic<int> stolen{0};
    std::atomic<bool> done{false};

    std::vector<std::thread> stealers;
    for (int t = 0; t < 4; ++t) {
        stealers.emplace_back([&]() {
            int val = 0;
            while (!done.load(std::memory_order_acquire)) {
                if (deque.try_steal(val))
                    stolen.fetch_add(1, std::memory_order_relaxed);
                else
                    std::this_thread::yield();
            }
            // Drain anything remaining
            while (deque.try_steal(val))
                stolen.fetch_add(1, std::memory_order_relaxed);
        });
    }

    int ownerPopped = 0;
    for (int i = 0; i < kRounds; ++i) {
        while (!deque.try_push(i))
            std::this_thread::yield();
        int val = 0;
        if (deque.try_pop(val))
            ++ownerPopped;
    }

    done.store(true, std::memory_order_release);
    for (auto& t : stealers)
        t.join();

    EXPECT_EQ(ownerPopped + stolen.load(std::memory_order_relaxed), kRounds);
}

TEST(ChaseLevDequeTests, EmptyStealReturnsFalse)
{
    ChaseLevDeque<int> deque(4);
    int val = 0;

    // Steal on empty returns false
    EXPECT_FALSE(deque.try_steal(val));

    // Push one, steal one, steal again should be false
    ASSERT_TRUE(deque.try_push(42));
    ASSERT_TRUE(deque.try_steal(val));
    EXPECT_EQ(val, 42);
    EXPECT_FALSE(deque.try_steal(val));
}
