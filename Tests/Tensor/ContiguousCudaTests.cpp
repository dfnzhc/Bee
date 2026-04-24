#include "Tensor/Cuda/Backend.hpp"
#include "Tensor/Tensor.hpp"

#include <gtest/gtest.h>

// ── 2D 转置：验证计划示例中的基本路径 ───────────────────────────────────────

TEST(ContiguousCudaTests, TransposedCudaTensorMaterializesOnDevice)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    auto a = bee::Tensor::arange(0, 12, 1, bee::DType::F32, bee::Device::CUDA).value();
    auto b = a.reshape({3, 4}).value().transpose(0, 1).value();
    auto c = b.contiguous().value();

    EXPECT_EQ(c.device(), bee::Device::CUDA);
    EXPECT_TRUE(c.is_contiguous());
    EXPECT_EQ(c.shape(), (bee::Shape{4, 3}));
}

// ── 3D 置换：通用路径（当前代码 CPU 回退有无限递归，需要 strided_copy 修复）────

TEST(ContiguousCudaTests, Permuted3DCudaTensorMaterializesOnDevice)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    // arange(0,24) → reshape(2,3,4) → permute(2,0,1) → shape={4,2,3}, 非连续
    auto a = bee::Tensor::arange(0, 24, 1, bee::DType::F32, bee::Device::CUDA).value();
    auto b = a.reshape({2, 3, 4}).value().permute({2, 0, 1}).value();

    EXPECT_FALSE(b.is_contiguous());
    EXPECT_EQ(b.shape(), (bee::Shape{4, 2, 3}));

    auto c = b.contiguous().value();

    EXPECT_EQ(c.device(), bee::Device::CUDA);
    EXPECT_TRUE(c.is_contiguous());
    EXPECT_EQ(c.shape(), (bee::Shape{4, 2, 3}));
}

// ── 数据正确性：CUDA 物化结果与 CPU 物化结果应逐元素相同 ───────────────────────

TEST(ContiguousCudaTests, ContiguousDataMatchesCpuReference)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    // CPU 参考：arange(0,12) → reshape(3,4) → transpose → contiguous
    auto cpu_a = bee::Tensor::arange(0, 12, 1, bee::DType::F32, bee::Device::CPU).value();
    auto cpu_b = cpu_a.reshape({3, 4}).value().transpose(0, 1).value();
    auto cpu_c = cpu_b.contiguous().value();

    // CUDA 物化
    auto cuda_a = bee::Tensor::arange(0, 12, 1, bee::DType::F32, bee::Device::CUDA).value();
    auto cuda_b = cuda_a.reshape({3, 4}).value().transpose(0, 1).value();
    auto cuda_c = cuda_b.contiguous().value();

    // 搬回 CPU 对比
    auto result = cuda_c.to(bee::Device::CPU).value();

    ASSERT_EQ(result.shape(), cpu_c.shape());
    const auto* expected = static_cast<const float*>(cpu_c.data_ptr());
    const auto* actual   = static_cast<const float*>(result.data_ptr());
    for (int64_t i = 0; i < result.numel(); ++i) {
        EXPECT_FLOAT_EQ(actual[i], expected[i]) << "index=" << i << " 元素不匹配";
    }
}

// ── 3D 数据正确性 ────────────────────────────────────────────────────────────

TEST(ContiguousCudaTests, Permuted3DDataMatchesCpuReference)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    auto cpu_a = bee::Tensor::arange(0, 24, 1, bee::DType::F32, bee::Device::CPU).value();
    auto cpu_b = cpu_a.reshape({2, 3, 4}).value().permute({2, 0, 1}).value();
    auto cpu_c = cpu_b.contiguous().value();

    auto cuda_a = bee::Tensor::arange(0, 24, 1, bee::DType::F32, bee::Device::CUDA).value();
    auto cuda_b = cuda_a.reshape({2, 3, 4}).value().permute({2, 0, 1}).value();
    auto cuda_c = cuda_b.contiguous().value();

    auto result = cuda_c.to(bee::Device::CPU).value();

    ASSERT_EQ(result.shape(), cpu_c.shape());
    const auto* expected = static_cast<const float*>(cpu_c.data_ptr());
    const auto* actual   = static_cast<const float*>(result.data_ptr());
    for (int64_t i = 0; i < result.numel(); ++i) {
        EXPECT_FLOAT_EQ(actual[i], expected[i]) << "index=" << i << " 元素不匹配";
    }
}

// ── 已连续的 CUDA 张量 contiguous() 应原样返回（共享 storage）────────────────

TEST(ContiguousCudaTests, AlreadyContiguousTensorReturnsSelf)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    auto a = bee::Tensor::arange(0, 6, 1, bee::DType::F32, bee::Device::CUDA).value();
    EXPECT_TRUE(a.is_contiguous());

    auto b = a.contiguous().value();
    EXPECT_TRUE(b.is_contiguous());
    EXPECT_EQ(b.device(), bee::Device::CUDA);
    // 共享同一 storage
    EXPECT_EQ(a.storage().get(), b.storage().get());
}
