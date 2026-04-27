/**
 * @File Tests/CUDA/LowPrecisionGemmTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Task 5: F16/BF16 lowp GEMM direct bridge tests.
 *        Inputs cast F32->F16/BF16 via ops_cast; output is F32.
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cuda_bf16.h>

#include <cstdint>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtF32  = 4;
constexpr int kDtF16  = 7;
constexpr int kDtBF16 = 8;

// 判断是否因缺少 kernel image 导致的错误（跳过而非失败）
auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

// 在设备上分配低精度缓冲，并通过 ops_cast 从 F32 主机数据填充
auto alloc_and_cast_to_lowp(int dst_dt, const std::vector<float>& host_f32, void** out_dev) -> bool
{
    const std::size_t n = host_f32.size();
    float* d_f32 = nullptr;
    if (cudaMalloc(&d_f32, n * sizeof(float)) != cudaSuccess) return false;
    if (cudaMemcpy(d_f32, host_f32.data(), n * sizeof(float), cudaMemcpyHostToDevice) != cudaSuccess) {
        (void)cudaFree(d_f32);
        return false;
    }
    // F16/BF16 各占 2 字节
    if (cudaMalloc(out_dev, n * 2) != cudaSuccess) {
        (void)cudaFree(d_f32);
        return false;
    }
    const int err = bee::cuda::detail::ops_cast(kDtF32, d_f32, dst_dt, *out_dev, n);
    (void)cudaFree(d_f32);
    if (err != 0) {
        (void)cudaFree(*out_dev);
        *out_dev = nullptr;
        return false;
    }
    return true;
}

// CPU 参考 F32 GEMM：C[M,N] = A[M,K] * B[K,N]
auto ref_gemm_f32(std::size_t M, std::size_t K, std::size_t N,
                  const std::vector<float>& A, const std::vector<float>& B) -> std::vector<float>
{
    std::vector<float> C(M * N, 0.0f);
    for (std::size_t i = 0; i < M; ++i)
        for (std::size_t j = 0; j < N; ++j) {
            float acc = 0.0f;
            for (std::size_t k = 0; k < K; ++k)
                acc += A[i * K + k] * B[k * N + j];
            C[i * N + j] = acc;
        }
    return C;
}

} // namespace

// F16 输入产生 F32 输出：A[2,3] x B[3,2]，值 arange(0..5)
TEST(CudaLowPrecisionGemm, F16InputsProduceF32Output)
{
    // A = [[0,1,2],[3,4,5]], B = [[0,1],[2,3],[4,5]]
    std::vector<float> a_f32 = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    std::vector<float> b_f32 = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};

    constexpr std::size_t M = 2, K = 3, N = 2;

    void*  d_a = nullptr;
    void*  d_b = nullptr;
    float* d_c = nullptr;

    if (!alloc_and_cast_to_lowp(kDtF16, a_f32, &d_a))
        GTEST_SKIP() << "Cannot alloc/cast F16 buffer";
    if (!alloc_and_cast_to_lowp(kDtF16, b_f32, &d_b)) {
        (void)cudaFree(d_a);
        GTEST_SKIP() << "Cannot alloc/cast F16 buffer";
    }
    if (cudaMalloc(&d_c, M * N * sizeof(float)) != cudaSuccess) {
        (void)cudaFree(d_a);
        (void)cudaFree(d_b);
        GTEST_SKIP() << "Cannot allocate F32 output buffer";
    }

    const int err = bee::cuda::detail::ops_matmul_lowp(kDtF16, d_a, d_b, d_c, M, K, N);

    if (is_kernel_image_missing(err)) {
        (void)cudaFree(d_a); (void)cudaFree(d_b); (void)cudaFree(d_c);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    EXPECT_EQ(err, 0) << "ops_matmul_lowp(F16) failed with err=" << err;

    // 拷回结果并与 CPU 参考值比对（F16 精度误差 <= 1e-2）
    std::vector<float> c_host(M * N);
    EXPECT_EQ(cudaMemcpy(c_host.data(), d_c, M * N * sizeof(float), cudaMemcpyDeviceToHost), cudaSuccess);

    const auto ref = ref_gemm_f32(M, K, N, a_f32, b_f32);
    for (std::size_t i = 0; i < M * N; ++i)
        EXPECT_NEAR(c_host[i], ref[i], 1e-2f) << "index=" << i;

    (void)cudaFree(d_a); (void)cudaFree(d_b); (void)cudaFree(d_c);
}

// BF16 输入产生 F32 输出：A[2,3] x B[3,2]，值 arange(0..5)
TEST(CudaLowPrecisionGemm, BF16InputsProduceF32Output)
{
    std::vector<float> a_f32 = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    std::vector<float> b_f32 = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};

    constexpr std::size_t M = 2, K = 3, N = 2;

    void*  d_a = nullptr;
    void*  d_b = nullptr;
    float* d_c = nullptr;

    if (!alloc_and_cast_to_lowp(kDtBF16, a_f32, &d_a))
        GTEST_SKIP() << "Cannot alloc/cast BF16 buffer";
    if (!alloc_and_cast_to_lowp(kDtBF16, b_f32, &d_b)) {
        (void)cudaFree(d_a);
        GTEST_SKIP() << "Cannot alloc/cast BF16 buffer";
    }
    if (cudaMalloc(&d_c, M * N * sizeof(float)) != cudaSuccess) {
        (void)cudaFree(d_a);
        (void)cudaFree(d_b);
        GTEST_SKIP() << "Cannot allocate F32 output buffer";
    }

    const int err = bee::cuda::detail::ops_matmul_lowp(kDtBF16, d_a, d_b, d_c, M, K, N);

    if (is_kernel_image_missing(err)) {
        (void)cudaFree(d_a); (void)cudaFree(d_b); (void)cudaFree(d_c);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    EXPECT_EQ(err, 0) << "ops_matmul_lowp(BF16) failed with err=" << err;

    std::vector<float> c_host(M * N);
    EXPECT_EQ(cudaMemcpy(c_host.data(), d_c, M * N * sizeof(float), cudaMemcpyDeviceToHost), cudaSuccess);

    const auto ref = ref_gemm_f32(M, K, N, a_f32, b_f32);
    // BF16 精度容差更大（<= 5e-2）
    for (std::size_t i = 0; i < M * N; ++i)
        EXPECT_NEAR(c_host[i], ref[i], 5e-2f) << "index=" << i;

    (void)cudaFree(d_a); (void)cudaFree(d_b); (void)cudaFree(d_c);
}

// 非法参数：不支持的 dtype（F32=4）应返回错误
TEST(CudaLowPrecisionGemm, UnsupportedDtypeReturnsError)
{
    float* d = nullptr;
    ASSERT_EQ(cudaMalloc(&d, 4 * sizeof(float)), cudaSuccess);
    float dummy[4] = {1,2,3,4};
    (void)cudaMemcpy(d, dummy, sizeof(dummy), cudaMemcpyHostToDevice);

    const int err = bee::cuda::detail::ops_matmul_lowp(kDtF32, d, d, d, 2, 2, 2);
    EXPECT_NE(err, 0) << "F32 dtype should be rejected by ops_matmul_lowp";
    EXPECT_NE(bee::cuda::detail::ops_matmul_lowp(kDtF32, nullptr, nullptr, nullptr, 0, 2, 2), 0)
        << "F32 dtype should be rejected even when M==0";

    (void)cudaFree(d);
}

// 非法参数：空指针（有效尺寸）应返回错误
TEST(CudaLowPrecisionGemm, NullPointerReturnsError)
{
    float* d_c = nullptr;
    ASSERT_EQ(cudaMalloc(&d_c, 4 * sizeof(float)), cudaSuccess);

    const int err = bee::cuda::detail::ops_matmul_lowp(kDtF16, nullptr, nullptr, d_c, 2, 2, 2);
    EXPECT_NE(err, 0) << "Null A/B should be rejected";

    (void)cudaFree(d_c);
}

// M==0 时应立即成功（no-op）
TEST(CudaLowPrecisionGemm, ZeroMIsNoOp)
{
    const int err = bee::cuda::detail::ops_matmul_lowp(kDtF16, nullptr, nullptr, nullptr, 0, 3, 2);
    EXPECT_EQ(err, 0) << "M==0 should be a no-op";
}

// N==0 时应立即成功（no-op）
TEST(CudaLowPrecisionGemm, ZeroNIsNoOp)
{
    const int err = bee::cuda::detail::ops_matmul_lowp(kDtF16, nullptr, nullptr, nullptr, 2, 3, 0);
    EXPECT_EQ(err, 0) << "N==0 should be a no-op";
}
