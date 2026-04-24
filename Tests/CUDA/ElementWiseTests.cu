/**
 * @File Tests/CUDA/ElementWiseTests.cu
 * @Brief B5 向量化 ElementWise 路径的正确性测试：
 *        覆盖 binary/unary × {F32, F64, I32, U8} × 各种 n（含非 VecN 对齐的尾部）。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtU8  = 1;
constexpr int kDtI32 = 2;
constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

constexpr int kBinAdd = 0;
constexpr int kBinMul = 2;
constexpr int kUnNeg  = 0;
constexpr int kUnAbs  = 1;

// 若 GPU 无 sm_120 image（调试卡等），跳过。
bool is_kernel_image_missing(int err)
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

template <typename T>
auto run_binary(int op, std::size_t n, const std::vector<T>& a, const std::vector<T>& b) -> std::optional<std::vector<T>>
{
    T *da = nullptr, *db = nullptr, *dc = nullptr;
    EXPECT_EQ(cudaMalloc(&da, n * sizeof(T)), cudaSuccess);
    if (!da)
        return std::vector<T>{};
    EXPECT_EQ(cudaMalloc(&db, n * sizeof(T)), cudaSuccess);
    if (!db) {
        (void)cudaFree(da);
        return std::vector<T>{};
    }
    EXPECT_EQ(cudaMalloc(&dc, n * sizeof(T)), cudaSuccess);
    if (!dc) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        return std::vector<T>{};
    }
    EXPECT_EQ(cudaMemcpy(da, a.data(), n * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);
    EXPECT_EQ(cudaMemcpy(db, b.data(), n * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);

    int dt = 0;
    if constexpr (std::is_same_v<T, std::uint8_t>)     dt = kDtU8;
    else if constexpr (std::is_same_v<T, std::int32_t>)dt = kDtI32;
    else if constexpr (std::is_same_v<T, float>)       dt = kDtF32;
    else if constexpr (std::is_same_v<T, double>)      dt = kDtF64;

    const int err = bee::cuda::detail::ops_binary(op, dt, da, db, dc, n);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(da); (void)cudaFree(db); (void)cudaFree(dc);
        return std::nullopt;
    }
    EXPECT_EQ(err, 0) << "ops_binary err=" << err;
    if (err != 0) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        (void)cudaFree(dc);
        return std::vector<T>{};
    }
    std::vector<T> out_host(n);
    EXPECT_EQ(cudaMemcpy(out_host.data(), dc, n * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(da); (void)cudaFree(db); (void)cudaFree(dc);
    return out_host;
}

template <typename T>
auto run_unary(int op, std::size_t n, const std::vector<T>& a) -> std::optional<std::vector<T>>
{
    T *da = nullptr, *dc = nullptr;
    EXPECT_EQ(cudaMalloc(&da, n * sizeof(T)), cudaSuccess);
    if (!da)
        return std::vector<T>{};
    EXPECT_EQ(cudaMalloc(&dc, n * sizeof(T)), cudaSuccess);
    if (!dc) {
        (void)cudaFree(da);
        return std::vector<T>{};
    }
    EXPECT_EQ(cudaMemcpy(da, a.data(), n * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);

    int dt = 0;
    if constexpr (std::is_same_v<T, std::int32_t>)dt = kDtI32;
    else if constexpr (std::is_same_v<T, float>)  dt = kDtF32;
    else if constexpr (std::is_same_v<T, double>) dt = kDtF64;

    const int err = bee::cuda::detail::ops_unary(op, dt, da, dc, n);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(da); (void)cudaFree(dc);
        return std::nullopt;
    }
    EXPECT_EQ(err, 0) << "ops_unary err=" << err;
    if (err != 0) {
        (void)cudaFree(da);
        (void)cudaFree(dc);
        return std::vector<T>{};
    }
    std::vector<T> out_host(n);
    EXPECT_EQ(cudaMemcpy(out_host.data(), dc, n * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(da); (void)cudaFree(dc);
    return out_host;
}

} // namespace

// ─── Binary Add：覆盖 vec 主体 + tail ──────────────────────────────────────
TEST(CudaElementWiseVec, BinaryAddF32_Tail)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{3}, std::size_t{4}, std::size_t{5},
                          std::size_t{100}, std::size_t{1024}, std::size_t{1027}, std::size_t{1 << 16}})
    {
        std::vector<float> a(n), b(n);
        for (std::size_t i = 0; i < n; ++i) { a[i] = static_cast<float>(i); b[i] = static_cast<float>(2 * i); }
        auto c = run_binary<float>(kBinAdd, n, a, b);
        if (!c)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_EQ(c->size(), n);
        for (std::size_t i = 0; i < n; ++i)
            ASSERT_FLOAT_EQ((*c)[i], a[i] + b[i]) << "n=" << n << " i=" << i;
    }
}

TEST(CudaElementWiseVec, BinaryMulF64_Tail)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{100}, std::size_t{1025}}) {
        std::vector<double> a(n), b(n);
        for (std::size_t i = 0; i < n; ++i) { a[i] = 1.5 + i; b[i] = 2.0 - 0.1 * i; }
        auto c = run_binary<double>(kBinMul, n, a, b);
        if (!c)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_EQ(c->size(), n);
        for (std::size_t i = 0; i < n; ++i)
            ASSERT_DOUBLE_EQ((*c)[i], a[i] * b[i]) << "n=" << n << " i=" << i;
    }
}

TEST(CudaElementWiseVec, BinaryAddI32_Tail)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{3}, std::size_t{5}, std::size_t{1024}, std::size_t{1030}}) {
        std::vector<std::int32_t> a(n), b(n);
        for (std::size_t i = 0; i < n; ++i) { a[i] = static_cast<std::int32_t>(i); b[i] = static_cast<std::int32_t>(-3 * i); }
        auto c = run_binary<std::int32_t>(kBinAdd, n, a, b);
        if (!c)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_EQ(c->size(), n);
        for (std::size_t i = 0; i < n; ++i)
            ASSERT_EQ((*c)[i], a[i] + b[i]) << "n=" << n << " i=" << i;
    }
}

TEST(CudaElementWiseVec, BinaryAddU8_Tail)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{15}, std::size_t{16}, std::size_t{17}, std::size_t{1024}, std::size_t{1031}}) {
        std::vector<std::uint8_t> a(n), b(n);
        for (std::size_t i = 0; i < n; ++i) { a[i] = static_cast<std::uint8_t>(i & 0x7F); b[i] = static_cast<std::uint8_t>((i * 3) & 0x7F); }
        auto c = run_binary<std::uint8_t>(kBinAdd, n, a, b);
        if (!c)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_EQ(c->size(), n);
        for (std::size_t i = 0; i < n; ++i)
            ASSERT_EQ((*c)[i], static_cast<std::uint8_t>(a[i] + b[i])) << "n=" << n << " i=" << i;
    }
}

// ─── Unary Neg/Abs：vec 路径覆盖 ────────────────────────────────────────
TEST(CudaElementWiseVec, UnaryNegF32_Tail)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{3}, std::size_t{100}, std::size_t{1025}}) {
        std::vector<float> a(n);
        for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<float>(i) - 50.f;
        auto c = run_unary<float>(kUnNeg, n, a);
        if (!c)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_EQ(c->size(), n);
        for (std::size_t i = 0; i < n; ++i)
            ASSERT_FLOAT_EQ((*c)[i], -a[i]) << "n=" << n << " i=" << i;
    }
}

TEST(CudaElementWiseVec, UnaryAbsI32_Tail)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{5}, std::size_t{1024}, std::size_t{1027}}) {
        std::vector<std::int32_t> a(n);
        for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<std::int32_t>(i) - 100;
        auto c = run_unary<std::int32_t>(kUnAbs, n, a);
        if (!c)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_EQ(c->size(), n);
        for (std::size_t i = 0; i < n; ++i) {
            const std::int32_t expected = a[i] < 0 ? -a[i] : a[i];
            ASSERT_EQ((*c)[i], expected) << "n=" << n << " i=" << i;
        }
    }
}
