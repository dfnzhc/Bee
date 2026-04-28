#pragma once

#include "SIMD/Traits.hpp"

#ifdef BEE_SIMD_ENABLE_AVX512

    #include <immintrin.h>

    #include <cmath>
    #include <cstddef>
    #include <cstdint>

namespace bee::simd
{

// AVX-512 后端提供 512-bit 寄存器宽度。当前特化要求 AVX512F；uint8_t
// 额外要求 AVX512BW。CMake 与运行时探测会同时检查 F/BW，避免在只支持
// 部分 AVX-512 扩展的机器上实例化字节后端。

// -----------------------------------------------------------------------
// float × AVX-512F：__m512，宽度 = 16
// -----------------------------------------------------------------------
template <>
struct SimdBackend<float, IsaAvx512>
{
    static constexpr std::size_t width = 16;
    using reg                          = __m512;

    // clang-format off
    static auto load(const float* p)  -> reg  { return _mm512_load_ps(p); }
    static auto loadu(const float* p) -> reg  { return _mm512_loadu_ps(p); }
    static auto store(float* p, reg v)  -> void { _mm512_store_ps(p, v); }
    static auto storeu(float* p, reg v) -> void { _mm512_storeu_ps(p, v); }
    static auto set1(float x) -> reg { return _mm512_set1_ps(x); }

    static auto add(reg a, reg b) -> reg { return _mm512_add_ps(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm512_sub_ps(a, b); }
    static auto mul(reg a, reg b) -> reg { return _mm512_mul_ps(a, b); }
    static auto div(reg a, reg b) -> reg { return _mm512_div_ps(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm512_min_ps(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm512_max_ps(a, b); }
    // clang-format on

    // 翻转符号位：通过整数 xor 清除符号 bit
    static auto neg(reg a) -> reg
    {
        __m512i sign_bit = _mm512_set1_epi32(static_cast<int>(0x80000000U));
        return _mm512_castsi512_ps(_mm512_xor_si512(_mm512_castps_si512(a), sign_bit));
    }

    // 绝对值：AVX-512F 原生指令
    static auto abs(reg a) -> reg
    {
        return _mm512_abs_ps(a);
    }

    static auto sqrt(reg a) -> reg
    {
        return _mm512_sqrt_ps(a);
    }

    // exp 回退标量逐元素计算。AVX-512F 不提供标准指数指令，当前实现以
    // std::exp 语义为准，性能敏感调用点应考虑专门向量数学实现。
    static auto exp(reg v) -> reg
    {
        alignas(64) float buf[16];
        _mm512_store_ps(buf, v);
        for (int i = 0; i < 16; ++i)
            buf[i] = std::exp(buf[i]);
        return _mm512_load_ps(buf);
    }

    // log 回退标量逐元素计算，边界行为与 std::log 保持一致。
    static auto log(reg v) -> reg
    {
        alignas(64) float buf[16];
        _mm512_store_ps(buf, v);
        for (int i = 0; i < 16; ++i)
            buf[i] = std::log(buf[i]);
        return _mm512_load_ps(buf);
    }

    // 水平求和（AVX-512F 软件内置函数）
    static auto reduce_sum(reg v) -> float
    {
        return _mm512_reduce_add_ps(v);
    }

    // 水平求最小值
    static auto reduce_min(reg v) -> float
    {
        return _mm512_reduce_min_ps(v);
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> float
    {
        return _mm512_reduce_max_ps(v);
    }
};

// -----------------------------------------------------------------------
// double × AVX-512F：__m512d，宽度 = 8
// -----------------------------------------------------------------------
template <>
struct SimdBackend<double, IsaAvx512>
{
    static constexpr std::size_t width = 8;
    using reg                          = __m512d;

    // clang-format off
    static auto load(const double* p)  -> reg  { return _mm512_load_pd(p); }
    static auto loadu(const double* p) -> reg  { return _mm512_loadu_pd(p); }
    static auto store(double* p, reg v)  -> void { _mm512_store_pd(p, v); }
    static auto storeu(double* p, reg v) -> void { _mm512_storeu_pd(p, v); }
    static auto set1(double x) -> reg { return _mm512_set1_pd(x); }

    static auto add(reg a, reg b) -> reg { return _mm512_add_pd(a, b); }
    static auto sub(reg a, reg b) -> reg { return _mm512_sub_pd(a, b); }
    static auto mul(reg a, reg b) -> reg { return _mm512_mul_pd(a, b); }
    static auto div(reg a, reg b) -> reg { return _mm512_div_pd(a, b); }

    static auto min(reg a, reg b) -> reg { return _mm512_min_pd(a, b); }
    static auto max(reg a, reg b) -> reg { return _mm512_max_pd(a, b); }
    // clang-format on

    static auto neg(reg a) -> reg
    {
        __m512i sign_bit = _mm512_set1_epi64(static_cast<long long>(0x8000000000000000ULL));
        return _mm512_castsi512_pd(_mm512_xor_si512(_mm512_castpd_si512(a), sign_bit));
    }

    static auto abs(reg a) -> reg
    {
        return _mm512_abs_pd(a);
    }

    static auto sqrt(reg a) -> reg
    {
        return _mm512_sqrt_pd(a);
    }

    static auto exp(reg v) -> reg
    {
        alignas(64) double buf[8];
        _mm512_store_pd(buf, v);
        for (int i = 0; i < 8; ++i)
            buf[i] = std::exp(buf[i]);
        return _mm512_load_pd(buf);
    }

    static auto log(reg v) -> reg
    {
        alignas(64) double buf[8];
        _mm512_store_pd(buf, v);
        for (int i = 0; i < 8; ++i)
            buf[i] = std::log(buf[i]);
        return _mm512_load_pd(buf);
    }

    static auto reduce_sum(reg v) -> double
    {
        return _mm512_reduce_add_pd(v);
    }
    static auto reduce_min(reg v) -> double
    {
        return _mm512_reduce_min_pd(v);
    }
    static auto reduce_max(reg v) -> double
    {
        return _mm512_reduce_max_pd(v);
    }
};

// -----------------------------------------------------------------------
// int32_t × AVX-512F：__m512i，宽度 = 16
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int32_t, IsaAvx512>
{
    static constexpr std::size_t width = 16;
    using reg                          = __m512i;

    static auto load(const int32_t* p) -> reg
    {
        return _mm512_load_si512(reinterpret_cast<const __m512i*>(p));
    }
    static auto loadu(const int32_t* p) -> reg
    {
        return _mm512_loadu_si512(reinterpret_cast<const __m512i*>(p));
    }
    static auto store(int32_t* p, reg v) -> void
    {
        _mm512_store_si512(reinterpret_cast<__m512i*>(p), v);
    }
    static auto storeu(int32_t* p, reg v) -> void
    {
        _mm512_storeu_si512(reinterpret_cast<__m512i*>(p), v);
    }
    static auto set1(int32_t x) -> reg
    {
        return _mm512_set1_epi32(x);
    }

    static auto add(reg a, reg b) -> reg
    {
        return _mm512_add_epi32(a, b);
    }
    static auto sub(reg a, reg b) -> reg
    {
        return _mm512_sub_epi32(a, b);
    }

    // AVX-512F 原生 i32 乘法（低 32 位）
    static auto mul(reg a, reg b) -> reg
    {
        return _mm512_mullo_epi32(a, b);
    }

    static auto min(reg a, reg b) -> reg
    {
        return _mm512_min_epi32(a, b);
    }
    static auto max(reg a, reg b) -> reg
    {
        return _mm512_max_epi32(a, b);
    }

    // 取反：0 - v
    static auto neg(reg a) -> reg
    {
        return _mm512_sub_epi32(_mm512_setzero_si512(), a);
    }

    // AVX-512F 原生绝对值
    static auto abs(reg a) -> reg
    {
        return _mm512_abs_epi32(a);
    }

    // 水平求和：16 → 8 → 4 → 1（借助 AVX2 中间层）
    static auto reduce_sum(reg v) -> int32_t
    {
        __m256i lo   = _mm512_castsi512_si256(v);
        __m256i hi   = _mm512_extracti64x4_epi64(v, 1);
        __m256i s    = _mm256_add_epi32(lo, hi);
        __m128i s4lo = _mm256_castsi256_si128(s);
        __m128i s4hi = _mm256_extracti128_si256(s, 1);
        __m128i s4   = _mm_add_epi32(s4lo, s4hi);
        s4           = _mm_add_epi32(s4, _mm_srli_si128(s4, 8));
        s4           = _mm_add_epi32(s4, _mm_srli_si128(s4, 4));
        return _mm_cvtsi128_si32(s4);
    }

    // 水平求最小值（借助 AVX2 _mm256_min_epi32）
    static auto reduce_min(reg v) -> int32_t
    {
        __m256i lo   = _mm512_castsi512_si256(v);
        __m256i hi   = _mm512_extracti64x4_epi64(v, 1);
        __m256i mn   = _mm256_min_epi32(lo, hi);
        __m128i m4lo = _mm256_castsi256_si128(mn);
        __m128i m4hi = _mm256_extracti128_si256(mn, 1);
        __m128i m4   = _mm_min_epi32(m4lo, m4hi);
        m4           = _mm_min_epi32(m4, _mm_srli_si128(m4, 8));
        m4           = _mm_min_epi32(m4, _mm_srli_si128(m4, 4));
        return _mm_cvtsi128_si32(m4);
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> int32_t
    {
        __m256i lo   = _mm512_castsi512_si256(v);
        __m256i hi   = _mm512_extracti64x4_epi64(v, 1);
        __m256i mx   = _mm256_max_epi32(lo, hi);
        __m128i m4lo = _mm256_castsi256_si128(mx);
        __m128i m4hi = _mm256_extracti128_si256(mx, 1);
        __m128i m4   = _mm_max_epi32(m4lo, m4hi);
        m4           = _mm_max_epi32(m4, _mm_srli_si128(m4, 8));
        m4           = _mm_max_epi32(m4, _mm_srli_si128(m4, 4));
        return _mm_cvtsi128_si32(m4);
    }
};

// -----------------------------------------------------------------------
// int64_t × AVX-512F：__m512i，宽度 = 8
// AVX-512F 原生支持 i64 min/max/abs，比 AVX2 完整
// -----------------------------------------------------------------------
template <>
struct SimdBackend<int64_t, IsaAvx512>
{
    static constexpr std::size_t width = 8;
    using reg                          = __m512i;

    static auto load(const int64_t* p) -> reg
    {
        return _mm512_load_si512(reinterpret_cast<const __m512i*>(p));
    }
    static auto loadu(const int64_t* p) -> reg
    {
        return _mm512_loadu_si512(reinterpret_cast<const __m512i*>(p));
    }
    static auto store(int64_t* p, reg v) -> void
    {
        _mm512_store_si512(reinterpret_cast<__m512i*>(p), v);
    }
    static auto storeu(int64_t* p, reg v) -> void
    {
        _mm512_storeu_si512(reinterpret_cast<__m512i*>(p), v);
    }
    static auto set1(int64_t x) -> reg
    {
        return _mm512_set1_epi64(x);
    }

    static auto add(reg a, reg b) -> reg
    {
        return _mm512_add_epi64(a, b);
    }
    static auto sub(reg a, reg b) -> reg
    {
        return _mm512_sub_epi64(a, b);
    }

    // AVX-512F 原生 i64 min/max（AVX2 不支持）
    static auto min(reg a, reg b) -> reg
    {
        return _mm512_min_epi64(a, b);
    }
    static auto max(reg a, reg b) -> reg
    {
        return _mm512_max_epi64(a, b);
    }

    // 取反：0 - v
    static auto neg(reg a) -> reg
    {
        return _mm512_sub_epi64(_mm512_setzero_si512(), a);
    }

    // AVX-512F 原生 i64 绝对值（AVX2 不支持）
    static auto abs(reg a) -> reg
    {
        return _mm512_abs_epi64(a);
    }

    // 水平求和：回退到标量提取。这里优先保持实现简单和跨编译器稳定；
    // 若未来需要更高吞吐量，可改为 shuffle/reduce 指令组合。
    static auto reduce_sum(reg v) -> int64_t
    {
        alignas(64) int64_t buf[8];
        _mm512_store_si512(reinterpret_cast<__m512i*>(buf), v);
        return buf[0] + buf[1] + buf[2] + buf[3] + buf[4] + buf[5] + buf[6] + buf[7];
    }

    // 水平求最小值：当前通过落到栈上再标量折叠实现，语义稳定但不是
    // 最优性能路径。
    static auto reduce_min(reg v) -> int64_t
    {
        alignas(64) int64_t buf[8];
        _mm512_store_si512(reinterpret_cast<__m512i*>(buf), v);
        int64_t r = buf[0];
        for (int i = 1; i < 8; ++i)
            r = r < buf[i] ? r : buf[i];
        return r;
    }

    // 水平求最大值
    static auto reduce_max(reg v) -> int64_t
    {
        alignas(64) int64_t buf[8];
        _mm512_store_si512(reinterpret_cast<__m512i*>(buf), v);
        int64_t r = buf[0];
        for (int i = 1; i < 8; ++i)
            r = r > buf[i] ? r : buf[i];
        return r;
    }
};

    // -----------------------------------------------------------------------
    // uint8_t × AVX-512BW：__m512i，宽度 = 64
    // 仅在支持 AVX512BW 时启用（MSVC /arch:AVX512 默认开启；GCC/Clang 需 -mavx512bw）
    // -----------------------------------------------------------------------
    #ifdef __AVX512BW__
template <>
struct SimdBackend<uint8_t, IsaAvx512>
{
    static constexpr std::size_t width = 64;
    using reg                          = __m512i;

    static auto load(const uint8_t* p) -> reg
    {
        return _mm512_load_si512(reinterpret_cast<const __m512i*>(p));
    }
    static auto loadu(const uint8_t* p) -> reg
    {
        return _mm512_loadu_si512(reinterpret_cast<const __m512i*>(p));
    }
    static auto store(uint8_t* p, reg v) -> void
    {
        _mm512_store_si512(reinterpret_cast<__m512i*>(p), v);
    }
    static auto storeu(uint8_t* p, reg v) -> void
    {
        _mm512_storeu_si512(reinterpret_cast<__m512i*>(p), v);
    }
    static auto set1(uint8_t x) -> reg
    {
        return _mm512_set1_epi8(static_cast<char>(x));
    }

    // 字节加法（环绕）
    static auto add(reg a, reg b) -> reg
    {
        return _mm512_add_epi8(a, b);
    }
    static auto sub(reg a, reg b) -> reg
    {
        return _mm512_sub_epi8(a, b);
    }

    // AVX512BW 无符号字节比较
    static auto min(reg a, reg b) -> reg
    {
        return _mm512_min_epu8(a, b);
    }
    static auto max(reg a, reg b) -> reg
    {
        return _mm512_max_epu8(a, b);
    }

    // 水平求和：_mm512_sad_epu8 返回 8 个 64-bit SAD 值，累加低 16 位。
    // 返回值保持 uint8_t，超过 255 的总和会按接口类型截断。
    static auto reduce_sum(reg v) -> uint8_t
    {
        __m512i              zero = _mm512_setzero_si512();
        __m512i              sad  = _mm512_sad_epu8(v, zero);
        alignas(64) uint64_t buf[8];
        _mm512_store_si512(reinterpret_cast<__m512i*>(buf), sad);
        uint32_t total = 0;
        for (int i = 0; i < 8; ++i)
            total += static_cast<uint32_t>(buf[i] & 0xFFFF);
        return static_cast<uint8_t>(total);
    }
};
    #endif // __AVX512BW__

} // namespace bee::simd

#endif // BEE_SIMD_ENABLE_AVX512
