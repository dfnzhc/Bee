#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

using namespace bee;

// ─────────────────────────────────────────────────────────────────────────────
// 批次/N-D matmul 形状推导
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, ThreeDimensionalInputsProduceExpectedOutputShape)
{
    auto a = bee::Tensor::ones({2, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 5}));
}

TEST(BatchedMatmulTests, BroadcastedBatchDimensionIsSupported)
{
    auto a = bee::Tensor::ones({1, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 5}));
}

TEST(BatchedMatmulTests, FourDimensionalInputsProduceExpectedShape)
{
    auto a = bee::Tensor::ones({2, 3, 4, 5}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 3, 5, 6}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 4, 6}));
}

TEST(BatchedMatmulTests, BatchBroadcastBothDimensions)
{
    // a: (2, 1, 3, 4), b: (1, 3, 4, 5) → (2, 3, 3, 5)
    auto a = bee::Tensor::ones({2, 1, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({1, 3, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 3, 5}));
}

// ─────────────────────────────────────────────────────────────────────────────
// 数值正确性验证
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, ThreeDMatmulNumericalCorrectness)
{
    // ones(2,3,4) × ones(2,4,5) → 每元素 = 4.0（K=4 个 1×1 累加）
    auto a = bee::Tensor::ones({2, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 5}));
    const auto* ptr = static_cast<const float*>(c.data_ptr());
    for (int i = 0; i < 2 * 3 * 5; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 4.0f) << "i=" << i;
}

TEST(BatchedMatmulTests, BroadcastedBatchNumericalCorrectness)
{
    // a: (1,3,4) × b: (2,4,5) → 每元素 = 4.0
    auto a = bee::Tensor::ones({1, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    const auto* ptr = static_cast<const float*>(c.data_ptr());
    for (int i = 0; i < 2 * 3 * 5; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 4.0f) << "i=" << i;
}

// ─────────────────────────────────────────────────────────────────────────────
// 回归：2D 路径不受影响
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, TwoDimensionalInputsStillWork)
{
    auto a = bee::Tensor::ones({3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{3, 5}));
    const auto* ptr = static_cast<const float*>(c.data_ptr());
    for (int i = 0; i < 3 * 5; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 4.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// 错误路径：1D 输入仍应拒绝
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, OneDimensionalInputIsRejected)
{
    auto a_r = bee::Tensor::ones({4}, bee::DType::F32);
    auto b_r = bee::Tensor::ones({4, 5}, bee::DType::F32);
    ASSERT_TRUE(a_r.has_value());
    ASSERT_TRUE(b_r.has_value());

    // ndim=1 < 2，应返回错误
    auto c = bee::matmul(*a_r, *b_r);
    EXPECT_FALSE(c.has_value());
}
