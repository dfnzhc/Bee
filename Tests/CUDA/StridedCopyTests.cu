/**
 * @File Tests/CUDA/StridedCopyTests.cu
 * @Brief CUDA strided_copy 算子的正确性测试，覆盖转置视图与维度上限检查。
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

auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

} // namespace

// 拷贝转置视图：src 为列主序，shape {3,2}, 拷贝为行主序
TEST(CudaStridedCopyTests, CopiesTransposedView)
{
    // src 数据：[1, 2, 3, 4, 5, 6]，视为列主序 3x2 矩阵（3 行 2 列）
    // 列主序 strides {1, 3}：shape {3, 2}，numel=6
    // 期望输出行主序：[1, 4, 2, 5, 3, 6]（转置后）
    std::vector<float> src{1, 2, 3, 4, 5, 6};
    std::vector<float> dst(6, 0.0f);

    const std::int64_t shape[2]   = {3, 2};
    const std::int64_t strides[2] = {1, 3}; // 列主序

    float* dsrc = nullptr;
    float* ddst = nullptr;

    EXPECT_EQ(cudaMalloc(&dsrc, src.size() * sizeof(float)), cudaSuccess);
    if (!dsrc)
        return;
    EXPECT_EQ(cudaMalloc(&ddst, dst.size() * sizeof(float)), cudaSuccess);
    if (!ddst) {
        (void)cudaFree(dsrc);
        return;
    }

    EXPECT_EQ(cudaMemcpy(dsrc, src.data(), src.size() * sizeof(float), cudaMemcpyHostToDevice), cudaSuccess);

    // strided_copy：从 src storage 按 shape/strides 拷贝到连续 dst
    const int err = bee::cuda::detail::ops_strided_copy(kDtF32, dsrc, ddst, shape, strides, 2, 0, 6);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    EXPECT_EQ(err, 0) << "ops_strided_copy err=" << err;
    if (err != 0) {
        (void)cudaFree(dsrc);
        (void)cudaFree(ddst);
        return;
    }

    EXPECT_EQ(cudaMemcpy(dst.data(), ddst, dst.size() * sizeof(float), cudaMemcpyDeviceToHost), cudaSuccess);

    // 验证转置结果
    EXPECT_FLOAT_EQ(dst[0], 1.0f);
    EXPECT_FLOAT_EQ(dst[1], 4.0f);
    EXPECT_FLOAT_EQ(dst[2], 2.0f);
    EXPECT_FLOAT_EQ(dst[3], 5.0f);
    EXPECT_FLOAT_EQ(dst[4], 3.0f);
    EXPECT_FLOAT_EQ(dst[5], 6.0f);

    (void)cudaFree(ddst);
    (void)cudaFree(dsrc);
}

// 拒绝超过 8 维的 strided_copy
TEST(CudaStridedCopyTests, RejectsMoreThanEightDimensions)
{
    std::vector<float> src(8, 1.0f);
    std::vector<float> dst(8, 0.0f);

    // 构造 9 维 shape/strides（虽然 numel 只有 8）
    const std::int64_t shape[9]   = {2, 1, 1, 1, 1, 1, 1, 1, 4};
    const std::int64_t strides[9] = {4, 4, 4, 4, 4, 4, 4, 4, 1};

    float* dsrc = nullptr;
    float* ddst = nullptr;

    EXPECT_EQ(cudaMalloc(&dsrc, src.size() * sizeof(float)), cudaSuccess);
    if (!dsrc)
        return;
    EXPECT_EQ(cudaMalloc(&ddst, dst.size() * sizeof(float)), cudaSuccess);
    if (!ddst) {
        (void)cudaFree(dsrc);
        return;
    }

    EXPECT_EQ(cudaMemcpy(dsrc, src.data(), src.size() * sizeof(float), cudaMemcpyHostToDevice), cudaSuccess);

    // ndim=9 应返回错误
    const int err = bee::cuda::detail::ops_strided_copy(kDtF32, dsrc, ddst, shape, strides, 9, 0, 8);
    EXPECT_NE(err, 0) << "strided_copy 应拒绝 ndim=9";

    (void)cudaFree(ddst);
    (void)cudaFree(dsrc);
}
