#pragma once

#include "Traits.hpp"
#include "Backends/Scalar.hpp"
#include "Backends/Sse2.hpp"
#include "Backends/Avx2.hpp"
#include "Backends/Avx512.hpp"
#include "Detect.hpp"
#include "Stream.hpp"

#if defined(BEE_SIMD_ENABLE_SSE2) || defined(BEE_SIMD_ENABLE_AVX2) || defined(BEE_SIMD_ENABLE_AVX512)
    #include <emmintrin.h>
#endif

namespace bee::simd
{
// 提交非时序写入的存储栅栏。
//
// stream_* 系列写入会绕过常规缓存路径，处理器允许其在内存系统中延迟
// 完成。调用方在一个连续批次的非时序写入结束后调用一次 sfence()，
// 可保证这些写入在后续依赖读取或跨线程可见性检查前完成。未启用 SIMD
// 后端时该函数为空操作。
inline void sfence() noexcept
{
#if defined(BEE_SIMD_ENABLE_SSE2) || defined(BEE_SIMD_ENABLE_AVX2) || defined(BEE_SIMD_ENABLE_AVX512)
    _mm_sfence();
#endif
}
} // namespace bee::simd
