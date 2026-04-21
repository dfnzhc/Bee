#pragma once

#include <cstddef>
#include <cstdint>

namespace bee::simd
{

// ISA 标签类型：用于编译期分派
struct IsaScalar  {};
struct IsaSse2    {};
struct IsaAvx2    {};
struct IsaAvx512  {};

// SimdBackend 主模板：仅声明不定义
// 对未特化的 (T, ISA) 组合，使用此模板会产生编译期错误
template <typename T, typename ISA>
struct SimdBackend;

} // namespace bee::simd
