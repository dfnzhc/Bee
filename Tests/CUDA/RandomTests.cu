/**
 * @File Tests/CUDA/RandomTests.cu
 * @Brief B7 设备侧 Philox 随机数 kernel 的正确性与统计合理性测试。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtU8  = 1;
constexpr int kDtI32 = 2;
constexpr int kDtI64 = 3;
constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

bool is_kernel_image_missing(int err)
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

template <typename T>
std::vector<T> run_uniform(std::size_t n, std::uint64_t seed)
{
    T* dptr = nullptr;
    [&] { ASSERT_EQ(cudaMalloc(&dptr, n * sizeof(T)), cudaSuccess); }();
    int dt = std::is_same_v<T, float> ? kDtF32 : kDtF64;
    const int err = bee::cuda::detail::ops_random_uniform(dt, dptr, n, seed);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dptr);
        ADD_FAILURE() << "No kernel image for current GPU";
        return {};
    }
    EXPECT_EQ(err, 0) << "random_uniform err=" << err;
    std::vector<T> host(n);
    EXPECT_EQ(cudaMemcpy(host.data(), dptr, n * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(dptr);
    return host;
}

template <typename T>
std::vector<T> run_normal(std::size_t n, std::uint64_t seed)
{
    T* dptr = nullptr;
    [&] { ASSERT_EQ(cudaMalloc(&dptr, n * sizeof(T)), cudaSuccess); }();
    int dt = std::is_same_v<T, float> ? kDtF32 : kDtF64;
    const int err = bee::cuda::detail::ops_random_normal(dt, dptr, n, seed);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dptr);
        ADD_FAILURE() << "No kernel image";
        return {};
    }
    EXPECT_EQ(err, 0);
    std::vector<T> host(n);
    EXPECT_EQ(cudaMemcpy(host.data(), dptr, n * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(dptr);
    return host;
}

template <typename T>
std::vector<T> run_int(std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed)
{
    T* dptr = nullptr;
    [&] { ASSERT_EQ(cudaMalloc(&dptr, n * sizeof(T)), cudaSuccess); }();
    int dt = 0;
    if constexpr (std::is_same_v<T, std::uint8_t>)       dt = kDtU8;
    else if constexpr (std::is_same_v<T, std::int32_t>)  dt = kDtI32;
    else if constexpr (std::is_same_v<T, std::int64_t>)  dt = kDtI64;
    const int err = bee::cuda::detail::ops_random_int(dt, dptr, n, low, high, seed);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dptr);
        ADD_FAILURE() << "No kernel image";
        return {};
    }
    EXPECT_EQ(err, 0);
    std::vector<T> host(n);
    EXPECT_EQ(cudaMemcpy(host.data(), dptr, n * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(dptr);
    return host;
}

} // namespace

TEST(CudaRandom, UniformF32_Range)
{
    const std::size_t n = 1 << 16;
    auto h = run_uniform<float>(n, 0xC0FFEEull);
    if (h.empty()) return;
    for (float v : h) {
        ASSERT_GE(v, 0.0f);
        ASSERT_LT(v, 1.0f);
    }
    // Mean ≈ 0.5
    double mean = 0.0;
    for (float v : h) mean += v;
    mean /= n;
    EXPECT_NEAR(mean, 0.5, 0.02);
}

TEST(CudaRandom, UniformF64_Range)
{
    const std::size_t n = 1 << 15;
    auto h = run_uniform<double>(n, 42);
    if (h.empty()) return;
    for (double v : h) {
        ASSERT_GE(v, 0.0);
        ASSERT_LT(v, 1.0);
    }
    double mean = 0.0;
    for (double v : h) mean += v;
    mean /= n;
    EXPECT_NEAR(mean, 0.5, 0.02);
}

TEST(CudaRandom, UniformF32_Deterministic)
{
    const std::size_t n = 4096;
    auto a = run_uniform<float>(n, 123);
    auto b = run_uniform<float>(n, 123);
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) ASSERT_EQ(a[i], b[i]);
}

TEST(CudaRandom, UniformF32_NonPow2Size)
{
    // 非 4 的倍数：尾部分支
    for (std::size_t n : {1u, 3u, 5u, 1025u, 1027u}) {
        auto h = run_uniform<float>(n, 7);
        if (h.empty()) return;
        ASSERT_EQ(h.size(), n);
        for (float v : h) {
            ASSERT_GE(v, 0.0f);
            ASSERT_LT(v, 1.0f);
        }
    }
}

TEST(CudaRandom, NormalF32_Moments)
{
    const std::size_t n = 1 << 18;
    auto h = run_normal<float>(n, 0xBEEFull);
    if (h.empty()) return;
    double mean = 0.0, m2 = 0.0;
    for (float v : h) mean += v;
    mean /= n;
    for (float v : h) {
        const double d = v - mean;
        m2 += d * d;
    }
    const double stddev = std::sqrt(m2 / n);
    EXPECT_NEAR(mean, 0.0, 0.02);
    EXPECT_NEAR(stddev, 1.0, 0.05);
}

TEST(CudaRandom, NormalF64_Moments)
{
    const std::size_t n = 1 << 17;
    auto h = run_normal<double>(n, 0xD00Dull);
    if (h.empty()) return;
    double mean = 0.0, m2 = 0.0;
    for (double v : h) mean += v;
    mean /= n;
    for (double v : h) {
        const double d = v - mean;
        m2 += d * d;
    }
    const double stddev = std::sqrt(m2 / n);
    EXPECT_NEAR(mean, 0.0, 0.03);
    EXPECT_NEAR(stddev, 1.0, 0.05);
}

TEST(CudaRandom, IntI32_Range)
{
    const std::size_t n    = 1 << 14;
    const std::int64_t low = -100, high = 100;
    auto h = run_int<std::int32_t>(n, low, high, 0xABCDull);
    if (h.empty()) return;
    for (auto v : h) {
        ASSERT_GE(v, low);
        ASSERT_LT(v, high);
    }
}

TEST(CudaRandom, IntU8_Range)
{
    const std::size_t n = 1 << 14;
    auto h = run_int<std::uint8_t>(n, 10, 200, 1);
    if (h.empty()) return;
    for (auto v : h) {
        ASSERT_GE(static_cast<int>(v), 10);
        ASSERT_LT(static_cast<int>(v), 200);
    }
}

TEST(CudaRandom, IntI64_Range)
{
    const std::size_t n = 8192;
    auto h = run_int<std::int64_t>(n, -10000, 10000, 999);
    if (h.empty()) return;
    for (auto v : h) {
        ASSERT_GE(v, -10000);
        ASSERT_LT(v, 10000);
    }
}
