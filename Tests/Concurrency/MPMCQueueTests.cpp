/**
 * @File MPMCQueueTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

#include "Concurrency/LockFree/MPMCQueue.hpp"

using namespace bee;

struct MpmcAllocatorThrowControl
{
    static inline int throw_on_construct = -1;
    static inline int construct_calls    = 0;
    static inline int destroy_calls      = 0;
    static inline int allocate_calls     = 0;
    static inline int deallocate_calls   = 0;

    static void Reset()
    {
        throw_on_construct = -1;
        construct_calls    = 0;
        destroy_calls      = 0;
        allocate_calls     = 0;
        deallocate_calls   = 0;
    }
};

template <typename T>
class ThrowOnConstructAllocator
{
public:
    using value_type = T;

    ThrowOnConstructAllocator() noexcept = default;

    template <typename U>
    ThrowOnConstructAllocator(const ThrowOnConstructAllocator<U>&) noexcept
    {
    }

    [[nodiscard]] T* allocate(std::size_t count)
    {
        ++MpmcAllocatorThrowControl::allocate_calls;
        return std::allocator<T>{}.allocate(count);
    }

    void deallocate(T* ptr, std::size_t count) noexcept
    {
        ++MpmcAllocatorThrowControl::deallocate_calls;
        std::allocator<T>{}.deallocate(ptr, count);
    }

    template <typename U, typename... Args>
    void construct(U* ptr, Args&&... args)
    {
        const int call_index = MpmcAllocatorThrowControl::construct_calls++;
        if (MpmcAllocatorThrowControl::throw_on_construct >= 0 && call_index == MpmcAllocatorThrowControl::throw_on_construct) {
            throw std::runtime_error("injected allocator construct failure");
        }
        std::construct_at(ptr, std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* ptr) noexcept
    {
        ++MpmcAllocatorThrowControl::destroy_calls;
        std::destroy_at(ptr);
    }

    template <typename U>
    struct rebind
    {
        using other = ThrowOnConstructAllocator<U>;
    };

    template <typename U>
    bool operator==(const ThrowOnConstructAllocator<U>&) const noexcept
    {
        return true;
    }

    template <typename U>
    bool operator!=(const ThrowOnConstructAllocator<U>&) const noexcept
    {
        return false;
    }
};

TEST(MPMCQueueTests, CapacityAtLeastOne)
{
    MPMCQueueBase<int> queue0(0);
    MPMCQueueBase<int> queue1(1);

    EXPECT_EQ(queue0.capacity(), 1u);
    EXPECT_EQ(queue1.capacity(), 1u);
}

TEST(MPMCQueueTests, TryPushTryPopAndSize)
{
    MPMCQueueBase<int> queue(8);
    EXPECT_TRUE(queue.is_empty());
    EXPECT_FALSE(queue.is_full());
    EXPECT_EQ(queue.size_approx(), 0);

    ASSERT_TRUE(queue.try_emplace(42));
    EXPECT_FALSE(queue.is_empty());
    EXPECT_EQ(queue.size_approx(), 1);

    int value = 0;
    ASSERT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue.is_empty());
    EXPECT_EQ(queue.size_approx(), 0);
}

TEST(MPMCQueueTests, FullConditionWithTryPush)
{
    MPMCQueueBase<int> queue(4);

    ASSERT_TRUE(queue.try_push(1));
    ASSERT_TRUE(queue.try_push(2));
    ASSERT_TRUE(queue.try_push(3));
    ASSERT_TRUE(queue.try_push(4));
    EXPECT_TRUE(queue.is_full());
    EXPECT_FALSE(queue.try_push(5));

    int value = 0;
    for (int expected : {1, 2, 3, 4}) {
        ASSERT_TRUE(queue.try_pop(value));
        EXPECT_EQ(value, expected);
    }
    EXPECT_TRUE(queue.is_empty());
}

TEST(MPMCQueueTests, ConstructorRollsBackWhenCellConstructionThrows)
{
    using TestAllocator = ThrowOnConstructAllocator<int>;
    MpmcAllocatorThrowControl::Reset();
    MpmcAllocatorThrowControl::throw_on_construct = 1;

    EXPECT_THROW((MPMCQueueBase<int, TestAllocator>(8)), std::runtime_error);
    EXPECT_EQ(MpmcAllocatorThrowControl::allocate_calls, 1);
    EXPECT_EQ(MpmcAllocatorThrowControl::deallocate_calls, 1);
    EXPECT_EQ(MpmcAllocatorThrowControl::destroy_calls, 1);
}

TEST(MPMCQueueTests, PushIfNotFullAndPopIfNotEmpty)
{
    MPMCQueueBase<int> queue(2);

    EXPECT_TRUE(queue.push_if_not_full(10));
    EXPECT_TRUE(queue.push_if_not_full(20));
    EXPECT_FALSE(queue.push_if_not_full(30));

    int out = 0;
    EXPECT_TRUE(queue.pop_if_not_empty(out));
    EXPECT_EQ(out, 10);
    EXPECT_TRUE(queue.pop_if_not_empty(out));
    EXPECT_EQ(out, 20);
    EXPECT_FALSE(queue.pop_if_not_empty(out));
}

TEST(MPMCQueueTests, SupportsMoveOnlyType)
{
    MPMCQueueBase<std::unique_ptr<int>> queue(2);
    ASSERT_TRUE(queue.try_push(std::make_unique<int>(7)));

    std::unique_ptr<int> output;
    ASSERT_TRUE(queue.try_pop(output));
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(*output, 7);
}

TEST(MPMCQueueTests, BlockingPushAndPopProgress)
{
    MPMCQueueBase<int> queue(2);
    queue.push(11);
    queue.push(22);

    std::atomic<bool> producer_entered{false};
    std::atomic<bool> producer_done{false};

    std::thread producer([&]() {
        producer_entered.store(true, std::memory_order_release);
        queue.push(33);
        producer_done.store(true, std::memory_order_release);
    });

    while (!producer_entered.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    for (int i = 0; i < 1024 && !producer_done.load(std::memory_order_acquire); ++i) {
        std::this_thread::yield();
    }
    EXPECT_FALSE(producer_done.load(std::memory_order_acquire));

    std::vector<int> consumed;
    consumed.reserve(3);

    int value = 0;
    queue.pop(value);
    consumed.push_back(value);

    producer.join();
    EXPECT_TRUE(producer_done.load(std::memory_order_acquire));

    queue.pop(value);
    consumed.push_back(value);
    queue.pop(value);
    consumed.push_back(value);

    ASSERT_EQ(consumed.size(), 3u);
    EXPECT_EQ(consumed[0], 11);
    EXPECT_EQ(consumed[1], 22);
    EXPECT_EQ(consumed[2], 33);
}

TEST(MPMCQueueTests, ConcurrentMPMCCorrectness)
{
    constexpr int kProducers        = 4;
    constexpr int kConsumers        = 4;
    constexpr int kItemsPerProducer = 25'000;
    constexpr int kTotalItems       = kProducers * kItemsPerProducer;

    MPMCQueueBase<int>                     queue(4096);
    std::vector<std::atomic<std::uint8_t>> seen(static_cast<size_t>(kTotalItems));
    for (auto& slot : seen) {
        slot.store(0, std::memory_order_relaxed);
    }

    std::atomic<int>  produced{0};
    std::atomic<int>  consumed{0};
    std::atomic<bool> valid{true};

    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int producer = 0; producer < kProducers; ++producer) {
        producers.emplace_back([&, producer]() {
            const int base = producer * kItemsPerProducer;
            for (int i = 0; i < kItemsPerProducer; ++i) {
                const int value = base + i;
                while (!queue.try_push(value)) {
                    std::this_thread::yield();
                }
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::vector<std::thread> consumers;
    consumers.reserve(kConsumers);
    for (int consumer = 0; consumer < kConsumers; ++consumer) {
        consumers.emplace_back([&]() {
            int value = 0;
            while (consumed.load(std::memory_order_acquire) < kTotalItems) {
                if (!queue.try_pop(value)) {
                    if (produced.load(std::memory_order_acquire) >= kTotalItems) {
                        if (consumed.load(std::memory_order_acquire) >= kTotalItems) {
                            break;
                        }
                    }
                    std::this_thread::yield();
                    continue;
                }

                if (value < 0 || value >= kTotalItems) {
                    valid.store(false, std::memory_order_relaxed);
                    continue;
                }
                auto&      flag     = seen[static_cast<size_t>(value)];
                const auto previous = flag.exchange(1, std::memory_order_relaxed);
                if (previous != 0) {
                    valid.store(false, std::memory_order_relaxed);
                    continue;
                }
                consumed.fetch_add(1, std::memory_order_release);
            }
        });
    }

    for (auto& thread : producers) {
        thread.join();
    }
    for (auto& thread : consumers) {
        thread.join();
    }

    EXPECT_EQ(produced.load(std::memory_order_relaxed), kTotalItems);
    EXPECT_EQ(consumed.load(std::memory_order_relaxed), kTotalItems);
    EXPECT_TRUE(valid.load(std::memory_order_relaxed));
    for (const auto& flag : seen) {
        EXPECT_EQ(flag.load(std::memory_order_relaxed), 1);
    }
}

// =============================================================================
// Additional MPMC Queue Tests
// =============================================================================

TEST(MPMCQueueTests, CapacityOneMultipleProducersConsumers)
{
    MPMCQueueBase<int> queue(4);
    constexpr int      kItemsPerProducer = 1000;
    constexpr int      kProducers        = 4;
    constexpr int      kConsumers        = 4;
    constexpr int      kTotalItems       = kProducers * kItemsPerProducer;

    std::vector<std::atomic<int>> seen(kTotalItems);
    for (auto& a : seen)
        a.store(0, std::memory_order_relaxed);

    std::atomic<int> producersDone{0};

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&queue, &producersDone, p]() {
            for (int i = 0; i < kItemsPerProducer; ++i) {
                int val = p * kItemsPerProducer + i;
                while (!queue.try_push(val))
                    std::this_thread::yield();
            }
            producersDone.fetch_add(1, std::memory_order_release);
        });
    }

    std::atomic<int>         totalConsumed{0};
    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&queue, &seen, &totalConsumed, &producersDone]() {
            int value = 0;
            while (true) {
                if (queue.try_pop(value)) {
                    seen[value].fetch_add(1, std::memory_order_relaxed);
                    totalConsumed.fetch_add(1, std::memory_order_relaxed);
                } else if (producersDone.load(std::memory_order_acquire) == kProducers && queue.is_empty()) {
                    break;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers)
        t.join();
    for (auto& t : consumers)
        t.join();

    EXPECT_EQ(totalConsumed.load(std::memory_order_relaxed), kTotalItems);
    for (int i = 0; i < kTotalItems; ++i)
        EXPECT_EQ(seen[i].load(std::memory_order_relaxed), 1);
}

TEST(MPMCQueueTests, TryPushOnFullReturnsFalse)
{
    MPMCQueueBase<int> queue(2);

    ASSERT_TRUE(queue.try_push(1));
    ASSERT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.is_full());
    EXPECT_FALSE(queue.try_push(3));

    int value = 0;
    ASSERT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);

    EXPECT_TRUE(queue.try_push(3));
    EXPECT_TRUE(queue.is_full());
}

TEST(MPMCQueueTests, AlternatingPushPop)
{
    MPMCQueueBase<int> queue(4);
    constexpr int      kCount = 10000;

    for (int i = 0; i < kCount; ++i) {
        ASSERT_TRUE(queue.try_push(i));
        int value = 0;
        ASSERT_TRUE(queue.try_pop(value));
        EXPECT_EQ(value, i);
    }
}
