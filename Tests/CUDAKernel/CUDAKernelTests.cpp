#include <gtest/gtest.h>

#include "CUDAKernel/CUDAKernel.hpp"

TEST(CUDAKernelComponentTests, ReturnsComponentName)
{
    EXPECT_EQ(bee::cuda::cuda_kernel_name(), "CUDAKernel");
}

TEST(CUDAKernelComponentTests, ExposesCudaToolkitVersion)
{
    EXPECT_FALSE(bee::cuda::cuda_toolkit_version().empty());
}

TEST(CUDAKernelComponentTests, CompiledWithNvcc)
{
    EXPECT_EQ(bee::cuda::cuda_kernel_compiled_with_nvcc(), 1);
}
