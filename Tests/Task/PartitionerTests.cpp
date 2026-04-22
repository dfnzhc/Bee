/**
 * @File PartitionerTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <numeric>
#include <vector>
#include <stdexcept>
#include <thread>

#include "Task/Parallel/Partitioner.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// =========================================================================
// partition() 测试
// =========================================================================

TEST(PartitionerTests, EmptyRange)
{
    auto chunks = detail::partition(0, 4);
    EXPECT_TRUE(chunks.empty());
}

TEST(PartitionerTests, ZeroWorkers)
{
    auto chunks = detail::partition(100, 0);
    EXPECT_TRUE(chunks.empty());
}

TEST(PartitionerTests, CoversEntireRange)
{
    // 验证所有块的并集恰好覆盖 [0, total)
    constexpr size_t total   = 1000;
    constexpr size_t workers = 4;

    auto chunks = detail::partition(total, workers);
    EXPECT_FALSE(chunks.empty());

    // 首块从 0 开始
    EXPECT_EQ(chunks.front().begin, 0u);
    // 末块到 total 结束
    EXPECT_EQ(chunks.back().end, total);

    // 相邻块无缝衔接
    for (size_t i = 1; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i].begin, chunks[i - 1].end);
    }

    // 每个块非空
    for (auto& c : chunks) {
        EXPECT_GT(c.end, c.begin);
    }
}

TEST(PartitionerTests, SmallRange)
{
    // total < workers 时，每块一个元素
    auto chunks = detail::partition(3, 8);
    EXPECT_EQ(chunks.size(), 3u);
    EXPECT_EQ(chunks[0].begin, 0u);
    EXPECT_EQ(chunks[0].end, 1u);
    EXPECT_EQ(chunks[2].end, 3u);
}

TEST(PartitionerTests, OversubscriptionFactor)
{
    // 4 倍过量分配：10000 元素 / 4 workers → 最多 16 块
    auto chunks = detail::partition(10000, 4);
    EXPECT_LE(chunks.size(), 4u * 4u);
    EXPECT_GE(chunks.size(), 4u);
}

// =========================================================================
// execute_chunks() 测试
// =========================================================================

TEST(PartitionerTests, ExecuteChunksBasic)
{
    WorkPool         pool(4);
    std::vector<int> data(500, 0);

    detail::execute_chunks(pool, data.size(), [&data](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            data[i] = 1;
        }
    });

    // 所有元素被设置为 1
    EXPECT_TRUE(std::all_of(data.begin(), data.end(), [](int v) { return v == 1; }));
}

TEST(PartitionerTests, ExecuteChunksEmpty)
{
    WorkPool pool(4);
    bool     called = false;

    detail::execute_chunks(pool, 0, [&called](size_t, size_t) { called = true; });

    EXPECT_FALSE(called);
}

TEST(PartitionerTests, ExecuteChunksException)
{
    WorkPool pool(4);

    EXPECT_THROW(
        detail::execute_chunks(
            pool,
            100,
            [](size_t begin, size_t) {
                if (begin == 0) {
                    throw std::runtime_error("test error");
                }
            }
        ),
        std::runtime_error
    );
}

TEST(PartitionerTests, ExecuteChunksCancellation)
{
    WorkPool         pool(4);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(detail::execute_chunks(pool, 1000, [](size_t, size_t) {}, ss.get_token()), std::runtime_error);
}

TEST(PartitionerTests, ExecuteChunksConcurrency)
{
    // 验证块确实在多线程上并行执行
    WorkPool         pool(4);
    std::atomic<int> max_concurrent{0};
    std::atomic<int> current{0};

    detail::execute_chunks(pool, 10000, [&](size_t begin, size_t end) {
        int prev = current.fetch_add(1, std::memory_order_relaxed);
        // 更新观察到的最大并发数
        int expected = max_concurrent.load(std::memory_order_relaxed);
        while (prev + 1 > expected) {
            max_concurrent.compare_exchange_weak(expected, prev + 1, std::memory_order_relaxed);
        }
        // 保持足够长的工作时间以确保重叠
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        volatile int sink = 0;
        for (size_t i = begin; i < end; ++i) {
            sink += static_cast<int>(i);
        }
        current.fetch_sub(1, std::memory_order_relaxed);
    });

    // 至少应该观察到 2 个并发执行
    EXPECT_GE(max_concurrent.load(), 2);
}
