/**
 * @File Tests/CUDA/CastTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/24
 * @Brief CUDA cast kernel 的正确性测试。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtBool = 0;
constexpr int kDtI32  = 2;
constexpr int kDtF32  = 4;
constexpr int kDtF64  = 5;
constexpr int kDtF16  = 7;
constexpr int kDtBF16 = 8;

auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice) || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

template <typename Src, typename Dst>
auto run_cast(int src_dt, int dst_dt, const std::vector<Src>& input) -> std::optional<std::vector<Dst>>
{
    void*      dsrc = nullptr;
    void*      ddst = nullptr;
    const auto n    = input.size();

    EXPECT_EQ(cudaMalloc(&dsrc, n * sizeof(Src)), cudaSuccess);
    if (!dsrc)
        return std::vector<Dst>{};
    EXPECT_EQ(cudaMalloc(&ddst, n * sizeof(Dst)), cudaSuccess);
    if (!ddst) {
        (void)cudaFree(dsrc);
        return std::vector<Dst>{};
    }
    EXPECT_EQ(cudaMemcpy(dsrc, input.data(), n * sizeof(Src), cudaMemcpyHostToDevice), cudaSuccess);

    const int err = bee::cuda::detail::ops_cast(src_dt, dsrc, dst_dt, ddst, n);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::nullopt;
    }
    EXPECT_EQ(err, 0) << "ops_cast err=" << err;
    if (err != 0) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::vector<Dst>{};
    }

    std::vector<Dst> output(n);
    EXPECT_EQ(cudaMemcpy(output.data(), ddst, n * sizeof(Dst), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(dsrc);
    (void)cudaFree(ddst);
    return output;
}

} // namespace

TEST(CudaCast, F32ToBoolNormalizesToZeroOne)
{
    const std::vector<float> input  = {0.0f, -3.5f, 2.0f, 0.25f, -0.0f};
    auto                     output = run_cast<float, std::uint8_t>(kDtF32, kDtBool, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    const std::vector<std::uint8_t> expected = {0u, 1u, 1u, 1u, 0u};
    ASSERT_EQ(output->size(), expected.size());
    EXPECT_EQ(*output, expected);
}

TEST(CudaCast, I32ToF64PreservesNumericValue)
{
    const std::vector<std::int32_t> input  = {-7, -1, 0, 9, 42};
    auto                            output = run_cast<std::int32_t, double>(kDtI32, kDtF64, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    ASSERT_EQ(output->size(), input.size());
    for (std::size_t i = 0; i < input.size(); ++i)
        EXPECT_DOUBLE_EQ((*output)[i], static_cast<double>(input[i])) << "i=" << i;
}

TEST(CudaCast, F32ToF16ToF32RoundTrip)
{
    const std::vector<float> input = {0.0f, 1.5f, -2.0f, 3.25f};

    auto f16 = run_cast<float, std::uint16_t>(kDtF32, kDtF16, input);
    if (!f16)
        GTEST_SKIP() << "No kernel image for current GPU";

    auto output = run_cast<std::uint16_t, float>(kDtF16, kDtF32, *f16);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    ASSERT_EQ(output->size(), input.size());
    for (std::size_t i = 0; i < input.size(); ++i)
        EXPECT_NEAR((*output)[i], input[i], 1e-3f) << "i=" << i;
}

TEST(CudaCast, F32ToBF16ToF32RoundTrip)
{
    const std::vector<float> input = {0.0f, 1.5f, -2.0f, 3.25f};

    auto bf16 = run_cast<float, std::uint16_t>(kDtF32, kDtBF16, input);
    if (!bf16)
        GTEST_SKIP() << "No kernel image for current GPU";

    auto output = run_cast<std::uint16_t, float>(kDtBF16, kDtF32, *bf16);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    ASSERT_EQ(output->size(), input.size());
    for (std::size_t i = 0; i < input.size(); ++i)
        EXPECT_NEAR((*output)[i], input[i], 1e-2f) << "i=" << i;
}
