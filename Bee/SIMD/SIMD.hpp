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
// 在 NT(non-temporal) 流式写之后，保证对其它核的可见性；
// 仅对使用了 stream_* 的路径需要调用一次。
inline void sfence() noexcept
{
#if defined(BEE_SIMD_ENABLE_SSE2) || defined(BEE_SIMD_ENABLE_AVX2) || defined(BEE_SIMD_ENABLE_AVX512)
    _mm_sfence();
#endif
}
} // namespace bee::simd
