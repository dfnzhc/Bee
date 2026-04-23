/**
 * @File WarpBlockTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * Bee::CUDA Warp/Block 原语测试：覆盖 shuffle / reduce / scan / broadcast /
 * any / all / ballot；多种 blockDim（32/64/128/256/512/1024 与非 32 倍数的 96/200）。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cstdint>
#include <numeric>
#include <vector>

#include "CUDA/Core/Warp.cuh"
#include "CUDA/Core/Block.cuh"

namespace
{

// ── 通用工具 ─────────────────────────────────────────────────────────────────

// 简易 RAII device buffer
template <typename T>
struct DBuf
{
    T*          p{nullptr};
    std::size_t n{0};
    DBuf(std::size_t count)
        : n(count)
    {
        cudaMalloc(&p, count * sizeof(T));
    }

    ~DBuf()
    {
        if (p)
            cudaFree(p);
    }

    DBuf(const DBuf&)                    = delete;
    auto operator=(const DBuf&) -> DBuf& = delete;
};

// skip if kernel launch fails with "no kernel image"
#define SKIP_IF_NO_IMAGE()                                                                   \
    {                                                                                        \
        cudaError_t _e = cudaGetLastError();                                                 \
        if (_e == cudaErrorNoKernelImageForDevice || _e == cudaErrorInvalidDeviceFunction) { \
            GTEST_SKIP() << "Kernel image incompatible: " << cudaGetErrorString(_e);         \
        }                                                                                    \
        ASSERT_EQ(_e, cudaSuccess) << cudaGetErrorString(_e);                                \
    }

// ── Warp shuffle / broadcast ─────────────────────────────────────────────────

__global__ void k_warp_broadcast(int* out, int src_lane)
{
    int v            = static_cast<int>(threadIdx.x) * 10 + 1;
    int b            = bee::cuda::warp_broadcast(v, src_lane);
    out[threadIdx.x] = b;
}

TEST(CudaWarp, Broadcast)
{
    constexpr int N = 32;
    DBuf<int>     d(N);
    k_warp_broadcast<<<1, N>>>(d.p, 7);
    SKIP_IF_NO_IMAGE();
    std::vector<int> h(N);
    ASSERT_EQ(cudaMemcpy(h.data(), d.p, N * sizeof(int), cudaMemcpyDeviceToHost), cudaSuccess);
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(h[i], 71);
}

__global__ void k_warp_shfl_xor(int* out)
{
    int v = static_cast<int>(threadIdx.x);
    // XOR 1 交换相邻 lane
    out[threadIdx.x] = bee::cuda::warp_shfl_xor(v, 1);
}

TEST(CudaWarp, ShflXor)
{
    constexpr int N = 32;
    DBuf<int>     d(N);
    k_warp_shfl_xor<<<1, N>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> h(N);
    cudaMemcpy(h.data(), d.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(h[i], i ^ 1);
}

__global__ void k_warp_shfl_up_down(int* up_out, int* dn_out)
{
    int v               = static_cast<int>(threadIdx.x) + 1;
    up_out[threadIdx.x] = bee::cuda::warp_shfl_up(v, 2u);
    dn_out[threadIdx.x] = bee::cuda::warp_shfl_down(v, 3u);
}

TEST(CudaWarp, ShflUpDown)
{
    constexpr int N = 32;
    DBuf<int>     u(N);
    DBuf<int>     d(N);
    k_warp_shfl_up_down<<<1, N>>>(u.p, d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> uh(N), dh(N);
    cudaMemcpy(uh.data(), u.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(dh.data(), d.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; ++i) {
        // up: delta=2, 对 lane<2 返回原值
        int expected_up = (i < 2) ? (i + 1) : (i - 2 + 1);
        // down: delta=3, 对 lane>=29 返回原值
        int expected_dn = (i >= 29) ? (i + 1) : (i + 3 + 1);
        EXPECT_EQ(uh[i], expected_up) << "lane " << i;
        EXPECT_EQ(dh[i], expected_dn) << "lane " << i;
    }
}

// ── Warp reduce（int / float / double）─────────────────────────────────────

__global__ void k_warp_reduce_sum_i(int* out)
{
    int v            = static_cast<int>(threadIdx.x) + 1;
    out[threadIdx.x] = bee::cuda::warp_reduce_sum(v);
}

__global__ void k_warp_reduce_sum_f(float* out)
{
    float v          = 0.5f * static_cast<float>(threadIdx.x);
    out[threadIdx.x] = bee::cuda::warp_reduce_sum(v);
}

__global__ void k_warp_reduce_sum_d(double* out)
{
    double v         = 0.25 * static_cast<double>(threadIdx.x);
    out[threadIdx.x] = bee::cuda::warp_reduce_sum(v);
}

__global__ void k_warp_reduce_min_max(int* mn, int* mx)
{
    int v           = 100 - static_cast<int>(threadIdx.x);
    mn[threadIdx.x] = bee::cuda::warp_reduce_min(v);
    mx[threadIdx.x] = bee::cuda::warp_reduce_max(v);
}

TEST(CudaWarp, ReduceSumInt)
{
    constexpr int N = 32;
    DBuf<int>     d(N);
    k_warp_reduce_sum_i<<<1, N>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> h(N);
    cudaMemcpy(h.data(), d.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    constexpr int expected = 32 * 33 / 2;
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(h[i], expected);
}

TEST(CudaWarp, ReduceSumFloat)
{
    constexpr int N = 32;
    DBuf<float>   d(N);
    k_warp_reduce_sum_f<<<1, N>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<float> h(N);
    cudaMemcpy(h.data(), d.p, N * sizeof(float), cudaMemcpyDeviceToHost);
    float expected = 0.0f;
    for (int i = 0; i < N; ++i)
        expected += 0.5f * i;
    for (int i = 0; i < N; ++i)
        EXPECT_FLOAT_EQ(h[i], expected);
}

TEST(CudaWarp, ReduceSumDouble)
{
    constexpr int N = 32;
    DBuf<double>  d(N);
    k_warp_reduce_sum_d<<<1, N>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<double> h(N);
    cudaMemcpy(h.data(), d.p, N * sizeof(double), cudaMemcpyDeviceToHost);
    double expected = 0.0;
    for (int i = 0; i < N; ++i)
        expected += 0.25 * i;
    for (int i = 0; i < N; ++i)
        EXPECT_DOUBLE_EQ(h[i], expected);
}

TEST(CudaWarp, ReduceMinMax)
{
    constexpr int N = 32;
    DBuf<int>     mn(N), mx(N);
    k_warp_reduce_min_max<<<1, N>>>(mn.p, mx.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> mnh(N), mxh(N);
    cudaMemcpy(mnh.data(), mn.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(mxh.data(), mx.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(mnh[i], 69);  // 100 - 31
        EXPECT_EQ(mxh[i], 100); // 100 - 0
    }
}

// ── Warp scan ────────────────────────────────────────────────────────────────

__global__ void k_warp_scan_inclusive(int* out)
{
    int v            = static_cast<int>(threadIdx.x) + 1;
    out[threadIdx.x] = bee::cuda::warp_scan_inclusive_sum(v);
}

__global__ void k_warp_scan_exclusive(int* out)
{
    int v            = static_cast<int>(threadIdx.x) + 1;
    out[threadIdx.x] = bee::cuda::warp_scan_exclusive_sum(v);
}

TEST(CudaWarp, ScanInclusive)
{
    constexpr int N = 32;
    DBuf<int>     d(N);
    k_warp_scan_inclusive<<<1, N>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> h(N);
    cudaMemcpy(h.data(), d.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    int acc = 0;
    for (int i = 0; i < N; ++i) {
        acc += i + 1;
        EXPECT_EQ(h[i], acc);
    }
}

TEST(CudaWarp, ScanExclusive)
{
    constexpr int N = 32;
    DBuf<int>     d(N);
    k_warp_scan_exclusive<<<1, N>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> h(N);
    cudaMemcpy(h.data(), d.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    int acc = 0;
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(h[i], acc);
        acc += i + 1;
    }
}

// ── Warp any/all/ballot ──────────────────────────────────────────────────────

__global__ void k_warp_any_all(int* anys, int* alls)
{
    bool pred         = (threadIdx.x == 17);
    anys[threadIdx.x] = bee::cuda::warp_any(pred) ? 1 : 0;
    alls[threadIdx.x] = bee::cuda::warp_all(pred) ? 1 : 0;
}

TEST(CudaWarp, AnyAll)
{
    constexpr int N = 32;
    DBuf<int>     a(N), b(N);
    k_warp_any_all<<<1, N>>>(a.p, b.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> ah(N), bh(N);
    cudaMemcpy(ah.data(), a.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(bh.data(), b.p, N * sizeof(int), cudaMemcpyDeviceToHost);
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(ah[i], 1);
        EXPECT_EQ(bh[i], 0);
    }
}

__global__ void k_warp_ballot(unsigned int* out)
{
    bool pred        = (threadIdx.x & 1) != 0;
    out[threadIdx.x] = bee::cuda::warp_ballot(pred);
}

TEST(CudaWarp, Ballot)
{
    constexpr int      N = 32;
    DBuf<unsigned int> d(N);
    k_warp_ballot<<<1, N>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<unsigned int> h(N);
    cudaMemcpy(h.data(), d.p, N * sizeof(unsigned int), cudaMemcpyDeviceToHost);
    constexpr unsigned int expected = 0xAAAAAAAAu;
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(h[i], expected);
}

// ── Block reduce ─────────────────────────────────────────────────────────────

template <int B>
__global__ void k_block_reduce_sum(int* out)
{
    using Red = bee::cuda::BlockReduce<int, B>;
    __shared__ typename Red::TempStorage smem;

    int v   = static_cast<int>(threadIdx.x) + 1;
    int sum = Red(smem).Sum(v);
    if (threadIdx.x == 0)
        *out = sum;
}

template <int B>
static void run_block_reduce_sum_case()
{
    DBuf<int> d(1);
    k_block_reduce_sum<B><<<1, B>>>(d.p);
    cudaError_t e = cudaGetLastError();
    if (e == cudaErrorNoKernelImageForDevice || e == cudaErrorInvalidDeviceFunction) {
        GTEST_SKIP() << cudaGetErrorString(e);
    }
    ASSERT_EQ(e, cudaSuccess) << cudaGetErrorString(e);
    int h = 0;
    cudaMemcpy(&h, d.p, sizeof(int), cudaMemcpyDeviceToHost);
    EXPECT_EQ(h, B * (B + 1) / 2) << "BlockSize=" << B;
}

TEST(CudaBlock, ReduceSum32)
{
    run_block_reduce_sum_case<32>();
}
TEST(CudaBlock, ReduceSum64)
{
    run_block_reduce_sum_case<64>();
}
TEST(CudaBlock, ReduceSum128)
{
    run_block_reduce_sum_case<128>();
}
TEST(CudaBlock, ReduceSum256)
{
    run_block_reduce_sum_case<256>();
}
TEST(CudaBlock, ReduceSum512)
{
    run_block_reduce_sum_case<512>();
}
TEST(CudaBlock, ReduceSum1024)
{
    run_block_reduce_sum_case<1024>();
}
TEST(CudaBlock, ReduceSum96)
{
    run_block_reduce_sum_case<96>();
}
TEST(CudaBlock, ReduceSum200)
{
    run_block_reduce_sum_case<200>();
}

template <int B>
__global__ void k_block_reduce_min(int* out)
{
    using Red = bee::cuda::BlockReduce<int, B, bee::cuda::WarpOpMin>;
    __shared__ typename Red::TempStorage smem;

    int v = 10000 - static_cast<int>(threadIdx.x);
    int r = Red(smem).Min(v, 0x7fffffff);
    if (threadIdx.x == 0)
        *out = r;
}

TEST(CudaBlock, ReduceMin)
{
    DBuf<int> d(1);
    k_block_reduce_min<256><<<1, 256>>>(d.p);
    SKIP_IF_NO_IMAGE();
    int h = 0;
    cudaMemcpy(&h, d.p, sizeof(int), cudaMemcpyDeviceToHost);
    EXPECT_EQ(h, 10000 - 255);
}

// ── Block scan ───────────────────────────────────────────────────────────────

template <int B>
__global__ void k_block_scan(int* out_inc, int* out_exc)
{
    using Scan = bee::cuda::BlockScan<int, B>;
    __shared__ typename Scan::TempStorage smem_inc;
    __shared__ typename Scan::TempStorage smem_exc;

    int v = 1;
    // Inclusive scan: out_inc[tid] = tid + 1
    int inc              = Scan(smem_inc).InclusiveSum(v);
    out_inc[threadIdx.x] = inc;
    // 重新计算（TempStorage 已污染，但 ExclusiveSum 内部先跑 inclusive 再减输入）
    int exc              = Scan(smem_exc).ExclusiveSum(v);
    out_exc[threadIdx.x] = exc;
}

TEST(CudaBlock, Scan256)
{
    constexpr int B = 256;
    DBuf<int>     di(B), de(B);
    k_block_scan<B><<<1, B>>>(di.p, de.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> ih(B), eh(B);
    cudaMemcpy(ih.data(), di.p, B * sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(eh.data(), de.p, B * sizeof(int), cudaMemcpyDeviceToHost);
    for (int i = 0; i < B; ++i) {
        EXPECT_EQ(ih[i], i + 1);
        EXPECT_EQ(eh[i], i);
    }
}

// ── Block broadcast ──────────────────────────────────────────────────────────

template <int B>
__global__ void k_block_broadcast(int* out)
{
    using BB = bee::cuda::BlockBroadcast<int>;
    __shared__ typename BB::TempStorage smem;

    int in           = (threadIdx.x == 77) ? 999 : 0;
    int b            = BB(smem).Broadcast(in, 77u);
    out[threadIdx.x] = b;
}

TEST(CudaBlock, Broadcast128)
{
    constexpr int B = 128;
    DBuf<int>     d(B);
    k_block_broadcast<B><<<1, B>>>(d.p);
    SKIP_IF_NO_IMAGE();
    std::vector<int> h(B);
    cudaMemcpy(h.data(), d.p, B * sizeof(int), cudaMemcpyDeviceToHost);
    for (int i = 0; i < B; ++i)
        EXPECT_EQ(h[i], 999);
}

} // namespace
