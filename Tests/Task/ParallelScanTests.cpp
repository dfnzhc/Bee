/**
 * @File ParallelScanTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/19
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <functional>
#include <numeric>
#include <ranges>
#include <vector>

#include "Task/Parallel/Scan.hpp"
#include "Concurrency/Thread/WorkPool.hpp"

using namespace bee;

// =========================================================================
// parallel_inclusive_scan
// =========================================================================

TEST(ParallelScanTests, InclusiveScanBasic)
{
    WorkPool pool(4);
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 1);
    std::vector<int> output(10000);

    auto result_it = parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{});

    EXPECT_EQ(result_it, output.end());

    // 验证与串行结果一致
    std::vector<int> expected(10000);
    std::inclusive_scan(input.begin(), input.end(), expected.begin(), std::plus<>{});
    EXPECT_EQ(output, expected);
}

TEST(ParallelScanTests, InclusiveScanSmall)
{
    WorkPool pool(4);
    std::vector<int> input = {1, 2, 3, 4, 5};
    std::vector<int> output(5);

    (void)parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{});

    EXPECT_EQ(output, (std::vector<int>{1, 3, 6, 10, 15}));
}

TEST(ParallelScanTests, InclusiveScanEmpty)
{
    WorkPool pool(4);
    std::vector<int> input;
    std::vector<int> output;

    auto result_it = parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{});
    EXPECT_EQ(result_it, output.end());
}

TEST(ParallelScanTests, InclusiveScanSingle)
{
    WorkPool pool(4);
    std::vector<int> input = {42};
    std::vector<int> output(1);

    (void)parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{});
    EXPECT_EQ(output[0], 42);
}

TEST(ParallelScanTests, InclusiveScanCancellation)
{
    WorkPool pool(4);
    std::vector<int> input(10000, 1);
    std::vector<int> output(10000);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(
        (void)parallel_inclusive_scan(pool, input.begin(), input.end(), output.begin(), std::plus<>{}, ss.get_token()),
        std::runtime_error
    );
}

TEST(ParallelScanTests, InclusiveScanRanges)
{
    WorkPool pool(4);
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 1);
    std::vector<int> output(10000);

    (void)parallel_inclusive_scan(pool, input, output.begin(), std::plus<>{});

    std::vector<int> expected(10000);
    std::inclusive_scan(input.begin(), input.end(), expected.begin(), std::plus<>{});
    EXPECT_EQ(output, expected);
}

// =========================================================================
// parallel_exclusive_scan
// =========================================================================

TEST(ParallelScanTests, ExclusiveScanBasic)
{
    WorkPool pool(4);
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 1);
    std::vector<int> output(10000);

    auto result_it = parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{});

    EXPECT_EQ(result_it, output.end());

    std::vector<int> expected(10000);
    std::exclusive_scan(input.begin(), input.end(), expected.begin(), 0, std::plus<>{});
    EXPECT_EQ(output, expected);
}

TEST(ParallelScanTests, ExclusiveScanSmall)
{
    WorkPool pool(4);
    std::vector<int> input = {1, 2, 3, 4, 5};
    std::vector<int> output(5);

    (void)parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{});

    EXPECT_EQ(output, (std::vector<int>{0, 1, 3, 6, 10}));
}

TEST(ParallelScanTests, ExclusiveScanEmpty)
{
    WorkPool pool(4);
    std::vector<int> input;
    std::vector<int> output;

    auto result_it = parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{});
    EXPECT_EQ(result_it, output.end());
}

TEST(ParallelScanTests, ExclusiveScanWithInit)
{
    WorkPool pool(4);
    std::vector<int> input = {1, 2, 3};
    std::vector<int> output(3);

    (void)parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 100, std::plus<>{});

    // exclusive_scan with init=100: [100, 101, 103]
    EXPECT_EQ(output, (std::vector<int>{100, 101, 103}));
}

TEST(ParallelScanTests, ExclusiveScanCancellation)
{
    WorkPool pool(4);
    std::vector<int> input(10000, 1);
    std::vector<int> output(10000);
    std::stop_source ss;
    ss.request_stop();

    EXPECT_THROW(
        (void)parallel_exclusive_scan(pool, input.begin(), input.end(), output.begin(), 0, std::plus<>{}, ss.get_token()),
        std::runtime_error
    );
}

TEST(ParallelScanTests, ExclusiveScanRanges)
{
    WorkPool pool(4);
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 1);
    std::vector<int> output(10000);

    (void)parallel_exclusive_scan(pool, input, output.begin(), 0, std::plus<>{});

    std::vector<int> expected(10000);
    std::exclusive_scan(input.begin(), input.end(), expected.begin(), 0, std::plus<>{});
    EXPECT_EQ(output, expected);
}
