#include "gtest/gtest.h"

#include "Tensor/Tensor.hpp"
#include "Tensor/Cuda/CudaAllocator.hpp"

using namespace bee;

class CudaStubTests : public ::testing::Test
{
protected:
};

// 测试 cuda::allocate 返回 NotImplemented
TEST_F(CudaStubTests, AllocateReturnsNotImplemented)
{
    auto result = tensor::cuda::allocate(16, 64);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：allocate");
}

// 测试 cuda::memcpy_h2d 返回 NotImplemented
TEST_F(CudaStubTests, MemcpyH2dReturnsNotImplemented)
{
    char dummy_src[16];
    char dummy_dst[16];
    auto result = tensor::cuda::memcpy_h2d(dummy_dst, dummy_src, 16);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：memcpy_h2d");
}

// 测试 cuda::memcpy_d2h 返回 NotImplemented
TEST_F(CudaStubTests, MemcpyD2hReturnsNotImplemented)
{
    char dummy_src[16];
    char dummy_dst[16];
    auto result = tensor::cuda::memcpy_d2h(dummy_dst, dummy_src, 16);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：memcpy_d2h");
}

// 测试 cuda::memcpy_d2d 返回 NotImplemented
TEST_F(CudaStubTests, MemcpyD2dReturnsNotImplemented)
{
    char dummy_src[16];
    char dummy_dst[16];
    auto result = tensor::cuda::memcpy_d2d(dummy_dst, dummy_src, 16);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：memcpy_d2d");
}

// 测试 cuda::synchronize 返回 NotImplemented
TEST_F(CudaStubTests, SynchronizeReturnsNotImplemented)
{
    auto result = tensor::cuda::synchronize();
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：synchronize");
}

// 测试 CudaAllocator::instance().device() 返回 Device::CUDA
TEST_F(CudaStubTests, CudaAllocatorDeviceIsCuda)
{
    auto& alloc = CudaAllocator::instance();
    EXPECT_EQ(alloc.device(), Device::CUDA);
}

// 测试 CudaAllocator::instance().allocate 返回 NotImplemented
TEST_F(CudaStubTests, CudaAllocatorAllocateReturnsNotImplemented)
{
    auto& alloc  = CudaAllocator::instance();
    auto  result = alloc.allocate(16, 64);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA 后端不可用：allocate");
}

// 测试 Tensor::empty(..., Device::CUDA) 仍返回 NotImplemented（不变）
TEST_F(CudaStubTests, TensorEmptyWithCudaReturnsNotImplemented)
{
    auto result = Tensor::empty({4}, DType::F32, Device::CUDA);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "CUDA backend not available");
}
