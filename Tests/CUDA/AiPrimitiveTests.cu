/**
 * @File Tests/CUDA/AiPrimitiveTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 直接 bridge 层级的 RMSNorm/RoPE/Embedding CUDA kernel 正确性测试。
 *        遵循 TDD：先于生产代码编写，确保接口签名符合预期。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

// dtype 编码常量（与 Api.hpp ScalarType 整型编码一致）
constexpr int kDtBool = 0;
constexpr int kDtI32  = 2;
constexpr int kDtI64  = 3;
constexpr int kDtF32  = 4;
constexpr int kDtF64  = 5;

// 若 GPU 无该 sm 的 kernel image，返回 true 以触发 GTEST_SKIP
auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

// 工具：上传 host 数据到设备
template <typename T>
auto upload(const std::vector<T>& data) -> T*
{
    T* d = nullptr;
    if (cudaMalloc(&d, data.size() * sizeof(T)) != cudaSuccess)
        return nullptr;
    if (cudaMemcpy(d, data.data(), data.size() * sizeof(T), cudaMemcpyHostToDevice) != cudaSuccess) {
        (void)cudaFree(d);
        return nullptr;
    }
    return d;
}

// 工具：下载设备数据到 host
template <typename T>
auto download(const T* d, std::size_t n) -> std::vector<T>
{
    std::vector<T> h(n);
    (void)cudaMemcpy(h.data(), d, n * sizeof(T), cudaMemcpyDeviceToHost);
    return h;
}

} // 匿名命名空间

// ─── RMSNorm ──────────────────────────────────────────────────────────────────

// 全 1 输入 + 全 1 权重：rms = 1, rms_inv = 1/sqrt(1+eps) ≈ 1, out ≈ 1
TEST(CudaAiPrimitives, RmsNormF32Ones)
{
    constexpr std::size_t rows = 2, dim = 4;

    std::vector<float> x(rows * dim, 1.0f);
    std::vector<float> w(dim, 1.0f);

    float* dx  = upload(x);
    float* dw  = upload(w);
    float* dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dout, rows * dim * sizeof(float)), cudaSuccess);

    const int err = bee::cuda::detail::ops_rms_norm(kDtF32, dx, dw, dout, rows, dim, 1e-5);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dx);
        (void)cudaFree(dw);
        (void)cudaFree(dout);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    ASSERT_EQ(err, 0) << "ops_rms_norm F32 Ones err=" << err;

    const auto result = download(dout, rows * dim);
    for (std::size_t i = 0; i < rows * dim; ++i)
        EXPECT_NEAR(result[i], 1.0f, 1e-5f) << "i=" << i;

    (void)cudaFree(dx);
    (void)cudaFree(dw);
    (void)cudaFree(dout);
}

// F64 非平凡输入与 CPU 参考值比对
TEST(CudaAiPrimitives, RmsNormF64MatchesReference)
{
    constexpr std::size_t rows = 2, dim = 4;
    constexpr double      eps  = 1e-6;

    const std::vector<double> x = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    const std::vector<double> w = {1.0, 1.0, 1.0, 1.0};

    // 计算 CPU 参考值
    std::vector<double> expected(rows * dim);
    for (std::size_t r = 0; r < rows; ++r) {
        double sum2 = 0.0;
        for (std::size_t i = 0; i < dim; ++i) {
            const double v  = x[r * dim + i];
            sum2           += v * v;
        }
        const double rms_inv = 1.0 / std::sqrt(sum2 / static_cast<double>(dim) + eps);
        for (std::size_t i = 0; i < dim; ++i)
            expected[r * dim + i] = x[r * dim + i] * rms_inv * w[i];
    }

    double* dx   = upload(x);
    double* dw   = upload(w);
    double* dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dout, rows * dim * sizeof(double)), cudaSuccess);

    const int err = bee::cuda::detail::ops_rms_norm(kDtF64, dx, dw, dout, rows, dim, eps);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dx);
        (void)cudaFree(dw);
        (void)cudaFree(dout);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    ASSERT_EQ(err, 0) << "ops_rms_norm F64 err=" << err;

    const auto result = download(dout, rows * dim);
    for (std::size_t i = 0; i < rows * dim; ++i)
        EXPECT_NEAR(result[i], expected[i], 1e-10) << "i=" << i;

    (void)cudaFree(dx);
    (void)cudaFree(dw);
    (void)cudaFree(dout);
}

// 非法参数应返回非零错误码
TEST(CudaAiPrimitives, RmsNormRejectsInvalidArgs)
{
    float* d = nullptr;
    ASSERT_EQ(cudaMalloc(&d, 4 * sizeof(float)), cudaSuccess);

    // 空指针
    EXPECT_NE(bee::cuda::detail::ops_rms_norm(kDtF32, nullptr, d, d, 1, 4, 1e-5), 0);
    // rows=0
    EXPECT_NE(bee::cuda::detail::ops_rms_norm(kDtF32, d, d, d, 0, 4, 1e-5), 0);
    // dim=0
    EXPECT_NE(bee::cuda::detail::ops_rms_norm(kDtF32, d, d, d, 1, 0, 1e-5), 0);
    // eps=0
    EXPECT_NE(bee::cuda::detail::ops_rms_norm(kDtF32, d, d, d, 1, 4, 0.0), 0);
    // eps<0
    EXPECT_NE(bee::cuda::detail::ops_rms_norm(kDtF32, d, d, d, 1, 4, -1e-5), 0);
    // 不支持的 dtype
    EXPECT_NE(bee::cuda::detail::ops_rms_norm(kDtBool, d, d, d, 1, 4, 1e-5), 0);

    (void)cudaFree(d);
}

// ─── RoPE ─────────────────────────────────────────────────────────────────────

// F32，position_offset=1，与 CPU split-half 参考值比对
// shape={1,1,4}，dim=4，half=2，pos=1，base=10000
//   pair_i=0: theta=1.0,  out[0]=cos(1)-sin(1), out[2]=sin(1)+cos(1)
//   pair_i=1: theta=0.01, out[1]=cos(0.01)-sin(0.01), out[3]=sin(0.01)+cos(0.01)
TEST(CudaAiPrimitives, RopeF32PositionOffset)
{
    constexpr std::size_t n_batch = 1, seq_len = 1, dim = 4;

    const std::vector<float> x = {1.0f, 1.0f, 1.0f, 1.0f};

    float* dx   = upload(x);
    float* dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dout, dim * sizeof(float)), cudaSuccess);

    const int err = bee::cuda::detail::ops_rope(kDtF32, dx, dout, n_batch, seq_len, dim, 10000.0, 1);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dx);
        (void)cudaFree(dout);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    ASSERT_EQ(err, 0) << "ops_rope F32 position_offset=1 err=" << err;

    const auto result = download(dout, dim);

    const double c0 = std::cos(1.0),  s0 = std::sin(1.0);
    const double c1 = std::cos(0.01), s1 = std::sin(0.01);

    EXPECT_NEAR(result[0], static_cast<float>(c0 - s0), 1e-5f) << "out[0]";
    EXPECT_NEAR(result[1], static_cast<float>(c1 - s1), 1e-5f) << "out[1]";
    EXPECT_NEAR(result[2], static_cast<float>(s0 + c0), 1e-5f) << "out[2]";
    EXPECT_NEAR(result[3], static_cast<float>(s1 + c1), 1e-5f) << "out[3]";

    (void)cudaFree(dx);
    (void)cudaFree(dout);
}

// F64 多 token，与 CPU 参考值比对
TEST(CudaAiPrimitives, RopeF64MatchesReference)
{
    constexpr std::size_t n_batch = 1, seq_len = 2, dim = 4;
    constexpr int64_t     offset  = 0;
    constexpr double      base    = 10000.0;
    constexpr std::size_t half    = dim / 2;
    constexpr std::size_t total   = n_batch * seq_len * dim;

    std::vector<double> x(total);
    for (std::size_t i = 0; i < total; ++i)
        x[i] = static_cast<double>(i + 1);

    // 计算 CPU 参考值
    std::vector<double> expected(total);
    for (std::size_t b = 0; b < n_batch; ++b) {
        for (std::size_t s = 0; s < seq_len; ++s) {
            const double   pos = static_cast<double>(offset + static_cast<int64_t>(s));
            const std::size_t ro = (b * seq_len + s) * dim;
            for (std::size_t i = 0; i < half; ++i) {
                const double theta = pos / std::pow(base, 2.0 * static_cast<double>(i) / static_cast<double>(dim));
                const double c     = std::cos(theta);
                const double si    = std::sin(theta);
                const double x0    = x[ro + i];
                const double x1    = x[ro + i + half];
                expected[ro + i]        = x0 * c - x1 * si;
                expected[ro + i + half] = x0 * si + x1 * c;
            }
        }
    }

    double* dx   = upload(x);
    double* dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dout, total * sizeof(double)), cudaSuccess);

    const int err = bee::cuda::detail::ops_rope(kDtF64, dx, dout, n_batch, seq_len, dim, base, offset);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dx);
        (void)cudaFree(dout);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    ASSERT_EQ(err, 0) << "ops_rope F64 err=" << err;

    const auto result = download(dout, total);
    for (std::size_t i = 0; i < total; ++i)
        EXPECT_NEAR(result[i], expected[i], 1e-10) << "i=" << i;

    (void)cudaFree(dx);
    (void)cudaFree(dout);
}

// 非法参数应返回非零错误码
TEST(CudaAiPrimitives, RopeRejectsInvalidArgs)
{
    float* d = nullptr;
    ASSERT_EQ(cudaMalloc(&d, 4 * sizeof(float)), cudaSuccess);

    // 空指针
    EXPECT_NE(bee::cuda::detail::ops_rope(kDtF32, nullptr, d, 1, 1, 4, 10000.0, 0), 0);
    // n_batch=0
    EXPECT_NE(bee::cuda::detail::ops_rope(kDtF32, d, d, 0, 1, 4, 10000.0, 0), 0);
    // seq_len=0
    EXPECT_NE(bee::cuda::detail::ops_rope(kDtF32, d, d, 1, 0, 4, 10000.0, 0), 0);
    // dim 为奇数
    EXPECT_NE(bee::cuda::detail::ops_rope(kDtF32, d, d, 1, 1, 3, 10000.0, 0), 0);
    // base=0
    EXPECT_NE(bee::cuda::detail::ops_rope(kDtF32, d, d, 1, 1, 4, 0.0, 0), 0);
    // base<0
    EXPECT_NE(bee::cuda::detail::ops_rope(kDtF32, d, d, 1, 1, 4, -1.0, 0), 0);
    // 不支持的 dtype
    EXPECT_NE(bee::cuda::detail::ops_rope(kDtBool, d, d, 1, 1, 4, 10000.0, 0), 0);

    (void)cudaFree(d);
}

// ─── Embedding ────────────────────────────────────────────────────────────────

// F32 weight + I64 ids：取第 0、2 行，验证数值
TEST(CudaAiPrimitives, EmbeddingF32I64ReturnsRows)
{
    constexpr std::size_t vocab = 3, hidden = 4, n_ids = 2;

    const std::vector<float>   w   = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    const std::vector<int64_t> ids = {0, 2};
    const std::vector<float>   exp = {0, 1, 2, 3, 8, 9, 10, 11};

    float*   dw   = upload(w);
    int64_t* di   = upload(ids);
    float*   dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dout, n_ids * hidden * sizeof(float)), cudaSuccess);

    const int err = bee::cuda::detail::ops_embedding(kDtF32, kDtI64, dw, di, dout, n_ids, hidden, vocab);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dw);
        (void)cudaFree(di);
        (void)cudaFree(dout);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    ASSERT_EQ(err, 0) << "ops_embedding F32/I64 err=" << err;

    const auto result = download(dout, n_ids * hidden);
    for (std::size_t i = 0; i < n_ids * hidden; ++i)
        EXPECT_FLOAT_EQ(result[i], exp[i]) << "i=" << i;

    (void)cudaFree(dw);
    (void)cudaFree(di);
    (void)cudaFree(dout);
}

// F64 weight + I32 ids：取第 1、0 行，验证数值
TEST(CudaAiPrimitives, EmbeddingF64I32ReturnsRows)
{
    constexpr std::size_t vocab = 3, hidden = 4, n_ids = 2;

    const std::vector<double>  w   = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    const std::vector<int32_t> ids = {1, 0};
    const std::vector<double>  exp = {4, 5, 6, 7, 0, 1, 2, 3};

    double*  dw   = upload(w);
    int32_t* di   = upload(ids);
    double*  dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dout, n_ids * hidden * sizeof(double)), cudaSuccess);

    const int err = bee::cuda::detail::ops_embedding(kDtF64, kDtI32, dw, di, dout, n_ids, hidden, vocab);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dw);
        (void)cudaFree(di);
        (void)cudaFree(dout);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    ASSERT_EQ(err, 0) << "ops_embedding F64/I32 err=" << err;

    const auto result = download(dout, n_ids * hidden);
    for (std::size_t i = 0; i < n_ids * hidden; ++i)
        EXPECT_DOUBLE_EQ(result[i], exp[i]) << "i=" << i;

    (void)cudaFree(dw);
    (void)cudaFree(di);
    (void)cudaFree(dout);
}

// 越界 id 必须返回非零错误码（不允许静默越界）
TEST(CudaAiPrimitives, EmbeddingOutOfBoundsReturnsError)
{
    constexpr std::size_t vocab = 3, hidden = 4, n_ids = 1;

    const std::vector<float>   w   = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    const std::vector<int64_t> ids = {5}; // id=5 超过 vocab=3

    float*   dw   = upload(w);
    int64_t* di   = upload(ids);
    float*   dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dout, n_ids * hidden * sizeof(float)), cudaSuccess);

    const int err = bee::cuda::detail::ops_embedding(kDtF32, kDtI64, dw, di, dout, n_ids, hidden, vocab);
    if (is_kernel_image_missing(err)) {
        (void)cudaFree(dw);
        (void)cudaFree(di);
        (void)cudaFree(dout);
        GTEST_SKIP() << "No kernel image for current GPU";
    }
    EXPECT_NE(err, 0) << "越界 id 应返回非零错误码";

    (void)cudaFree(dw);
    (void)cudaFree(di);
    (void)cudaFree(dout);
}

// 非法参数应返回非零错误码
TEST(CudaAiPrimitives, EmbeddingRejectsInvalidArgs)
{
    float*   dw   = nullptr;
    int64_t* di   = nullptr;
    float*   dout = nullptr;
    ASSERT_EQ(cudaMalloc(&dw,   12 * sizeof(float)),   cudaSuccess);
    ASSERT_EQ(cudaMalloc(&di,    2 * sizeof(int64_t)), cudaSuccess);
    ASSERT_EQ(cudaMalloc(&dout,  8 * sizeof(float)),   cudaSuccess);

    // 空指针
    EXPECT_NE(bee::cuda::detail::ops_embedding(kDtF32, kDtI64, nullptr, di, dout, 2, 4, 3), 0);
    EXPECT_NE(bee::cuda::detail::ops_embedding(kDtF32, kDtI64, dw, nullptr, dout, 2, 4, 3), 0);
    // n_ids=0
    EXPECT_NE(bee::cuda::detail::ops_embedding(kDtF32, kDtI64, dw, di, dout, 0, 4, 3), 0);
    // hidden=0
    EXPECT_NE(bee::cuda::detail::ops_embedding(kDtF32, kDtI64, dw, di, dout, 2, 0, 3), 0);
    // 不支持的 weight dtype
    EXPECT_NE(bee::cuda::detail::ops_embedding(kDtBool, kDtI64, dw, di, dout, 2, 4, 3), 0);
    // 不支持的 ids dtype
    EXPECT_NE(bee::cuda::detail::ops_embedding(kDtF32, kDtBool, dw, di, dout, 2, 4, 3), 0);

    (void)cudaFree(dw);
    (void)cudaFree(di);
    (void)cudaFree(dout);
}
