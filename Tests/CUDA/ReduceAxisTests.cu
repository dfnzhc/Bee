/**
 * @File Tests/CUDA/ReduceAxisTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/24
 * @Brief CUDA reduce_axis kernel 的正确性测试。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtI32 = 2;
constexpr int kDtI64 = 3;
constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

constexpr int kRdSum  = 0;
constexpr int kRdMin  = 1;
constexpr int kRdMax  = 2;
constexpr int kRdProd = 3;

auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

template <typename T>
auto run_reduce_axis(int op, int dt, std::size_t outer, std::size_t axis, std::size_t inner, const std::vector<T>& input) -> std::optional<std::vector<T>>
{
    T* dsrc = nullptr;
    T* ddst = nullptr;
    const auto src_elems = outer * axis * inner;
    const auto dst_elems = outer * inner;

    EXPECT_EQ(cudaMalloc(&dsrc, src_elems * sizeof(T)), cudaSuccess);
    if (!dsrc)
        return std::vector<T>{};
    EXPECT_EQ(cudaMalloc(&ddst, dst_elems * sizeof(T)), cudaSuccess);
    if (!ddst) {
        (void)cudaFree(dsrc);
        return std::vector<T>{};
    }
    EXPECT_EQ(cudaMemcpy(dsrc, input.data(), src_elems * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);

    const int err = bee::cuda::detail::ops_reduce_axis(op, dt, dsrc, ddst, outer, axis, inner);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::nullopt;
    }
    EXPECT_EQ(err, 0) << "ops_reduce_axis err=" << err;
    if (err != 0) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return std::vector<T>{};
    }

    std::vector<T> output(dst_elems);
    EXPECT_EQ(cudaMemcpy(output.data(), ddst, dst_elems * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(dsrc);
    (void)cudaFree(ddst);
    return output;
}

template <typename T>
auto reduce_axis_ref(int op, std::size_t outer, std::size_t axis, std::size_t inner, const std::vector<T>& input) -> std::vector<T>
{
    std::vector<T> output(outer * inner);
    for (std::size_t o = 0; o < outer; ++o) {
        for (std::size_t i = 0; i < inner; ++i) {
            T acc = input[(o * axis) * inner + i];
            for (std::size_t k = 1; k < axis; ++k) {
                const T value = input[(o * axis + k) * inner + i];
                switch (op) {
                case kRdSum: acc += value; break;
                case kRdMin: acc = value < acc ? value : acc; break;
                case kRdMax: acc = value > acc ? value : acc; break;
                case kRdProd: acc *= value; break;
                default: break;
                }
            }
            output[o * inner + i] = acc;
        }
    }
    return output;
}

} // namespace

TEST(CudaReduceAxis, SumF32NonAlignedShape)
{
    constexpr std::size_t outer = 2;
    constexpr std::size_t axis  = 3;
    constexpr std::size_t inner = 5;
    std::vector<float> input(outer * axis * inner);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<float>((i % 11) - 5) * 0.25f;

    auto output = run_reduce_axis<float>(kRdSum, kDtF32, outer, axis, inner, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    const auto expected = reduce_axis_ref(kRdSum, outer, axis, inner, input);
    ASSERT_EQ(output->size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_FLOAT_EQ((*output)[i], expected[i]) << "i=" << i;
}

TEST(CudaReduceAxis, MinI32NonAlignedShape)
{
    constexpr std::size_t outer = 3;
    constexpr std::size_t axis  = 4;
    constexpr std::size_t inner = 2;
    std::vector<std::int32_t> input(outer * axis * inner);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<std::int32_t>((i * 7) % 19) - 9;

    auto output = run_reduce_axis<std::int32_t>(kRdMin, kDtI32, outer, axis, inner, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    EXPECT_EQ(*output, reduce_axis_ref(kRdMin, outer, axis, inner, input));
}

TEST(CudaReduceAxis, MaxI64NonAlignedShape)
{
    constexpr std::size_t outer = 1;
    constexpr std::size_t axis  = 5;
    constexpr std::size_t inner = 3;
    std::vector<std::int64_t> input(outer * axis * inner);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<std::int64_t>(i) * 13 - 17;

    auto output = run_reduce_axis<std::int64_t>(kRdMax, kDtI64, outer, axis, inner, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    EXPECT_EQ(*output, reduce_axis_ref(kRdMax, outer, axis, inner, input));
}

TEST(CudaReduceAxis, ProdF64NonAlignedShape)
{
    constexpr std::size_t outer = 2;
    constexpr std::size_t axis  = 3;
    constexpr std::size_t inner = 2;
    std::vector<double> input(outer * axis * inner);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = 1.0 + static_cast<double>((i % 5) + 1) * 0.1;

    auto output = run_reduce_axis<double>(kRdProd, kDtF64, outer, axis, inner, input);
    if (!output)
        GTEST_SKIP() << "No kernel image for current GPU";

    const auto expected = reduce_axis_ref(kRdProd, outer, axis, inner, input);
    ASSERT_EQ(output->size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_DOUBLE_EQ((*output)[i], expected[i]) << "i=" << i;
}
