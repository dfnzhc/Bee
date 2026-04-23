/**
 * @File Cpu/Gemm/GemmCommon.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/23
 * @Brief This file is part of Bee.
 *
 * GEMM 内核共用的分块常量、对齐分配、软件预取宏。
 *
 * 约定：所有 GEMM 采用 **行主序**，行距 = 列数 N（A: [M,K] → lda=K；B: [K,N] → ldb=N；
 * C: [M,N] → ldc=N）。C 在调用前已 memset 为 0，内核只做 += 累加。
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if defined(_MSC_VER)
    #include <malloc.h>
#endif

#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
    #include <xmmintrin.h>
    #define BEE_GEMM_PREFETCH_T0(p) _mm_prefetch(reinterpret_cast<const char*>(p), _MM_HINT_T0)
    #define BEE_GEMM_PREFETCH_T1(p) _mm_prefetch(reinterpret_cast<const char*>(p), _MM_HINT_T1)
#else
    #define BEE_GEMM_PREFETCH_T0(p) ((void)(p))
    #define BEE_GEMM_PREFETCH_T1(p) ((void)(p))
#endif

namespace bee::cpu::gemm
{

// ── 分块常量 ─────────────────────────────────────────────────────────────────
struct Avx2BlockSize
{
    static constexpr int MC    = 192;
    static constexpr int KC    = 384;
    static constexpr int NC    = 2048;
    static constexpr int MR    = 8;
    static constexpr int NR_F  = 8; // SGEMM / I32 列方向 tile
    static constexpr int NR_D  = 4; // DGEMM 列方向 tile
    static constexpr int NR_I8 = 8;
};

struct Sse2BlockSize
{
    static constexpr int MC    = 128;
    static constexpr int KC    = 256;
    static constexpr int NC    = 1024;
    static constexpr int MR    = 4;
    static constexpr int NR_F  = 4;
    static constexpr int NR_D  = 2;
    static constexpr int NR_I8 = 4;
};

// ── 对齐分配 ─────────────────────────────────────────────────────────────────
inline auto aligned_malloc(std::size_t bytes, std::size_t alignment = 64) -> void*
{
#if defined(_MSC_VER)
    return _aligned_malloc(bytes, alignment);
#else
    void* p = nullptr;
    if (posix_memalign(&p, alignment, bytes) != 0)
        return nullptr;
    return p;
#endif
}

inline auto aligned_free(void* p) -> void
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}

struct AlignedBuffer
{
    void*       ptr{nullptr};
    std::size_t bytes{0};

    AlignedBuffer(std::size_t b, std::size_t alignment = 64)
        : bytes(b)
    {
        ptr = aligned_malloc(b, alignment);
    }

    ~AlignedBuffer()
    {
        if (ptr)
            aligned_free(ptr);
    }

    AlignedBuffer(const AlignedBuffer&)                    = delete;
    auto operator=(const AlignedBuffer&) -> AlignedBuffer& = delete;

    template <typename T>
    auto as() -> T*
    {
        return static_cast<T*>(ptr);
    }
};

template <typename T>
inline auto min_i(T a, T b) -> T
{
    return a < b ? a : b;
}

template <typename T>
inline auto ceil_div(T a, T b) -> T
{
    return (a + b - 1) / b;
}

} // namespace bee::cpu::gemm
