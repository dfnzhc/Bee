/**
 * @File VectorType.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/1
 * @Brief 
 */

#pragma once

#include "../Setup.hpp"

namespace bee {

/// 向量
template<i32 N, typename T>
struct vec;

// clang-format off
template<typename T> using vec1_t = vec<1, T>;
template<typename T> using vec2_t = vec<2, T>;
template<typename T> using vec3_t = vec<3, T>;
template<typename T> using vec4_t = vec<4, T>;
// clang-format on

using bool1 = vec1_t<bool>;
using bool2 = vec2_t<bool>;
using bool3 = vec3_t<bool>;
using bool4 = vec4_t<bool>;

using int1 = vec1_t<i32>;
using int2 = vec2_t<i32>;
using int3 = vec3_t<i32>;
using int4 = vec4_t<i32>;

using uint1 = vec1_t<uint>;
using uint2 = vec2_t<uint>;
using uint3 = vec3_t<uint>;
using uint4 = vec4_t<uint>;

using float1 = vec1_t<f32>;
using float2 = vec2_t<f32>;
using float3 = vec3_t<f32>;
using float4 = vec4_t<f32>;

} // namespace bee