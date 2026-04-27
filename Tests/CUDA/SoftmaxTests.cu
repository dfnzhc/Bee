/**
 * @File Tests/CUDA/SoftmaxTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/27
 * @Brief CUDA 原生 softmax kernel 的正确性测试。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cmath>
#include <optional>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;
constexpr int kDtI32 = 2;

auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice) || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

template <typename T>
auto softmax_ref(std::size_t outer, std::size_t axis, std::size_t inner, const std::vector<T>& input) -> std::vector<T>
{
    std::vector<T> output(input.size());
    for (std::size_t o = 0; o < outer; ++o) {
        for (std::size_t i = 0; i < inner; ++i) {
            const std::size_t base = o * axis * inner + i;
            T                 maxv = input[base];
            for (std::size_t a = 1; a < axis; ++a)
                maxv = input[base + a * inner] > maxv ? input[base + a * inner] : maxv;

            long double sum = 0.0L;
            for (std::size_t a = 0; a < axis; ++a) {
                const T v                 = static_cast<T>(std::exp(static_cast<long double>(input[base + a * inner] - maxv)));
                output[base + a * inner] = v;
                sum                      += static_cast<long double>(v);
            }

            for (std::size_t a = 0; a < axis; ++a)
                output[base + a * inner] = static_cast<T>(static_cast<long double>(output[base + a * inner]) / sum);
        }
    }
    return output;
}

template <typename T>
auto dtype_code() -> int;

template <>
auto dtype_code<float>() -> int
{
    return kDtF32;
}

template <>
auto dtype_code<double>() -> int
{
    return kDtF64;
}

template <typename T>
auto run_softmax(std::size_t outer, std::size_t axis, std::size_t inner, const std::vector<T>& input) -> std::optional<std::vector<T>>
{
    T*         dsrc  = nullptr;
    T*         ddst  = nullptr;
    const auto elems = outer * axis * inner;

    EXPECT_EQ(cudaMalloc(&dsrc, elems * sizeof(T)), cudaSuccess);
    if (!dsrc)
        return std::vector<T>{};
    EXPECT_EQ(cudaMalloc(&ddst, elems * sizeof(T)), cudaSuccess);
    if (!ddst) {
        (void)cudaFree(dsrc);
        return std::vector<T>{};
    }
    EXPECT_EQ(cudaMemcpy(dsrc, input.data(), elems * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);

    const int err = bee::cuda::detail::ops_softmax(dtype_code<T>(), dsrc, ddst, outer, axis, inner);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::nullopt;
    }
    EXPECT_EQ(err, 0) << "ops_softmax err=" << err;
    if (err != 0) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::vector<T>{};
    }

    std::vector<T> output(elems);
    EXPECT_EQ(cudaMemcpy(output.data(), ddst, elems * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(dsrc);
    (void)cudaFree(ddst);
    return output;
}

} // 匿名命名空间

TEST(CudaSoftmax, ComputesRowsF32)
{
    constexpr std::size_t outer = 2;
    constexpr std::size_t axis  = 3;
    constexpr std::size_t inner = 1;
    const std::vector<float> input{0.0f, 1.0f, 2.0f, 0.0f, 1.0f, 2.0f};

    auto output = run_softmax(outer, axis, inner, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    const auto expected = softmax_ref(outer, axis, inner, input);
    ASSERT_EQ(output->size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_NEAR((*output)[i], expected[i], 1e-6f) << "i=" << i;
}

TEST(CudaSoftmax, ComputesNonLastDimF32)
{
    constexpr std::size_t outer = 2;
    constexpr std::size_t axis  = 3;
    constexpr std::size_t inner = 2;
    const std::vector<float> input{0.0f, 10.0f, 1.0f, 11.0f, 2.0f, 12.0f, 3.0f, 13.0f, 4.0f, 14.0f, 5.0f, 15.0f};

    auto output = run_softmax(outer, axis, inner, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    const auto expected = softmax_ref(outer, axis, inner, input);
    ASSERT_EQ(output->size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_NEAR((*output)[i], expected[i], 1e-6f) << "i=" << i;

    for (std::size_t o = 0; o < outer; ++o) {
        for (std::size_t i = 0; i < inner; ++i) {
            float sum = 0.0f;
            for (std::size_t a = 0; a < axis; ++a)
                sum += (*output)[o * axis * inner + a * inner + i];
            EXPECT_NEAR(sum, 1.0f, 1e-6f) << "outer=" << o << " inner=" << i;
        }
    }
}

TEST(CudaSoftmax, ComputesNonLastDimF64)
{
    constexpr std::size_t outer = 2;
    constexpr std::size_t axis  = 3;
    constexpr std::size_t inner = 2;
    const std::vector<double> input{0.0, 10.0, 1.0, 11.0, 2.0, 12.0, 3.0, 13.0, 4.0, 14.0, 5.0, 15.0};

    auto output = run_softmax(outer, axis, inner, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    const auto expected = softmax_ref(outer, axis, inner, input);
    ASSERT_EQ(output->size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_NEAR((*output)[i], expected[i], 1e-12) << "i=" << i;
}

TEST(CudaSoftmax, RejectsInvalidArguments)
{
    float* dsrc = nullptr;
    float* ddst = nullptr;
    ASSERT_EQ(cudaMalloc(&dsrc, 6 * sizeof(float)), cudaSuccess);
    ASSERT_EQ(cudaMalloc(&ddst, 6 * sizeof(float)), cudaSuccess);

    EXPECT_EQ(bee::cuda::detail::ops_softmax(kDtI32, dsrc, ddst, 2, 3, 1), static_cast<int>(cudaErrorInvalidValue));
    EXPECT_EQ(bee::cuda::detail::ops_softmax(kDtF32, dsrc, ddst, 2, 0, 1), static_cast<int>(cudaErrorInvalidValue));
    EXPECT_EQ(bee::cuda::detail::ops_softmax(kDtF32, nullptr, ddst, 2, 3, 1), static_cast<int>(cudaErrorInvalidValue));
    EXPECT_EQ(bee::cuda::detail::ops_softmax(kDtF32, dsrc, nullptr, 2, 3, 1), static_cast<int>(cudaErrorInvalidValue));

    (void)cudaFree(dsrc);
    (void)cudaFree(ddst);
}
