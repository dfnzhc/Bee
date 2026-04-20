#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

using namespace bee;

// ── zeros ─────────────────────────────────────────────────────────────────────

TEST(CreationTests, ZerosF32_3x4_ShapeAndValues)
{
    auto result = Tensor::zeros({3, 4}, DType::F32);
    ASSERT_TRUE(result.has_value());

    const Tensor& t = *result;
    EXPECT_EQ(t.ndim(), 2);
    EXPECT_EQ(t.numel(), 12);
    EXPECT_TRUE(t.is_contiguous());
    EXPECT_EQ(t.dtype(), DType::F32);

    // 所有元素均为 0.0f
    const auto* ptr = static_cast<const float*>(t.data_ptr());
    for (int64_t i = 0; i < t.numel(); ++i)
        EXPECT_EQ(ptr[i], 0.0f);
}

TEST(CreationTests, ZerosBool)
{
    auto result = Tensor::zeros({5}, DType::Bool);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const bool*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], false);
}

TEST(CreationTests, ZerosU8)
{
    auto result = Tensor::zeros({4}, DType::U8);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const uint8_t*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], uint8_t{0});
}

TEST(CreationTests, ZerosI64)
{
    auto result = Tensor::zeros({6}, DType::I64);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const int64_t*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], int64_t{0});
}

TEST(CreationTests, ZerosScalar1D)
{
    // 用 1D shape {1} 代替 0-rank 标量
    auto result = Tensor::zeros({1}, DType::F32);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->numel(), 1);

    const auto* ptr = static_cast<const float*>(result->data_ptr());
    EXPECT_EQ(ptr[0], 0.0f);
}

// ── ones ──────────────────────────────────────────────────────────────────────

TEST(CreationTests, OnesF32)
{
    auto result = Tensor::ones({3, 4}, DType::F32);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const float*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], 1.0f);
}

TEST(CreationTests, OnesI32)
{
    auto result = Tensor::ones({5}, DType::I32);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const int32_t*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], int32_t{1});
}

TEST(CreationTests, OnesI64)
{
    auto result = Tensor::ones({8}, DType::I64);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const int64_t*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], int64_t{1});
}

TEST(CreationTests, OnesBoolIsTrueBytes)
{
    auto result = Tensor::ones({4}, DType::Bool);
    ASSERT_TRUE(result.has_value());

    // Bool ones 应使每字节为 0x01（true）
    const auto* ptr = static_cast<const uint8_t*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], uint8_t{0x01});
}

// ── full ──────────────────────────────────────────────────────────────────────

TEST(CreationTests, FullF32_3dot5)
{
    auto result = Tensor::full({2, 3}, DType::F32, 3.5);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const float*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_FLOAT_EQ(ptr[i], 3.5f);
}

TEST(CreationTests, FullI32_Negative)
{
    auto result = Tensor::full({4}, DType::I32, -2.0);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const int32_t*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], int32_t{-2});
}

TEST(CreationTests, FullBool_Zero_IsFalse)
{
    auto result = Tensor::full({3}, DType::Bool, 0.0);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const bool*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], false);
}

TEST(CreationTests, FullBool_NonZero_IsTrue)
{
    auto result = Tensor::full({3}, DType::Bool, 1.5);
    ASSERT_TRUE(result.has_value());

    const auto* ptr = static_cast<const bool*>(result->data_ptr());
    for (int64_t i = 0; i < result->numel(); ++i)
        EXPECT_EQ(ptr[i], true);
}

// ── arange ────────────────────────────────────────────────────────────────────

TEST(CreationTests, Arange_0_To_5_DefaultI64)
{
    auto result = Tensor::arange(0, 5);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->numel(), 5);
    EXPECT_EQ(result->dtype(), DType::I64);

    const auto* ptr = static_cast<const int64_t*>(result->data_ptr());
    for (int64_t i = 0; i < 5; ++i)
        EXPECT_EQ(ptr[i], i);
}

TEST(CreationTests, Arange_2_10_Step3_F32)
{
    auto result = Tensor::arange(2, 10, 3, DType::F32);
    ASSERT_TRUE(result.has_value());

    // {2.0, 5.0, 8.0}
    EXPECT_EQ(result->numel(), 3);
    const auto* ptr = static_cast<const float*>(result->data_ptr());
    EXPECT_FLOAT_EQ(ptr[0], 2.0f);
    EXPECT_FLOAT_EQ(ptr[1], 5.0f);
    EXPECT_FLOAT_EQ(ptr[2], 8.0f);
}

TEST(CreationTests, Arange_10_2_StepNeg2_I32)
{
    auto result = Tensor::arange(10, 2, -2, DType::I32);
    ASSERT_TRUE(result.has_value());

    // {10, 8, 6, 4}
    EXPECT_EQ(result->numel(), 4);
    const auto* ptr = static_cast<const int32_t*>(result->data_ptr());
    EXPECT_EQ(ptr[0], int32_t{10});
    EXPECT_EQ(ptr[1], int32_t{8});
    EXPECT_EQ(ptr[2], int32_t{6});
    EXPECT_EQ(ptr[3], int32_t{4});
}

TEST(CreationTests, Arange_StartEqEnd_EmptyTensor)
{
    auto result = Tensor::arange(0, 0);
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(result->numel(), 0);
    const Shape expected{0};
    EXPECT_EQ(result->shape(), expected);
}

TEST(CreationTests, Arange_StepZero_ReturnsError)
{
    auto result = Tensor::arange(0, 5, 0);
    EXPECT_FALSE(result.has_value());
}

TEST(CreationTests, Arange_StepNegWithPositiveRange_ReturnsError)
{
    auto result = Tensor::arange(0, 5, -1);
    EXPECT_FALSE(result.has_value());
}

TEST(CreationTests, Arange_BoolDtype_ReturnsError)
{
    auto result = Tensor::arange(0, 5, 1, DType::Bool);
    EXPECT_FALSE(result.has_value());
}

// ── CUDA 路径 ─────────────────────────────────────────────────────────────────

TEST(CreationTests, Zeros_CUDA_ReturnsError)
{
    auto result = Tensor::zeros({1}, DType::F32, Device::CUDA);
    EXPECT_FALSE(result.has_value());
}
