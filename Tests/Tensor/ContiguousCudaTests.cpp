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

// ── slice 回归：非零 offset + 非单位 stride ──────────────────────────────────

TEST(ContiguousCudaTests, SlicedCudaTensorContiguousMatchesCpuReference)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    // CPU 参考：arange(0,10) → slice(0, 2, 8, 2) → 期望值 {2,4,6}
    auto cpu_a = bee::Tensor::arange(0, 10, 1, bee::DType::I64, bee::Device::CPU).value();
    auto cpu_b = cpu_a.slice(0, 2, 8, 2).value(); // offset=2, stride=2, shape={3}
    auto cpu_c = cpu_b.contiguous().value();

    // CUDA 路径
    auto cuda_a = bee::Tensor::arange(0, 10, 1, bee::DType::I64, bee::Device::CUDA).value();
    auto cuda_b = cuda_a.slice(0, 2, 8, 2).value();
    EXPECT_FALSE(cuda_b.is_contiguous());

    auto cuda_c = cuda_b.contiguous().value();
    EXPECT_TRUE(cuda_c.is_contiguous());
    EXPECT_EQ(cuda_c.shape(), cpu_c.shape());

    auto result = cuda_c.to(bee::Device::CPU).value();
    const auto* expected = static_cast<const int64_t*>(cpu_c.data_ptr());
    const auto* actual   = static_cast<const int64_t*>(result.data_ptr());
    for (int64_t i = 0; i < result.numel(); ++i) {
        EXPECT_EQ(actual[i], expected[i]) << "index=" << i << " 元素不匹配";
    }
}

// ── clone：CUDA 非连续张量应返回 contiguous 独立副本 ─────────────────────────

TEST(ContiguousCudaTests, CloneNonContiguousCudaTensorReturnsContiguousCopy)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    auto a = bee::Tensor::arange(0, 12, 1, bee::DType::F32, bee::Device::CUDA).value();
    auto b = a.reshape({3, 4}).value().transpose(0, 1).value(); // 非连续
    EXPECT_FALSE(b.is_contiguous());

    // clone() 不应返回错误
    auto c = b.clone();
    ASSERT_TRUE(c.has_value()) << "clone() 失败: " << c.error().format();
    EXPECT_TRUE(c->is_contiguous());
    EXPECT_EQ(c->device(), bee::Device::CUDA);
    EXPECT_EQ(c->shape(), (bee::Shape{4, 3}));

    // 独立 storage
    EXPECT_NE(c->storage().get(), b.storage().get());

    // 数据正确性：与 CPU 参考对比
    auto cpu_a = bee::Tensor::arange(0, 12, 1, bee::DType::F32, bee::Device::CPU).value();
    auto cpu_b = cpu_a.reshape({3, 4}).value().transpose(0, 1).value();
    auto cpu_c = cpu_b.contiguous().value();

    auto result = c->to(bee::Device::CPU).value();
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

// ── slice + offset：验证非连续切片物化时正确读取偏移数据 ───────────────────

TEST(ContiguousCudaTests, SliceWithOffsetMaterializesCorrectValues)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    // CPU 参考：arange(0,12) → reshape(3,4) → slice(0, 1, 3) → shape={2,4}, 数据=[4,5,6,7,8,9,10,11]
    auto cpu = bee::Tensor::arange(0, 12, 1, bee::DType::F32).value().reshape({3, 4}).value();
    auto gpu = cpu.to(bee::Device::CUDA).value();
    auto sliced = gpu.slice(0, 1, 3).value(); // 取第 1-2 行
    auto cont = sliced.contiguous().value();
    auto back = cont.to(bee::Device::CPU).value();

    ASSERT_EQ(back.shape(), (bee::Shape{2, 4}));
    const auto* p = static_cast<const float*>(back.data_ptr());
    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ(p[i], static_cast<float>(i + 4)) << "index=" << i << " 元素不匹配";
}
