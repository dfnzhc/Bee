#pragma once

#include "SimdTraits.hpp"

namespace bee::simd
{

// 根据编译宏选择默认 ISA
// 本任务仅实现 Scalar 与 AVX2 后端：
//   - 检测到 AVX2 → 使用 IsaAvx2
//   - 其他情况（含仅 SSE2 或仅 AVX512）→ 降级为 IsaScalar，避免因无后端特化引发编译错误

#if defined(BEE_TENSOR_SIMD_AVX2)
    using DefaultIsa = IsaAvx2;
#else
    using DefaultIsa = IsaScalar;
#endif

// 返回当前默认 ISA 的名称字符串
constexpr auto default_isa_name() -> const char*
{
#if defined(BEE_TENSOR_SIMD_AVX2)
    return "Avx2";
#else
    return "Scalar";
#endif
}

} // namespace bee::simd
