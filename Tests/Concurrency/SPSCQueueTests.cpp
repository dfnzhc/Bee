/**
 * @File SPSCQueueTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/25
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

#include "Concurrency/LockFree/SPSCQueue.hpp"

using namespace bee;

struct CountingObject
{
    static inline std::atomic<int> alive_count{0};

    int value = 0;

    explicit CountingObject(int v = 0)
        : value(v)
    {
        alive_count.fetch_add(1, std::memory_order_relaxed);
    }

    CountingObject(const CountingObject& other)
        : value(other.value)
    {
        alive_count.fetch_add(1, std::memory_order_relaxed);
    }

    CountingObject(CountingObject&& other) noexcept
        : value(other.value)
    {
        alive_count.fetch_add(1, std::memory_order_relaxed);
        other.value = -1;
    }

    CountingObject& operator=(const CountingObject& other)     = default;
    CountingObject& operator=(CountingObject&& other) noexcept = default;

    ~CountingObject()
    {
        alive_count.fetch_sub(1, std::memory_order_relaxed);
    }
};

TEST(SPSCQueueDynamicTests, CapacityAtLeastOne)
{
    SPSCQueue<int> queue0(0);
    SPSCQueue<int> queue1(1);

    EXPECT_EQ(queue0.capacity(), 1u);
    EXPECT_EQ(queue1.capacity(), 1u);
}

TEST(SPSCQueueDynamicTests, DefaultCapacityPolicyRoundsUpToPowerOfTwo)
{
    SPSCQueue<int> queue3(3);
    SPSCQueue<int> queue5(5);
    SPSCQueue<int> queue8(8);

    EXPECT_EQ(queue3.capacity(), 3u);
    EXPECT_EQ(queue5.capacity(), 7u);
    EXPECT_EQ(queue8.capacity(), 15u);
}

TEST(SPSCQueueDynamicTests, ExactCapacityPolicyPreservesRequestedCapacity)
{
    SPSCQueueExact<int> queue3(3);
    SPSCQueueExact<int> queue5(5);

    EXPECT_EQ(queue3.capacity(), 3u);
    EXPECT_EQ(queue5.capacity(), 5u);
}

TEST(SPSCQueueDynamicTests, TryPushPopAndStateTransitions)
{
    SPSCQueue<int> queue(4);

    EXPECT_TRUE(queue.is_empty());
    EXPECT_FALSE(queue.is_full());
    EXPECT_EQ(queue.size_approx(), 0u);

    ASSERT_TRUE(queue.try_push(7));
    ASSERT_FALSE(queue.is_empty());
    ASSERT_EQ(queue.size_approx(), 1u);

    int out = 0;
    ASSERT_TRUE(queue.try_pop(out));
    EXPECT_EQ(out, 7);
    EXPECT_TRUE(queue.is_empty());
    EXPECT_EQ(queue.size_approx(), 0u);
}

TEST(SPSCQueueDynamicTests, FullConditionAndWrapAround)
{
    SPSCQueueExact<int> queue(4);

    ASSERT_TRUE(queue.try_push(1));
    ASSERT_TRUE(queue.try_push(2));
    ASSERT_TRUE(queue.try_push(3));
    ASSERT_TRUE(queue.try_push(4));

    EXPECT_TRUE(queue.is_full());
    EXPECT_FALSE(queue.try_push(5));

    int out = 0;
    ASSERT_TRUE(queue.try_pop(out));
    EXPECT_EQ(out, 1);
    EXPECT_FALSE(queue.is_full());

    ASSERT_TRUE(queue.try_push(5));
    EXPECT_TRUE(queue.is_full());

    for (int expected : {2, 3, 4, 5}) {
        ASSERT_TRUE(queue.try_pop(out));
        EXPECT_EQ(out, expected);
    }

    EXPECT_TRUE(queue.is_empty());
    EXPECT_FALSE(queue.is_full());
}

TEST(SPSCQueueDynamicTests, FrontAndPopTwoPhaseConsumption)
{
    SPSCQueue<int> queue(3);

    ASSERT_TRUE(queue.try_push(11));
    ASSERT_TRUE(queue.try_push(22));

    int* first = queue.front();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(*first, 11);
    queue.pop();

    int* second = queue.front();
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(*second, 22);
    queue.pop();

    EXPECT_EQ(queue.front(), nullptr);
    EXPECT_TRUE(queue.is_empty());
}

TEST(SPSCQueueDynamicTests, SupportsMoveOnlyType)
{
    SPSCQueue<std::unique_ptr<int>> queue(2);

    ASSERT_TRUE(queue.try_push(std::make_unique<int>(42)));

    std::unique_ptr<int> out;
    ASSERT_TRUE(queue.try_pop(out));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(*out, 42);
}

TEST(SPSCQueueDynamicTests, DestructorReleasesRemainingElements)
{
    CountingObject::alive_count.store(0, std::memory_order_relaxed);

    {
        SPSCQueue<CountingObject> queue(8);
        for (int i = 0; i < 5; ++i) {
            ASSERT_TRUE(queue.try_emplace(i));
        }
        EXPECT_EQ(CountingObject::alive_count.load(std::memory_order_relaxed), 5);
    }

    EXPECT_EQ(CountingObject::alive_count.load(std::memory_order_relaxed), 0);
}

TEST(SPSCQueueDynamicTests, ConcurrentProducerConsumerPreservesOrder)
{
    constexpr std::uint32_t kCount = 300'000;
    SPSCQueue<std::uint32_t> queue(4096);

    std::vector<std::uint32_t> consumed;
    consumed.reserve(kCount);

    std::atomic<bool> producer_done{false};

    std::thread producer([&queue, &producer_done]() {
        for (std::uint32_t i = 0; i < kCount; ++i) {
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&queue, &producer_done, &consumed]() {
        std::uint32_t value = 0;
        while (!producer_done.load(std::memory_order_acquire) || consumed.size() < kCount) {
            if (queue.try_pop(value)) {
                consumed.push_back(value);
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(consumed.size(), static_cast<std::size_t>(kCount));
    for (std::uint32_t i = 0; i < kCount; ++i) {
        EXPECT_EQ(consumed[i], i);
    }
}
