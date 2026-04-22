#include "gtest/gtest.h"

#include "Tensor/Tensor.hpp"
#include "Tensor/Cuda/CudaAllocator.hpp"

#include <vector>

using namespace bee;

#if !defined(BEE_TENSOR_WITH_CUDA)

// ── BEE_TENSOR_WITH_CUDA=OFF：验证 stub 路径 ─────────────────────────────────

TEST(CudaStubTests, AllocateReturnsNotAvailable)
{
    auto result = tensor::cuda::allocate(16, 64);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：allocate");
}

TEST(CudaStubTests, MemcpyH2dReturnsNotAvailable)
{
    char dummy_src[16] = {};
    char dummy_dst[16] = {};
    auto result = tensor::cuda::memcpy_h2d(dummy_dst, dummy_src, 16);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：memcpy_h2d");
}

TEST(CudaStubTests, CudaAllocatorDeviceIsCuda)
{
    auto& alloc = CudaAllocator::instance();
    EXPECT_EQ(alloc.device(), Device::CUDA);
}

TEST(CudaStubTests, TensorEmptyWithCudaFails)
{
    auto result = Tensor::empty({4}, DType::F32, Device::CUDA);
    ASSERT_FALSE(result);
}

#else // BEE_TENSOR_WITH_CUDA

// ── BEE_TENSOR_WITH_CUDA=ON：验证真实 CUDA 后端端到端 ──────────────────────────

#include "CUDA/Api.hpp"

namespace
{

bool cuda_available()
{
    return cuda::device_count() > 0;
}

} // namespace

TEST(CudaBackend, CudaAllocatorDeviceIsCuda)
{
    auto& alloc = CudaAllocator::instance();
    EXPECT_EQ(alloc.device(), Device::CUDA);
}

TEST(CudaBackend, EmptyOnCudaAllocatesDevicePointer)
{
    if (!cuda_available()) GTEST_SKIP() << "No CUDA device";
    auto t = Tensor::empty({16, 8}, DType::F32, Device::CUDA);
    ASSERT_TRUE(t.has_value()) << t.error().message.view();
    EXPECT_EQ(t->device(), Device::CUDA);
    EXPECT_EQ(t->numel(), 16 * 8);
    EXPECT_TRUE(t->is_contiguous());
    EXPECT_NE(t->data_ptr(), nullptr);
}

TEST(CudaBackend, ZerosOnCudaIsAllZero)
{
    if (!cuda_available()) GTEST_SKIP() << "No CUDA device";
    constexpr std::size_t N = 64;
    auto t = Tensor::zeros({static_cast<std::int64_t>(N)}, DType::F32, Device::CUDA);
    ASSERT_TRUE(t.has_value()) << t.error().message.view();

    auto cpu_r = t->to(Device::CPU);
    ASSERT_TRUE(cpu_r.has_value()) << cpu_r.error().message.view();
    const auto* ptr = static_cast<const float*>(cpu_r->data_ptr());
    for (std::size_t i = 0; i < N; ++i) EXPECT_FLOAT_EQ(ptr[i], 0.0f);
}

TEST(CudaBackend, ToDeviceRoundTripPreservesValues)
{
    if (!cuda_available()) GTEST_SKIP() << "No CUDA device";

    constexpr std::int64_t N = 128;
    auto cpu = Tensor::arange(0, N, 1, DType::F32, Device::CPU);
    ASSERT_TRUE(cpu.has_value());

    auto on_cuda = cpu->to(Device::CUDA);
    ASSERT_TRUE(on_cuda.has_value()) << on_cuda.error().message.view();
    EXPECT_EQ(on_cuda->device(), Device::CUDA);

    auto back = on_cuda->to(Device::CPU);
    ASSERT_TRUE(back.has_value()) << back.error().message.view();
    EXPECT_EQ(back->device(), Device::CPU);

    const auto* src = static_cast<const float*>(cpu->data_ptr());
    const auto* dst = static_cast<const float*>(back->data_ptr());
    for (std::int64_t i = 0; i < N; ++i) EXPECT_FLOAT_EQ(dst[i], src[i]) << "i=" << i;
}

TEST(CudaBackend, ArangeOnCudaWorksViaCpuBridge)
{
    if (!cuda_available()) GTEST_SKIP() << "No CUDA device";
    auto t = Tensor::arange(0, 32, 1, DType::I32, Device::CUDA);
    ASSERT_TRUE(t.has_value()) << t.error().message.view();
    EXPECT_EQ(t->device(), Device::CUDA);

    auto back = t->to(Device::CPU);
    ASSERT_TRUE(back.has_value());
    const auto* ptr = static_cast<const std::int32_t*>(back->data_ptr());
    for (std::int32_t i = 0; i < 32; ++i) EXPECT_EQ(ptr[i], i);
}

TEST(CudaBackend, CloneOnCudaProducesIndependentStorage)
{
    if (!cuda_available()) GTEST_SKIP() << "No CUDA device";
    auto a = Tensor::arange(0, 16, 1, DType::F32, Device::CUDA);
    ASSERT_TRUE(a.has_value());
    auto b = a->clone();
    ASSERT_TRUE(b.has_value()) << b.error().message.view();
    EXPECT_EQ(b->device(), Device::CUDA);
    EXPECT_NE(b->data_ptr(), a->data_ptr());

    auto a_cpu = a->to(Device::CPU);
    auto b_cpu = b->to(Device::CPU);
    ASSERT_TRUE(a_cpu.has_value());
    ASSERT_TRUE(b_cpu.has_value());
    const auto* pa = static_cast<const float*>(a_cpu->data_ptr());
    const auto* pb = static_cast<const float*>(b_cpu->data_ptr());
    for (int i = 0; i < 16; ++i) EXPECT_FLOAT_EQ(pa[i], pb[i]);
}

TEST(CudaBackend, ToSameDeviceReturnsShallowCopy)
{
    if (!cuda_available()) GTEST_SKIP() << "No CUDA device";
    auto t = Tensor::zeros({4}, DType::F32, Device::CUDA);
    ASSERT_TRUE(t.has_value());
    auto same = t->to(Device::CUDA);
    ASSERT_TRUE(same.has_value());
    EXPECT_EQ(same->data_ptr(), t->data_ptr());
}

#endif // BEE_TENSOR_WITH_CUDA
