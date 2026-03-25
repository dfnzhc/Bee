/**
 * @File CountMinSketchTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <DataStructure/Randomized/CountMinSketch.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

using namespace bee;

// =============================================================================
// 基本构造测试
// =============================================================================

TEST(CountMinSketchTests, ConstructWithWidthAndDepth)
{
    CountMinSketch<int> cms(100, 5);
    EXPECT_EQ(cms.width(), 100u);
    EXPECT_EQ(cms.depth(), 5u);
    EXPECT_TRUE(cms.empty());
    EXPECT_EQ(cms.totalCount(), 0u);
}

TEST(CountMinSketchTests, ConstructWithZeroWidthThrows)
{
    EXPECT_THROW(CountMinSketch<int>(0, 5), std::invalid_argument);
}

TEST(CountMinSketchTests, ConstructWithZeroDepthThrows)
{
    EXPECT_THROW(CountMinSketch<int>(100, 0), std::invalid_argument);
}

TEST(CountMinSketchTests, ConstructFromErrorParams)
{
    auto cms = CountMinSketch<int>::from_error_params(0.01, 0.001);
    // width = ceil(e / 0.01) = ceil(271.8) = 272
    EXPECT_GE(cms.width(), 272u);
    // depth = ceil(ln(1/0.001)) = ceil(6.9) = 7
    EXPECT_GE(cms.depth(), 7u);
}

TEST(CountMinSketchTests, InvalidEpsilonThrows)
{
    EXPECT_THROW(CountMinSketch<int>::from_error_params(0.0, 0.01), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>::from_error_params(1.0, 0.01), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>::from_error_params(-0.5, 0.01), std::invalid_argument);
}

TEST(CountMinSketchTests, InvalidDeltaThrows)
{
    EXPECT_THROW(CountMinSketch<int>::from_error_params(0.01, 0.0), std::invalid_argument);
    EXPECT_THROW(CountMinSketch<int>::from_error_params(0.01, 1.0), std::invalid_argument);
}

// =============================================================================
// 插入与查询测试
// =============================================================================

TEST(CountMinSketchTests, InsertAndEstimateSingleElement)
{
    CountMinSketch<int> cms(1000, 5);
    cms.insert(42);
    EXPECT_GE(cms.estimate(42), 1u);
    EXPECT_EQ(cms.totalCount(), 1u);
    EXPECT_FALSE(cms.empty());
}

TEST(CountMinSketchTests, MultipleInsertsSameElement)
{
    CountMinSketch<int> cms(1000, 5);
    for (int i = 0; i < 100; ++i) {
        cms.insert(7);
    }
    EXPECT_GE(cms.estimate(7), 100u);
}

TEST(CountMinSketchTests, UpdateWithCount)
{
    CountMinSketch<int> cms(1000, 5);
    cms.update(10, 50);
    EXPECT_GE(cms.estimate(10), 50u);
    EXPECT_EQ(cms.totalCount(), 50u);
}

TEST(CountMinSketchTests, EstimateNeverUnderestimates)
{
    CountMinSketch<std::string> cms(500, 7);
    std::unordered_map<std::string, std::uint64_t> actual;

    // 插入一些已知频率的元素
    for (int i = 0; i < 1000; ++i) {
        std::string key = "key_" + std::to_string(i % 50);
        cms.insert(key);
        actual[key]++;
    }

    // 验证估计值不低于真实值
    for (auto& [key, count] : actual) {
        EXPECT_GE(cms.estimate(key), count) << "key=" << key;
    }
}

TEST(CountMinSketchTests, UnseenElementEstimateIsZeroOrSmall)
{
    CountMinSketch<int> cms(1000, 5);
    for (int i = 0; i < 100; ++i) {
        cms.insert(i);
    }

    // 未插入的元素，估计值应为 0 或者很小的误差
    auto est = cms.estimate(9999);
    // 由于可能存在哈希碰撞，不强制必须为 0，但应该很小
    EXPECT_LE(est, 10u);
}

TEST(CountMinSketchTests, StringKeys)
{
    CountMinSketch<std::string> cms(200, 5);
    cms.insert("hello");
    cms.insert("hello");
    cms.insert("world");

    EXPECT_GE(cms.estimate("hello"), 2u);
    EXPECT_GE(cms.estimate("world"), 1u);
}

// =============================================================================
// 清空测试
// =============================================================================

TEST(CountMinSketchTests, ClearResetsAllCounters)
{
    CountMinSketch<int> cms(100, 3);
    for (int i = 0; i < 100; ++i) {
        cms.insert(i);
    }
    EXPECT_FALSE(cms.empty());

    cms.clear();
    EXPECT_TRUE(cms.empty());
    EXPECT_EQ(cms.totalCount(), 0u);
    EXPECT_EQ(cms.estimate(0), 0u);
}

// =============================================================================
// 合并测试
// =============================================================================

TEST(CountMinSketchTests, MergeTwoSketches)
{
    CountMinSketch<int> cms1(200, 5);
    CountMinSketch<int> cms2(200, 5);

    // 确保种子一致
    cms2.setSeeds(cms1.seeds());

    for (int i = 0; i < 100; ++i) {
        cms1.insert(1);
    }
    for (int i = 0; i < 50; ++i) {
        cms2.insert(1);
    }

    cms1.merge(cms2);
    EXPECT_GE(cms1.estimate(1), 150u);
    EXPECT_EQ(cms1.totalCount(), 150u);
}

TEST(CountMinSketchTests, MergeDimensionMismatchThrows)
{
    CountMinSketch<int> cms1(100, 3);
    CountMinSketch<int> cms2(200, 3);
    EXPECT_THROW(cms1.merge(cms2), std::invalid_argument);
}

TEST(CountMinSketchTests, MergeSeedMismatchThrows)
{
    CountMinSketch<int> cms1(100, 3);
    CountMinSketch<int> cms2(100, 3);
    // 默认种子相同（基于行号），但手动设置不同种子
    std::vector<std::uint64_t> seeds = {1, 2, 3};
    cms2.setSeeds(seeds);
    EXPECT_THROW(cms1.merge(cms2), std::invalid_argument);
}

// =============================================================================
// 种子管理测试
// =============================================================================

TEST(CountMinSketchTests, SetSeedsClearsData)
{
    CountMinSketch<int> cms(100, 3);
    cms.insert(1);
    EXPECT_FALSE(cms.empty());

    cms.setSeeds(cms.seeds());
    EXPECT_TRUE(cms.empty());
}

TEST(CountMinSketchTests, SetSeedsWrongSizeThrows)
{
    CountMinSketch<int> cms(100, 3);
    std::vector<std::uint64_t> seeds = {1, 2}; // 长度为 2，但 depth 为 3
    EXPECT_THROW(cms.setSeeds(seeds), std::invalid_argument);
}

// =============================================================================
// 精度测试
// =============================================================================

TEST(CountMinSketchTests, ErrorBoundIsRespected)
{
    // 使用较大的 sketch 来验证误差界
    double epsilon = 0.001;
    double delta   = 0.01;
    auto cms       = CountMinSketch<int>::from_error_params(epsilon, delta);

    std::unordered_map<int, std::uint64_t> actual;
    int N = 100000;

    for (int i = 0; i < N; ++i) {
        int key = i % 1000;
        cms.insert(key);
        actual[key]++;
    }

    // 验证大部分估计值不超过 actual + epsilon * N
    int violations = 0;
    for (auto& [key, count] : actual) {
        auto est = cms.estimate(key);
        if (est > count + static_cast<std::uint64_t>(epsilon * N) + 1) {
            ++violations;
        }
    }

    // 允许少量超限（概率保证对所有查询同时成立有复杂性）
    EXPECT_LE(violations, static_cast<int>(actual.size() * delta * 2));
}

// =============================================================================
// 溢出与饱和计数测试
// =============================================================================

TEST(CountMinSketchTests, UpdateSaturatesCounterAndTotalCount)
{
    CountMinSketch<int, std::uint8_t> cms(512, 5);

    cms.update(42, static_cast<std::uint8_t>(250));
    cms.update(42, static_cast<std::uint8_t>(20));

    EXPECT_EQ(cms.estimate(42), std::numeric_limits<std::uint8_t>::max());
    EXPECT_EQ(cms.totalCount(), std::numeric_limits<std::uint8_t>::max());
}

TEST(CountMinSketchTests, MergeSaturatesCounterAndTotalCount)
{
    CountMinSketch<int, std::uint8_t> cms1(512, 5);
    CountMinSketch<int, std::uint8_t> cms2(512, 5);
    cms2.setSeeds(cms1.seeds());

    cms1.update(1, static_cast<std::uint8_t>(240));
    cms2.update(1, static_cast<std::uint8_t>(50));

    cms1.merge(cms2);

    EXPECT_EQ(cms1.estimate(1), std::numeric_limits<std::uint8_t>::max());
    EXPECT_EQ(cms1.totalCount(), std::numeric_limits<std::uint8_t>::max());
}
