/**
 * @File CUDAKernelTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/7
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include "cuKernels/CUDAKernel.hpp"

using namespace bee;

TEST(CUDAKernelComponentTests, ReturnsComponentName)
{
    EXPECT_EQ(cuda_kernel_name(), "cuKernels");
}

TEST(CUDAKernelComponentTests, ExposesCudaToolkitVersion)
{
    EXPECT_FALSE(cuda_toolkit_version().empty());
}

TEST(CUDAKernelComponentTests, CompiledWithNvcc)
{
    EXPECT_EQ(cuda_kernel_compiled_with_nvcc(), 1);
}
