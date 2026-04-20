#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

using namespace bee;

// ── 默认构造 ─────────────────────────────────────────────────────────────────

TEST(TensorTests, DefaultConstructorUndefined)
{
    Tensor t;
    EXPECT_FALSE(t.defined());
}

// ── Tensor::empty CPU ────────────────────────────────────────────────────────

TEST(TensorTests, EmptyF32_2x3)
{
    auto result = Tensor::empty({2, 3}, DType::F32);
    ASSERT_TRUE(result.has_value());

    const Tensor& t = *result;
    EXPECT_TRUE(t.defined());
    EXPECT_EQ(t.ndim(), 2);
    EXPECT_EQ(t.numel(), 6);
    EXPECT_EQ(t.dtype(), DType::F32);
    EXPECT_EQ(t.device(), Device::CPU);
    EXPECT_TRUE(t.is_contiguous());

    const Strides expected_strides = {3, 1};
    EXPECT_EQ(t.strides(), expected_strides);

    EXPECT_NE(t.data_ptr(), nullptr);
    EXPECT_EQ(t.storage_offset(), 0);
}

TEST(TensorTests, EmptyScalar)
{
    auto result = Tensor::empty({}, DType::F64);
    ASSERT_TRUE(result.has_value());

    const Tensor& t = *result;
    EXPECT_EQ(t.ndim(), 0);
    EXPECT_EQ(t.numel(), 1);
}

TEST(TensorTests, Empty1D)
{
    auto result = Tensor::empty({5}, DType::I64);
    ASSERT_TRUE(result.has_value());

    const Tensor& t = *result;
    EXPECT_EQ(t.ndim(), 1);
    EXPECT_EQ(t.numel(), 5);

    const Strides expected_strides = {1};
    EXPECT_EQ(t.strides(), expected_strides);
}

// ── CUDA 路径返回错误 ────────────────────────────────────────────────────────

TEST(TensorTests, EmptyCudaReturnsError)
{
    auto result = Tensor::empty({4}, DType::I64, Device::CUDA);
    EXPECT_FALSE(result.has_value());
}

// ── 拷贝共享 impl ─────────────────────────────────────────────────────────────

TEST(TensorTests, CopySharesStorage)
{
    auto result = Tensor::empty({3, 3}, DType::F32);
    ASSERT_TRUE(result.has_value());

    Tensor t1 = *result;
    Tensor t2 = t1; // 浅拷贝，共享 impl

    // 两个 Tensor 共享同一 storage
    EXPECT_EQ(t1.storage().get(), t2.storage().get());
    EXPECT_EQ(t1.impl().get(), t2.impl().get());
}

// ── clone 返回独立 storage ────────────────────────────────────────────────────

TEST(TensorTests, CloneIsIndependent)
{
    auto result = Tensor::empty({2, 4}, DType::F32);
    ASSERT_TRUE(result.has_value());

    auto clone_result = result->clone();
    ASSERT_TRUE(clone_result.has_value());

    // storage 不同
    EXPECT_NE(result->storage().get(), clone_result->storage().get());

    // shape / dtype 相等
    EXPECT_EQ(result->shape(), clone_result->shape());
    EXPECT_EQ(result->dtype(), clone_result->dtype());
    EXPECT_EQ(result->numel(), clone_result->numel());
    EXPECT_TRUE(clone_result->is_contiguous());
}

TEST(TensorTests, CloneUndefinedReturnsError)
{
    Tensor t;
    auto result = t.clone();
    EXPECT_FALSE(result.has_value());
}

// ── 负维度与零维度 ────────────────────────────────────────────────────────────

TEST(TensorTests, EmptyRejectsNegativeDimensions)
{
    // 负维度会导致 size_t 溢出，必须返回错误
    auto result = Tensor::empty({-1, 2}, DType::F32);
    EXPECT_FALSE(result.has_value());
}

TEST(TensorTests, EmptyAllowsZeroDimension)
{
    // 零维度是合法的空张量
    auto result = Tensor::empty({0, 3}, DType::F32);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->numel(), 0);
}
