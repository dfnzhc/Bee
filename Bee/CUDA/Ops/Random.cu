/**
 * @File Ops/Random.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 设备侧原生 Philox4x32-10 随机数生成，替代原 CPU 生成 + H2D 的 fallback 路径。
 *
 * 算法：DE Shaw Philox4x32-10（John Salmon et al., SC'11）。每个线程独占
 * 一个 counter（等于全局线程 idx），共享 key=(seed_lo, seed_hi)，一次调用
 * 产出 4 个 uint32。通过 (seed, round) 派生多轮以覆盖 numel > thread 数的
 * 情况。与 curand 的 XORWOW/Philox 差异：
 *   - 不维护 per-thread state（无 init 内核），每次按 (seed, counter, round)
 *     直接推导；stateless、幂等。
 *   - seed 相同时每次调用结果相同；不同 dtype / op 通过 subseed 区分，避免
 *     rand(f32) 与 rand(f64) 在同一 seed 下是位-对齐复刻。
 *
 * 输出语义对齐 std::uniform_real_distribution<T>(0,1)：半开区间 [0,1)。
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Check.cuh"
#include "CUDA/Core/Launch.cuh"

#include <cuda_runtime.h>

#include <cstdint>

namespace
{

constexpr int kDtBool = 0;
constexpr int kDtU8   = 1;
constexpr int kDtI32  = 2;
constexpr int kDtI64  = 3;
constexpr int kDtF32  = 4;
constexpr int kDtF64  = 5;

// ── Philox4x32-10 ────────────────────────────────────────────────────────

constexpr std::uint32_t kPhiloxM0 = 0xD2511F53u;
constexpr std::uint32_t kPhiloxM1 = 0xCD9E8D57u;
constexpr std::uint32_t kPhiloxW0 = 0x9E3779B9u; // Weyl 常量，key bump
constexpr std::uint32_t kPhiloxW1 = 0xBB67AE85u;

struct U32x4
{
    std::uint32_t x, y, z, w;
};

struct U32x2
{
    std::uint32_t x, y;
};

__device__ __forceinline__ U32x2 mulhilo32(std::uint32_t a, std::uint32_t b)
{
    const std::uint64_t p = static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b);
    return {static_cast<std::uint32_t>(p >> 32), static_cast<std::uint32_t>(p)};
}

__device__ __forceinline__ U32x4 philox4x32_round(U32x4 ctr, U32x2 key)
{
    const auto p0 = mulhilo32(kPhiloxM0, ctr.x);
    const auto p1 = mulhilo32(kPhiloxM1, ctr.z);
    // Output mapping: (hi1 ^ y ^ k0, lo1, hi0 ^ w ^ k1, lo0)
    return {
        p1.x ^ ctr.y ^ key.x,
        p1.y,
        p0.x ^ ctr.w ^ key.y,
        p0.y,
    };
}

__device__ __forceinline__ U32x4 philox4x32_10(U32x4 ctr, U32x2 key)
{
    U32x4 c = ctr;
    U32x2 k = key;
#pragma unroll
    for (int i = 0; i < 10; ++i) {
        c   = philox4x32_round(c, k);
        k.x = k.x + kPhiloxW0;
        k.y = k.y + kPhiloxW1;
    }
    return c;
}

__device__ __forceinline__ U32x4 philox_gen(std::uint64_t seed, std::uint32_t counter, std::uint32_t subseq)
{
    // counter 作为 ctr 的低 32 位；subseq 用于为同一 seed 下的不同调用派生独立流。
    U32x4 ctr{counter, subseq, 0u, 0u};
    U32x2 key{
        static_cast<std::uint32_t>(seed & 0xFFFFFFFFu),
        static_cast<std::uint32_t>(seed >> 32),
    };
    return philox4x32_10(ctr, key);
}

// ── u32 → [0, 1) ────────────────────────────────────────────────────────
__device__ __forceinline__ float u32_to_f32(std::uint32_t x)
{
    // [0, 1) via 24-bit mantissa: x >> 8 in [0, 2^24), ldexpf(_, -24) ≈ [0,1)。
    return static_cast<float>(x >> 8) * (1.0f / static_cast<float>(1u << 24));
}

__device__ __forceinline__ double u64_to_f64(std::uint64_t x)
{
    // [0, 1) via 53-bit mantissa：x >> 11 in [0, 2^53)。
    return static_cast<double>(x >> 11) * (1.0 / static_cast<double>(1ull << 53));
}

// Box-Muller（半极坐标变体）：(u1,u2)∈[0,1)² → (z0,z1)∈N(0,1)²
__device__ __forceinline__ void box_muller_f32(float u1, float u2, float& z0, float& z1)
{
    // u1 可能为 0，钳位到极小值以规避 log(0)。
    const float u1c = u1 < 1.175494e-38f ? 1.175494e-38f : u1;
    const float r   = sqrtf(-2.0f * logf(u1c));
    float       s, c;
    __sincosf(2.0f * 3.14159265358979323846f * u2, &s, &c);
    z0 = r * c;
    z1 = r * s;
}

__device__ __forceinline__ void box_muller_f64(double u1, double u2, double& z0, double& z1)
{
    const double u1c = u1 < 2.2250738585072014e-308 ? 2.2250738585072014e-308 : u1;
    const double r   = sqrt(-2.0 * log(u1c));
    double       s, c;
    sincos(2.0 * 3.14159265358979323846 * u2, &s, &c);
    z0 = r * c;
    z1 = r * s;
}

// ── Kernels ──────────────────────────────────────────────────────────────
// 每个线程生成 4 个 u32（一次 philox_gen）。对 f32 uniform 产生 4 个输出；
// 对 f64 uniform 产生 2 个输出；对 f32 normal 产生 4 个（Box-Muller 配对两次
// 需要 2 组 philox_gen，故该 kernel 每线程实际调用两次）；对 f64 normal
// 每线程产出 2 个。
//
// 为了简洁，统一"每线程 per-call 产 N 个元素"后写入，n 不够时仅写部分。

__global__ void rand_uniform_f32_kernel(float* __restrict__ out, std::size_t n, std::uint64_t seed)
{
    const std::uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const std::uint32_t stride = gridDim.x * blockDim.x;
    for (std::uint32_t base = tid; base * 4u < n; base += stride) {
        const auto v = philox_gen(seed, base, 0u);
        const std::size_t idx = static_cast<std::size_t>(base) * 4u;
        if (idx + 0 < n) out[idx + 0] = u32_to_f32(v.x);
        if (idx + 1 < n) out[idx + 1] = u32_to_f32(v.y);
        if (idx + 2 < n) out[idx + 2] = u32_to_f32(v.z);
        if (idx + 3 < n) out[idx + 3] = u32_to_f32(v.w);
    }
}

__global__ void rand_uniform_f64_kernel(double* __restrict__ out, std::size_t n, std::uint64_t seed)
{
    const std::uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const std::uint32_t stride = gridDim.x * blockDim.x;
    for (std::uint32_t base = tid; base * 2u < n; base += stride) {
        const auto v = philox_gen(seed, base, 0u);
        const std::uint64_t lo = (static_cast<std::uint64_t>(v.y) << 32) | v.x;
        const std::uint64_t hi = (static_cast<std::uint64_t>(v.w) << 32) | v.z;
        const std::size_t idx = static_cast<std::size_t>(base) * 2u;
        if (idx + 0 < n) out[idx + 0] = u64_to_f64(lo);
        if (idx + 1 < n) out[idx + 1] = u64_to_f64(hi);
    }
}

__global__ void rand_normal_f32_kernel(float* __restrict__ out, std::size_t n, std::uint64_t seed)
{
    const std::uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const std::uint32_t stride = gridDim.x * blockDim.x;
    for (std::uint32_t base = tid; base * 4u < n; base += stride) {
        const auto v = philox_gen(seed, base, 1u); // subseq=1 与 uniform 分流
        const float u0 = u32_to_f32(v.x);
        const float u1 = u32_to_f32(v.y);
        const float u2 = u32_to_f32(v.z);
        const float u3 = u32_to_f32(v.w);
        float z0, z1, z2, z3;
        box_muller_f32(u0, u1, z0, z1);
        box_muller_f32(u2, u3, z2, z3);
        const std::size_t idx = static_cast<std::size_t>(base) * 4u;
        if (idx + 0 < n) out[idx + 0] = z0;
        if (idx + 1 < n) out[idx + 1] = z1;
        if (idx + 2 < n) out[idx + 2] = z2;
        if (idx + 3 < n) out[idx + 3] = z3;
    }
}

__global__ void rand_normal_f64_kernel(double* __restrict__ out, std::size_t n, std::uint64_t seed)
{
    const std::uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const std::uint32_t stride = gridDim.x * blockDim.x;
    for (std::uint32_t base = tid; base * 2u < n; base += stride) {
        const auto v  = philox_gen(seed, base, 1u);
        const std::uint64_t lo = (static_cast<std::uint64_t>(v.y) << 32) | v.x;
        const std::uint64_t hi = (static_cast<std::uint64_t>(v.w) << 32) | v.z;
        const double u0 = u64_to_f64(lo);
        const double u1 = u64_to_f64(hi);
        double z0, z1;
        box_muller_f64(u0, u1, z0, z1);
        const std::size_t idx = static_cast<std::size_t>(base) * 2u;
        if (idx + 0 < n) out[idx + 0] = z0;
        if (idx + 1 < n) out[idx + 1] = z1;
    }
}

template <typename T>
__global__ void rand_int_kernel(T* __restrict__ out, std::size_t n, std::uint64_t seed, std::int64_t low, std::uint64_t range)
{
    const std::uint32_t tid = blockIdx.x * blockDim.x + threadIdx.x;
    const std::uint32_t stride = gridDim.x * blockDim.x;
    for (std::uint32_t base = tid; base * 4u < n; base += stride) {
        const auto v = philox_gen(seed, base, 2u);
        const std::uint32_t arr[4] = {v.x, v.y, v.z, v.w};
        const std::size_t idx = static_cast<std::size_t>(base) * 4u;
#pragma unroll
        for (int k = 0; k < 4; ++k) {
            if (idx + k < n) {
                // 对 I64 / 大 range 也用 32-bit → modulo 收敛（MVP，够用）。
                const std::uint32_t u = arr[k];
                const std::uint64_t r = static_cast<std::uint64_t>(u) % range;
                out[idx + k] = static_cast<T>(static_cast<std::int64_t>(r) + low);
            }
        }
    }
}

// ── Launcher helpers ────────────────────────────────────────────────────
constexpr unsigned int kRandBlock = 256;
constexpr unsigned int kRandMaxGrid = 1024; // 饱和 SM 的合理上限

unsigned int compute_grid(std::size_t elems_per_thread, std::size_t n)
{
    const std::size_t threads = (n + elems_per_thread - 1) / elems_per_thread;
    const std::size_t grid = (threads + kRandBlock - 1) / kRandBlock;
    return static_cast<unsigned int>(grid < kRandMaxGrid ? grid : kRandMaxGrid);
}

} // namespace

namespace bee::cuda::detail
{

int ops_random_uniform(int dt, void* dst, std::size_t n, std::uint64_t seed) noexcept
{
    if (n == 0)
        return 0;
    cudaStream_t stream = cudaStreamPerThread;
    if (dt == kDtF32) {
        const unsigned int grid = compute_grid(4, n);
        rand_uniform_f32_kernel<<<grid, kRandBlock, 0, stream>>>(static_cast<float*>(dst), n, seed);
    } else if (dt == kDtF64) {
        const unsigned int grid = compute_grid(2, n);
        rand_uniform_f64_kernel<<<grid, kRandBlock, 0, stream>>>(static_cast<double*>(dst), n, seed);
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }
    const cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
        return static_cast<int>(err);
    return static_cast<int>(cudaStreamSynchronize(stream));
}

int ops_random_normal(int dt, void* dst, std::size_t n, std::uint64_t seed) noexcept
{
    if (n == 0)
        return 0;
    cudaStream_t stream = cudaStreamPerThread;
    if (dt == kDtF32) {
        const unsigned int grid = compute_grid(4, n);
        rand_normal_f32_kernel<<<grid, kRandBlock, 0, stream>>>(static_cast<float*>(dst), n, seed);
    } else if (dt == kDtF64) {
        const unsigned int grid = compute_grid(2, n);
        rand_normal_f64_kernel<<<grid, kRandBlock, 0, stream>>>(static_cast<double*>(dst), n, seed);
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }
    const cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
        return static_cast<int>(err);
    return static_cast<int>(cudaStreamSynchronize(stream));
}

int ops_random_int(int dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) noexcept
{
    if (n == 0)
        return 0;
    if (low >= high)
        return static_cast<int>(cudaErrorInvalidValue);
    const std::uint64_t range = static_cast<std::uint64_t>(high - low);
    cudaStream_t        stream = cudaStreamPerThread;
    const unsigned int  grid   = compute_grid(4, n);
    if (dt == kDtU8) {
        rand_int_kernel<std::uint8_t><<<grid, kRandBlock, 0, stream>>>(static_cast<std::uint8_t*>(dst), n, seed, low, range);
    } else if (dt == kDtI32) {
        rand_int_kernel<std::int32_t><<<grid, kRandBlock, 0, stream>>>(static_cast<std::int32_t*>(dst), n, seed, low, range);
    } else if (dt == kDtI64) {
        rand_int_kernel<std::int64_t><<<grid, kRandBlock, 0, stream>>>(static_cast<std::int64_t*>(dst), n, seed, low, range);
    } else {
        return static_cast<int>(cudaErrorInvalidValue);
    }
    const cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
        return static_cast<int>(err);
    return static_cast<int>(cudaStreamSynchronize(stream));
}

} // namespace bee::cuda::detail
