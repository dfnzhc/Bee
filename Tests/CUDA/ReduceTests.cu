/**
 * @File Tests/CUDA/ReduceTests.cu
 * @Brief B6 warp-shuffle global reduce 正确性测试：
 *        覆盖 {Sum, Min, Max, Prod} × {I32, I64, F32, F64, U8(min/max)} × 多组 n（含非 block 对齐）。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtU8  = 1;
constexpr int kDtI32 = 2;
constexpr int kDtI64 = 3;
constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

constexpr int kRdSum  = 0;
constexpr int kRdMin  = 1;
constexpr int kRdMax  = 2;
constexpr int kRdProd = 3;

bool is_kernel_image_missing(int err)
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

template <typename T>
struct ReduceResult
{
    bool skipped = false;
    bool ok      = false;
    T    value{};
};

template <typename T>
auto run_reduce(int op, int dt, const std::vector<T>& a) -> ReduceResult<T>
{
    T *da = nullptr, *dc = nullptr;
    const std::size_t n = a.size();
    ReduceResult<T> result{};
    EXPECT_EQ(cudaMalloc(&da, n * sizeof(T)), cudaSuccess);
    if (!da)
        return result;
    EXPECT_EQ(cudaMalloc(&dc, sizeof(T)), cudaSuccess);
    if (!dc) {
        (void)cudaFree(da);
        return result;
    }
    EXPECT_EQ(cudaMemcpy(da, a.data(), n * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);

    const int err = bee::cuda::detail::ops_reduce_global(op, dt, da, dc, n);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(da); (void)cudaFree(dc);
        result.skipped = true;
        return result;
    }
    EXPECT_EQ(err, 0) << "ops_reduce_global err=" << err;
    if (err != 0) {
        (void)cudaFree(da);
        (void)cudaFree(dc);
        return result;
    }

    T h{};
    EXPECT_EQ(cudaMemcpy(&h, dc, sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(da); (void)cudaFree(dc);
    result.ok    = true;
    result.value = h;
    return result;
}

} // namespace

TEST(CudaReduceWarp, SumI32)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{31}, std::size_t{32}, std::size_t{256},
                          std::size_t{257}, std::size_t{1 << 12}, std::size_t{1 << 20}}) {
        std::vector<std::int32_t> a(n);
        std::int64_t ref = 0;
        for (std::size_t i = 0; i < n; ++i) { a[i] = static_cast<std::int32_t>((i % 7) - 3); ref += a[i]; }
        const auto got = run_reduce<std::int32_t>(kRdSum, kDtI32, a);
        if (got.skipped)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_TRUE(got.ok);
        ASSERT_EQ(got.value, static_cast<std::int32_t>(ref)) << "n=" << n;
    }
}

TEST(CudaReduceWarp, SumF32)
{
    for (std::size_t n : {std::size_t{1}, std::size_t{256}, std::size_t{1025}, std::size_t{1 << 18}}) {
        std::vector<float> a(n);
        double ref = 0;
        for (std::size_t i = 0; i < n; ++i) { a[i] = 1.0f / static_cast<float>(i + 1); ref += a[i]; }
        const auto got = run_reduce<float>(kRdSum, kDtF32, a);
        if (got.skipped)
            GTEST_SKIP() << "No kernel image for current GPU";
        ASSERT_TRUE(got.ok);
        ASSERT_NEAR(got.value, static_cast<float>(ref), std::max(1e-3, 1e-3 * std::abs(ref))) << "n=" << n;
    }
}

TEST(CudaReduceWarp, MinMaxF64)
{
    const std::size_t n = 10000;
    std::vector<double> a(n);
    for (std::size_t i = 0; i < n; ++i) a[i] = std::sin(static_cast<double>(i));
    const auto ref_min = *std::min_element(a.begin(), a.end());
    const auto ref_max = *std::max_element(a.begin(), a.end());
    const auto got_min = run_reduce<double>(kRdMin, kDtF64, a);
    if (got_min.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";
    ASSERT_TRUE(got_min.ok);
    ASSERT_DOUBLE_EQ(got_min.value, ref_min);

    const auto got_max = run_reduce<double>(kRdMax, kDtF64, a);
    if (got_max.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";
    ASSERT_TRUE(got_max.ok);
    ASSERT_DOUBLE_EQ(got_max.value, ref_max);
}

TEST(CudaReduceWarp, MinMaxU8)
{
    const std::size_t n = 5000;
    std::vector<std::uint8_t> a(n);
    for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<std::uint8_t>((i * 13 + 7) & 0xFF);
    const std::uint8_t ref_min = *std::min_element(a.begin(), a.end());
    const std::uint8_t ref_max = *std::max_element(a.begin(), a.end());
    const auto got_min = run_reduce<std::uint8_t>(kRdMin, kDtU8, a);
    if (got_min.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";
    ASSERT_TRUE(got_min.ok);
    ASSERT_EQ(got_min.value, ref_min);

    const auto got_max = run_reduce<std::uint8_t>(kRdMax, kDtU8, a);
    if (got_max.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";
    ASSERT_TRUE(got_max.ok);
    ASSERT_EQ(got_max.value, ref_max);
}

TEST(CudaReduceWarp, ProdI64)
{
    const std::size_t n = 40;
    std::vector<std::int64_t> a(n, 2);
    a[0] = -1;
    std::int64_t ref = 1;
    for (auto v : a) ref *= v;
    const auto got = run_reduce<std::int64_t>(kRdProd, kDtI64, a);
    if (got.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";
    ASSERT_TRUE(got.ok);
    ASSERT_EQ(got.value, ref);
}
