/**
 * @File ApiTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA 组件对外 API 的基础 host 端测试。
 */

#include <gtest/gtest.h>

#include "CUDA/Api.hpp"

using namespace bee;

TEST(CUDAComponent, ReturnsComponentName)
{
    EXPECT_EQ(cuda::component_name(), "CUDA");
}

TEST(CUDAComponent, ExposesToolkitVersion)
{
    EXPECT_FALSE(cuda::toolkit_version().empty());
}

TEST(CUDAComponent, CompiledWithNvcc)
{
    EXPECT_TRUE(cuda::compiled_with_nvcc());
}

TEST(CUDAComponent, DeviceCountNonNegative)
{
    // 设备数至少 >= 0；若主机无 CUDA 设备应返回 0 而非抛错
    EXPECT_GE(cuda::device_count(), 0);
}

TEST(CUDAComponent, DeviceSynchronizeSucceedsWhenDevicePresent)
{
    if (cuda::device_count() <= 0) {
        GTEST_SKIP() << "No CUDA device available";
    }
    ASSERT_TRUE(cuda::set_device(0).has_value());
    ASSERT_TRUE(cuda::device_synchronize().has_value());
}

TEST(CUDAComponent, MemoryApisAreStubUntilM2)
{
    // M0 阶段：allocate / memcpy_* 仍为占位实现，返回 Recoverable 错误
    auto r = cuda::allocate(16, 16);
    EXPECT_FALSE(r.has_value());
}
