/**
 * @File Tests/CUDA/MatmulTests.cu
 * @Brief CUDA matmul 算子的正确性测试，覆盖基本矩阵乘法与非 4 对齐 fallback 路径。
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

// 小矩阵乘法：A[2,3] x B[3,2] = C[2,2]
TEST(CudaMatmulTests, F32SmallMatrix)
{
    // A = [1 2 3; 4 5 6]，行主序
    std::vector<float> a{1, 2, 3, 4, 5, 6};
    // B = [7 8; 9 10; 11 12]，行主序
    std::vector<float> b{7, 8, 9, 10, 11, 12};
    // C = [1*7+2*9+3*11, 1*8+2*10+3*12; 4*7+5*9+6*11, 4*8+5*10+6*12]
    //   = [58, 64; 139, 154]
    std::vector<float> c(4, 0.0f);

    float* da = nullptr;
    float* db = nullptr;
    float* dc = nullptr;

    EXPECT_EQ(cudaMalloc(&da, a.size() * sizeof(float)), cudaSuccess);
    if (!da)
        return;
    EXPECT_EQ(cudaMalloc(&db, b.size() * sizeof(float)), cudaSuccess);
    if (!db) {
        (void)cudaFree(da);
        return;
    }
    EXPECT_EQ(cudaMalloc(&dc, c.size() * sizeof(float)), cudaSuccess);
    if (!dc) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        return;
    }

    EXPECT_EQ(cudaMemcpy(da, a.data(), a.size() * sizeof(float), cudaMemcpyHostToDevice), cudaSuccess);
    EXPECT_EQ(cudaMemcpy(db, b.data(), b.size() * sizeof(float), cudaMemcpyHostToDevice), cudaSuccess);

    // matmul: C[M,N] = A[M,K] * B[K,N]，M=2, K=3, N=2
    const int err = bee::cuda::detail::ops_matmul(kDtF32, da, db, dc, 2, 3, 2);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        (void)cudaFree(dc);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    EXPECT_EQ(err, 0) << "ops_matmul err=" << err;
    if (err != 0) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        (void)cudaFree(dc);
        return;
    }

    EXPECT_EQ(cudaMemcpy(c.data(), dc, c.size() * sizeof(float), cudaMemcpyDeviceToHost), cudaSuccess);

    EXPECT_FLOAT_EQ(c[0], 58.0f);
    EXPECT_FLOAT_EQ(c[1], 64.0f);
    EXPECT_FLOAT_EQ(c[2], 139.0f);
    EXPECT_FLOAT_EQ(c[3], 154.0f);

    (void)cudaFree(dc);
    (void)cudaFree(db);
    (void)cudaFree(da);
}

// 非 4 对齐矩阵乘法：M=5, K=3, N=7，期望触发 baseline/CUTLASS fallback 并正确计算
TEST(CudaMatmulTests, F32NonMultipleOfFourFallsBackAndComputes)
{
    constexpr std::size_t m = 5, k = 3, n = 7;
    std::vector<float> a(m * k, 1.0f); // A 全 1
    std::vector<float> b(k * n, 2.0f); // B 全 2
    std::vector<float> c(m * n, 0.0f);

    float* da = nullptr;
    float* db = nullptr;
    float* dc = nullptr;

    EXPECT_EQ(cudaMalloc(&da, a.size() * sizeof(float)), cudaSuccess);
    if (!da)
        return;
    EXPECT_EQ(cudaMalloc(&db, b.size() * sizeof(float)), cudaSuccess);
    if (!db) {
        (void)cudaFree(da);
        return;
    }
    EXPECT_EQ(cudaMalloc(&dc, c.size() * sizeof(float)), cudaSuccess);
    if (!dc) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        return;
    }

    EXPECT_EQ(cudaMemcpy(da, a.data(), a.size() * sizeof(float), cudaMemcpyHostToDevice), cudaSuccess);
    EXPECT_EQ(cudaMemcpy(db, b.data(), b.size() * sizeof(float), cudaMemcpyHostToDevice), cudaSuccess);

    // 每个输出元素 = sum(1 * 2 for 3 次) = 6
    const int err = bee::cuda::detail::ops_matmul(kDtF32, da, db, dc, m, k, n);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        (void)cudaFree(dc);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    EXPECT_EQ(err, 0) << "ops_matmul err=" << err;
    if (err != 0) {
        (void)cudaFree(da);
        (void)cudaFree(db);
        (void)cudaFree(dc);
        return;
    }

    EXPECT_EQ(cudaMemcpy(c.data(), dc, c.size() * sizeof(float), cudaMemcpyDeviceToHost), cudaSuccess);

    for (std::size_t i = 0; i < c.size(); ++i)
        EXPECT_FLOAT_EQ(c[i], 6.0f) << "index=" << i;

    (void)cudaFree(dc);
    (void)cudaFree(db);
    (void)cudaFree(da);
}
