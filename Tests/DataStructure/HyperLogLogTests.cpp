/**
 * @File HyperLogLogTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/23
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <DataStructure/Randomized/HyperLogLog.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

using namespace bee;

// =============================================================================
// 构造测试
// =============================================================================

TEST(HyperLogLogTests, DefaultPrecision)
{
    HyperLogLog<int> hll;
    EXPECT_EQ(hll.precision(), 14);
    EXPECT_EQ(hll.registerCount(), 1u << 14);
    EXPECT_TRUE(hll.empty());
}

TEST(HyperLogLogTests, CustomPrecision)
{
    HyperLogLog<int> hll(10);
    EXPECT_EQ(hll.precision(), 10);
    EXPECT_EQ(hll.registerCount(), 1024u);
}

TEST(HyperLogLogTests, PrecisionTooLowThrows)
{
    EXPECT_THROW(HyperLogLog<int>(3), std::invalid_argument);
    EXPECT_THROW(HyperLogLog<int>(0), std::invalid_argument);
}

TEST(HyperLogLogTests, PrecisionTooHighThrows)
{
    EXPECT_THROW(HyperLogLog<int>(19), std::invalid_argument);
    EXPECT_THROW(HyperLogLog<int>(255), std::invalid_argument);
}

TEST(HyperLogLogTests, MinMaxPrecision)
{
    EXPECT_NO_THROW(HyperLogLog<int>(4));
    EXPECT_NO_THROW(HyperLogLog<int>(18));
}

// =============================================================================
// 插入与基数估计测试
// =============================================================================

TEST(HyperLogLogTests, EmptyEstimateIsZeroOrNear)
{
    HyperLogLog<int> hll(14);
    EXPECT_NEAR(hll.estimate(), 0.0, 50.0);
}

TEST(HyperLogLogTests, SingleElementCardinality)
{
    HyperLogLog<int> hll(14);
    hll.insert(42);
    EXPECT_FALSE(hll.empty());
    // 单个元素，估计值应接近 1
    double est = hll.estimate();
    EXPECT_GT(est, 0.0);
    EXPECT_LT(est, 10.0);
}

TEST(HyperLogLogTests, DuplicateInsertDoesNotChangeEstimate)
{
    HyperLogLog<int> hll(14);
    hll.insert(42);
    double est1 = hll.estimate();

    // 重复插入相同元素不应改变估计值（幂等性）
    for (int i = 0; i < 1000; ++i) {
        hll.insert(42);
    }
    double est2 = hll.estimate();
    EXPECT_DOUBLE_EQ(est1, est2);
}

TEST(HyperLogLogTests, SmallCardinalityEstimate)
{
    HyperLogLog<int> hll(14);
    int n = 100;
    for (int i = 0; i < n; ++i) {
        hll.insert(i);
    }

    double est   = hll.estimate();
    double error = std::abs(est - n) / n;
    // 对于 p=14，标准误差约 1.04/sqrt(16384) ≈ 0.81%
    // 允许 20% 的容差（小基数修正可能有偏差）
    EXPECT_LT(error, 0.2) << "estimate=" << est << " actual=" << n;
}

TEST(HyperLogLogTests, MediumCardinalityEstimate)
{
    HyperLogLog<int> hll(14);
    int n = 10000;
    for (int i = 0; i < n; ++i) {
        hll.insert(i);
    }

    double est   = hll.estimate();
    double error = std::abs(est - n) / n;
    EXPECT_LT(error, 0.05) << "estimate=" << est << " actual=" << n;
}

TEST(HyperLogLogTests, LargeCardinalityEstimate)
{
    HyperLogLog<int> hll(14);
    int n = 1000000;
    for (int i = 0; i < n; ++i) {
        hll.insert(i);
    }

    double est   = hll.estimate();
    double error = std::abs(est - n) / n;
    // 对于大基数，允许 5% 误差
    EXPECT_LT(error, 0.05) << "estimate=" << est << " actual=" << n;
}

TEST(HyperLogLogTests, StringElements)
{
    HyperLogLog<std::string> hll(12);
    int n = 5000;
    for (int i = 0; i < n; ++i) {
        hll.insert("string_" + std::to_string(i));
    }

    double est   = hll.estimate();
    double error = std::abs(est - n) / n;
    EXPECT_LT(error, 0.1) << "estimate=" << est << " actual=" << n;
}

// =============================================================================
// 标准误差测试
// =============================================================================

TEST(HyperLogLogTests, StandardErrorDecreasesWithPrecision)
{
    HyperLogLog<int> hll4(4);
    HyperLogLog<int> hll8(8);
    HyperLogLog<int> hll14(14);

    EXPECT_GT(hll4.standardError(), hll8.standardError());
    EXPECT_GT(hll8.standardError(), hll14.standardError());
}

TEST(HyperLogLogTests, StandardErrorFormula)
{
    HyperLogLog<int> hll(10);
    double expected = 1.04 / std::sqrt(1024.0);
    EXPECT_NEAR(hll.standardError(), expected, 1e-10);
}

// =============================================================================
// 合并测试
// =============================================================================

TEST(HyperLogLogTests, MergeDisjointSets)
{
    HyperLogLog<int> hll1(14);
    HyperLogLog<int> hll2(14);

    int n1 = 5000, n2 = 5000;
    for (int i = 0; i < n1; ++i) {
        hll1.insert(i);
    }
    for (int i = n1; i < n1 + n2; ++i) {
        hll2.insert(i);
    }

    hll1.merge(hll2);

    double est   = hll1.estimate();
    double error = std::abs(est - (n1 + n2)) / (n1 + n2);
    EXPECT_LT(error, 0.05) << "estimate=" << est << " actual=" << (n1 + n2);
}

TEST(HyperLogLogTests, MergeOverlappingSets)
{
    HyperLogLog<int> hll1(14);
    HyperLogLog<int> hll2(14);

    // 集合有 50% 重叠
    for (int i = 0; i < 5000; ++i) {
        hll1.insert(i);
    }
    for (int i = 2500; i < 7500; ++i) {
        hll2.insert(i);
    }

    hll1.merge(hll2);

    // 并集实际有 7500 个不同元素
    double est   = hll1.estimate();
    double error = std::abs(est - 7500.0) / 7500.0;
    EXPECT_LT(error, 0.05) << "estimate=" << est << " actual=7500";
}

TEST(HyperLogLogTests, MergePrecisionMismatchThrows)
{
    HyperLogLog<int> hll1(10);
    HyperLogLog<int> hll2(12);
    EXPECT_THROW(hll1.merge(hll2), std::invalid_argument);
}

TEST(HyperLogLogTests, MergeWithEmpty)
{
    HyperLogLog<int> hll1(14);
    HyperLogLog<int> hll2(14);

    for (int i = 0; i < 1000; ++i) {
        hll1.insert(i);
    }

    double estBefore = hll1.estimate();
    hll1.merge(hll2);
    double estAfter = hll1.estimate();

    EXPECT_DOUBLE_EQ(estBefore, estAfter);
}

// =============================================================================
// 清空与状态测试
// =============================================================================

TEST(HyperLogLogTests, ClearResetsAll)
{
    HyperLogLog<int> hll(14);
    for (int i = 0; i < 1000; ++i) {
        hll.insert(i);
    }
    EXPECT_FALSE(hll.empty());

    hll.clear();
    EXPECT_TRUE(hll.empty());
}

TEST(HyperLogLogTests, EstimateCardinalityRoundsCorrectly)
{
    HyperLogLog<int> hll(14);
    for (int i = 0; i < 1000; ++i) {
        hll.insert(i);
    }

    std::size_t intEst = hll.cardinality();
    double rawEst      = hll.estimate();
    EXPECT_EQ(intEst, static_cast<std::size_t>(rawEst + 0.5));
}

// =============================================================================
// 边界情况测试
// =============================================================================

TEST(HyperLogLogTests, LowPrecisionEstimate)
{
    HyperLogLog<int> hll(4);
    for (int i = 0; i < 100; ++i) {
        hll.insert(i);
    }
    // p=4 只有 16 个寄存器，误差会很大，只验证能正常运行且结果合理
    double est = hll.estimate();
    EXPECT_GT(est, 10.0);
    EXPECT_LT(est, 500.0);
}

TEST(HyperLogLogTests, InsertNegativeNumbers)
{
    HyperLogLog<int> hll(14);
    for (int i = -500; i < 500; ++i) {
        hll.insert(i);
    }

    double est   = hll.estimate();
    double error = std::abs(est - 1000.0) / 1000.0;
    EXPECT_LT(error, 0.1);
}
