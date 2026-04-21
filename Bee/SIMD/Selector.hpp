#pragma once

#include "Traits.hpp"

namespace bee::simd
{

// 根据编译宏选择默认 ISA（按最高可用优先级选择）

#if defined(BEE_TENSOR_SIMD_AVX512)
    using DefaultIsa = IsaAvx512;
#elif defined(BEE_TENSOR_SIMD_AVX2)
    using DefaultIsa = IsaAvx2;
#elif defined(BEE_TENSOR_SIMD_SSE2)
    using DefaultIsa = IsaSse2;
#else
    using DefaultIsa = IsaScalar;
#endif

// 返回当前默认 ISA 的名称字符串
constexpr auto default_isa_name() -> const char*
{
#if defined(BEE_TENSOR_SIMD_AVX512)
    return "Avx512";
#elif defined(BEE_TENSOR_SIMD_AVX2)
    return "Avx2";
#elif defined(BEE_TENSOR_SIMD_SSE2)
    return "Sse2";
#else
    return "Scalar";
#endif
}

} // namespace bee::simd
