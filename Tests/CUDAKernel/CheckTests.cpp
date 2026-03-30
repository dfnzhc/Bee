#include <gtest/gtest.h>

#include <stdexcept>

#include "CUDAKernel/Check.cuh"

TEST(CUDAKernelCheckTests, CheckCudaSuccessDoesNotThrow)
{
    EXPECT_NO_THROW(bee::cuda::check_cuda(cudaSuccess, "cudaSuccess", __FILE__, __LINE__));
}

TEST(CUDAKernelCheckTests, CheckCudaFailureThrows)
{
    EXPECT_THROW(bee::cuda::check_cuda(cudaErrorInvalidValue, "fake_expr", __FILE__, __LINE__), std::runtime_error);
}

TEST(CUDAKernelCheckTests, CheckMacroSupportsSuccess)
{
    EXPECT_NO_THROW(GPU_CUDA_CHECK(cudaSuccess));
}
