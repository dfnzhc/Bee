/**
 * @File Tests/CUDA/TransposeTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/24
 * @Brief CUDA transpose kernel 的正确性测试。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice) || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

template <typename T>
auto run_transpose(int dt, std::size_t rows, std::size_t cols, const std::vector<T>& input) -> std::optional<std::vector<T>>
{
    T*         dsrc = nullptr;
    T*         ddst = nullptr;
    const auto n    = rows * cols;

    EXPECT_EQ(cudaMalloc(&dsrc, n * sizeof(T)), cudaSuccess);
    if (!dsrc)
        return std::vector<T>{};
    EXPECT_EQ(cudaMalloc(&ddst, n * sizeof(T)), cudaSuccess);
    if (!ddst) {
        (void)cudaFree(dsrc);
        return std::vector<T>{};
    }
    EXPECT_EQ(cudaMemcpy(dsrc, input.data(), n * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);

    const int err = bee::cuda::detail::ops_transpose_2d(dt, dsrc, ddst, rows, cols);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::nullopt;
    }
    EXPECT_EQ(err, 0) << "ops_transpose_2d err=" << err;
    if (err != 0) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::vector<T>{};
    }

    std::vector<T> output(n);
    EXPECT_EQ(cudaMemcpy(output.data(), ddst, n * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(dsrc);
    (void)cudaFree(ddst);
    return output;
}

template <typename T>
auto transpose_ref(std::size_t rows, std::size_t cols, const std::vector<T>& input) -> std::vector<T>
{
    std::vector<T> output(rows * cols);
    for (std::size_t r = 0; r < rows; ++r)
        for (std::size_t c = 0; c < cols; ++c)
            output[c * rows + r] = input[r * cols + c];
    return output;
}

} // namespace

TEST(CudaTranspose, F64RectangularFallbackPath)
{
    constexpr std::size_t rows = 3;
    constexpr std::size_t cols = 5;
    std::vector<double>   input(rows * cols);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<double>(i) * 0.5;

    auto output = run_transpose<double>(kDtF64, rows, cols, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    EXPECT_EQ(*output, transpose_ref(rows, cols, input));
}

TEST(CudaTranspose, F32VectorizedPathOnAlignedTile)
{
    constexpr std::size_t rows = 32;
    constexpr std::size_t cols = 64;
    std::vector<float>    input(rows * cols);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<float>((i % 17) - 8) * 0.25f;

    auto output = run_transpose<float>(kDtF32, rows, cols, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    const auto expected = transpose_ref(rows, cols, input);
    ASSERT_EQ(output->size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_FLOAT_EQ((*output)[i], expected[i]) << "i=" << i;
}
