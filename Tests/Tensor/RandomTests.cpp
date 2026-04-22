#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cmath>
#include <limits>
#include <numeric>

using namespace bee;

// ── rand 固定 seed 可复现 ────────────────────────────────────────────────────

TEST(RandomTests, Rand_ReproducibleWithSeed)
{
    auto r1 = rand({100}, DType::F32, 42);
    auto r2 = rand({100}, DType::F32, 42);
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());

    const auto* p1 = static_cast<const float*>(r1->data_ptr());
    const auto* p2 = static_cast<const float*>(r2->data_ptr());
    for (int i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ(p1[i], p2[i]) << "index=" << i;
}

// ── rand 元素均在 [0, 1) ──────────────────────────────────────────────────────

TEST(RandomTests, Rand_RangeCheck)
{
    auto r = rand({200}, DType::F32, 7);
    ASSERT_TRUE(r.has_value());

    const auto* p = static_cast<const float*>(r->data_ptr());
    for (int i = 0; i < 200; ++i) {
        EXPECT_GE(p[i], 0.0f) << "index=" << i;
        EXPECT_LT(p[i], 1.0f) << "index=" << i;
    }
}

// ── randn 均值约 0，标准差约 1（seed 固定，统计稳定）────────────────────────

TEST(RandomTests, Randn_Statistics)
{
    auto r = randn({1000}, DType::F32, 1);
    ASSERT_TRUE(r.has_value());

    const auto* p = static_cast<const float*>(r->data_ptr());
    const int   n = 1000;

    // 计算均值
    double sum = 0.0;
    for (int i = 0; i < n; ++i)
        sum += static_cast<double>(p[i]);
    double mean_val = sum / n;

    // 计算标准差
    double var = 0.0;
    for (int i = 0; i < n; ++i) {
        double d  = static_cast<double>(p[i]) - mean_val;
        var      += d * d;
    }
    double std_val = std::sqrt(var / n);

    EXPECT_LT(std::abs(mean_val), 0.2) << "均值=" << mean_val;
    EXPECT_GT(std_val, 0.7) << "标准差=" << std_val;
    EXPECT_LT(std_val, 1.3) << "标准差=" << std_val;
}

// ── randint 元素均在 [low, high) ─────────────────────────────────────────────

TEST(RandomTests, Randint_RangeCheck)
{
    auto r = randint(0, 10, {200}, DType::I32, 7);
    ASSERT_TRUE(r.has_value());

    const auto* p = static_cast<const int32_t*>(r->data_ptr());
    for (int i = 0; i < 200; ++i) {
        EXPECT_GE(p[i], 0) << "index=" << i;
        EXPECT_LT(p[i], 10) << "index=" << i;
    }
}

// ── rand 不支持整数 dtype → Err ──────────────────────────────────────────────

TEST(RandomTests, Rand_InvalidDtype)
{
    auto r = rand({10}, DType::I32, 0);
    EXPECT_FALSE(r.has_value());
}

// ── randn 不支持整数 dtype → Err ─────────────────────────────────────────────

TEST(RandomTests, Randn_InvalidDtype)
{
    auto r = randn({10}, DType::I32, 0);
    EXPECT_FALSE(r.has_value());
}

// ── randint low >= high → Err ─────────────────────────────────────────────────

TEST(RandomTests, Randint_EmptyRange)
{
    auto r = randint(5, 5, {3});
    EXPECT_FALSE(r.has_value());
}

// ── randint U8 且 low < 0 → Err ──────────────────────────────────────────────

TEST(RandomTests, Randint_U8_NegativeLow)
{
    auto r = randint(-1, 10, {3}, DType::U8);
    EXPECT_FALSE(r.has_value());
}

// ── randint 不支持非整数 dtype → Err ─────────────────────────────────────────

TEST(RandomTests, Randint_InvalidDtype)
{
    auto r = randint(0, 10, {5}, DType::F32);
    EXPECT_FALSE(r.has_value());
}

TEST(RandomTests, Randint_I32_OutOfRange)
{
    auto r1 = randint(static_cast<int64_t>(std::numeric_limits<int32_t>::min()) - 1, 10, {4}, DType::I32, 1);
    auto r2 = randint(0, static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 2, {4}, DType::I32, 1);
    EXPECT_FALSE(r1.has_value());
    EXPECT_FALSE(r2.has_value());
}

TEST(RandomTests, Randint_U8_HighTooLarge)
{
    auto r = randint(0, 300, {8}, DType::U8, 1);
    EXPECT_FALSE(r.has_value());
}
