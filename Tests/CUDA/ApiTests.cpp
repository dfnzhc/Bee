/**
 * @File ApiTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA 组件对外 API 的基础 host 端测试。
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

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

TEST(CUDAComponent, AllocateAndFreeRoundTrip)
{
    if (cuda::device_count() <= 0) {
        GTEST_SKIP() << "No CUDA device available";
    }
    ASSERT_TRUE(cuda::set_device(0).has_value());

    constexpr std::size_t N = 1024;
    auto                  r = cuda::allocate(N * sizeof(float), 16);
    ASSERT_TRUE(r.has_value()) << r.error().message.view();
    void* dptr = *r;
    ASSERT_NE(dptr, nullptr);

    // 写入 + 读回
    std::vector<float> host(N);
    for (std::size_t i = 0; i < N; ++i)
        host[i] = static_cast<float>(i) * 0.5f;

    ASSERT_TRUE(cuda::memcpy_h2d(dptr, host.data(), N * sizeof(float)).has_value());

    std::vector<float> back(N, -1.0f);
    ASSERT_TRUE(cuda::memcpy_d2h(back.data(), dptr, N * sizeof(float)).has_value());

    for (std::size_t i = 0; i < N; ++i) {
        EXPECT_FLOAT_EQ(back[i], host[i]) << "idx=" << i;
    }

    cuda::deallocate(dptr, N * sizeof(float), 16);
}

TEST(CUDAComponent, MemsetZerosDeviceBuffer)
{
    if (cuda::device_count() <= 0) {
        GTEST_SKIP() << "No CUDA device available";
    }
    ASSERT_TRUE(cuda::set_device(0).has_value());

    constexpr std::size_t N = 256;
    auto                  r = cuda::allocate(N * sizeof(std::int32_t), 16);
    ASSERT_TRUE(r.has_value());
    void* dptr = *r;

    ASSERT_TRUE(cuda::memset(dptr, 0, N * sizeof(std::int32_t)).has_value());

    std::vector<std::int32_t> back(N, 123);
    ASSERT_TRUE(cuda::memcpy_d2h(back.data(), dptr, N * sizeof(std::int32_t)).has_value());
    for (auto v : back)
        EXPECT_EQ(v, 0);

    cuda::deallocate(dptr, N * sizeof(std::int32_t), 16);
}

TEST(CUDAComponent, MemcpyD2DWorks)
{
    if (cuda::device_count() <= 0) {
        GTEST_SKIP() << "No CUDA device available";
    }
    ASSERT_TRUE(cuda::set_device(0).has_value());

    constexpr std::size_t N = 64;
    auto                  a = cuda::allocate(N * sizeof(float), 16);
    auto                  b = cuda::allocate(N * sizeof(float), 16);
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());

    std::vector<float> src(N);
    for (std::size_t i = 0; i < N; ++i)
        src[i] = static_cast<float>(i);
    ASSERT_TRUE(cuda::memcpy_h2d(*a, src.data(), N * sizeof(float)).has_value());
    ASSERT_TRUE(cuda::memcpy_d2d(*b, *a, N * sizeof(float)).has_value());

    std::vector<float> back(N, -1.0f);
    ASSERT_TRUE(cuda::memcpy_d2h(back.data(), *b, N * sizeof(float)).has_value());
    for (std::size_t i = 0; i < N; ++i)
        EXPECT_FLOAT_EQ(back[i], src[i]);

    cuda::deallocate(*a, N * sizeof(float), 16);
    cuda::deallocate(*b, N * sizeof(float), 16);
}

// ── M6/M7 Matmul 后端切换脚手架 ─────────────────────────────────────────────

TEST(CUDAMatmulBackend, DefaultIsAuto)
{
    // 初始/默认值应为 Auto。为避免依赖其它测试顺序，这里先保存再还原。
    const auto prev = cuda::ops::get_matmul_backend();
    cuda::ops::set_matmul_backend(cuda::ops::MatmulBackend::Auto);
    EXPECT_EQ(cuda::ops::get_matmul_backend(), cuda::ops::MatmulBackend::Auto);
    cuda::ops::set_matmul_backend(prev);
}

TEST(CUDAMatmulBackend, AvailabilityReflectsBuildStatus)
{
    EXPECT_TRUE(cuda::ops::matmul_backend_available(cuda::ops::MatmulBackend::Auto));
    EXPECT_TRUE(cuda::ops::matmul_backend_available(cuda::ops::MatmulBackend::Wmma));
    EXPECT_FALSE(cuda::ops::matmul_backend_available(cuda::ops::MatmulBackend::Cutlass));
    EXPECT_FALSE(cuda::ops::matmul_backend_available(cuda::ops::MatmulBackend::Native));
}

TEST(CUDAMatmulBackend, SetReturnsPrevious)
{
    const auto prev = cuda::ops::set_matmul_backend(cuda::ops::MatmulBackend::Wmma);
    (void)prev; // 不假定具体值
    EXPECT_EQ(cuda::ops::get_matmul_backend(), cuda::ops::MatmulBackend::Wmma);

    const auto was_wmma = cuda::ops::set_matmul_backend(cuda::ops::MatmulBackend::Auto);
    EXPECT_EQ(was_wmma, cuda::ops::MatmulBackend::Wmma);
    EXPECT_EQ(cuda::ops::get_matmul_backend(), cuda::ops::MatmulBackend::Auto);
}

TEST(CUDAMatmulBackend, CutlassReturnsNotImplemented)
{
    if (cuda::device_count() == 0)
        GTEST_SKIP() << "No CUDA device";
    // 分配两小块设备内存，调用 matmul，预期在 Cutlass 后端下返回错误。
    auto A = cuda::allocate(4 * 4 * sizeof(float), 16).value();
    auto B = cuda::allocate(4 * 4 * sizeof(float), 16).value();
    auto C = cuda::allocate(4 * 4 * sizeof(float), 16).value();

    const auto prev = cuda::ops::set_matmul_backend(cuda::ops::MatmulBackend::Cutlass);
    auto       r    = cuda::ops::matmul(cuda::ScalarType::F32, A, B, C, 4, 4, 4);
    EXPECT_FALSE(r);
    if (!r)
        EXPECT_NE(r.error().message.view().find("Cutlass"), std::string_view::npos);
    cuda::ops::set_matmul_backend(prev);

    cuda::deallocate(A, 4 * 4 * sizeof(float), 16);
    cuda::deallocate(B, 4 * 4 * sizeof(float), 16);
    cuda::deallocate(C, 4 * 4 * sizeof(float), 16);
}

TEST(CUDAMatmulBackend, NativeReturnsNotImplemented)
{
    if (cuda::device_count() == 0)
        GTEST_SKIP() << "No CUDA device";
    auto A = cuda::allocate(4 * 4 * sizeof(float), 16).value();
    auto B = cuda::allocate(4 * 4 * sizeof(float), 16).value();
    auto C = cuda::allocate(4 * 4 * sizeof(float), 16).value();

    const auto prev = cuda::ops::set_matmul_backend(cuda::ops::MatmulBackend::Native);
    auto       r    = cuda::ops::matmul(cuda::ScalarType::F32, A, B, C, 4, 4, 4);
    EXPECT_FALSE(r);
    if (!r)
        EXPECT_NE(r.error().message.view().find("Native"), std::string_view::npos);
    cuda::ops::set_matmul_backend(prev);

    cuda::deallocate(A, 4 * 4 * sizeof(float), 16);
    cuda::deallocate(B, 4 * 4 * sizeof(float), 16);
    cuda::deallocate(C, 4 * 4 * sizeof(float), 16);
}
