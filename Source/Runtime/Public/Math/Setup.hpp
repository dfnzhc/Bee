/**
 * @File Setup.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

#ifndef GLM_FORCE_CXX17
#  define GLM_FORCE_CXX17
#endif
#ifndef GLM_FORCE_SWIZZLE
#  define GLM_FORCE_SWIZZLE
#endif
#ifndef GLM_FORCE_RADIANS
#  define GLM_FORCE_RADIANS
#endif
#ifndef GLM_FORCE_RIGHT_HANDED
#  define GLM_FORCE_RIGHT_HANDED
#endif
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#  define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#ifndef GLM_ENABLE_EXPERIMENTAL
#  define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/hash.hpp>

#include <concepts>

namespace bee {

template<int N, typename T> using vec_t = glm::vec<N, T>;

template<typename T> using vec1_t = vec_t<1, T>;
template<typename T> using vec2_t = vec_t<2, T>;
template<typename T> using vec3_t = vec_t<3, T>;
template<typename T> using vec4_t = vec_t<4, T>;

#define DEFINE_VECTOR_ALIAS(type, suffix)                                                                                                            \
    using vec1##suffix = vec1_t<type>;                                                                                                               \
    using vec2##suffix = vec2_t<type>;                                                                                                               \
    using vec3##suffix = vec3_t<type>;                                                                                                               \
    using vec4##suffix = vec4_t<type>;

DEFINE_VECTOR_ALIAS(f32, )
DEFINE_VECTOR_ALIAS(int, i)
DEFINE_VECTOR_ALIAS(uint, u)
DEFINE_VECTOR_ALIAS(f32, f)
DEFINE_VECTOR_ALIAS(f64, d)

#undef DEFINE_VECTOR_ALIAS

#define DEFINE_VECTOR_ALIAS(type)                                                                                                                    \
    using type##1 = vec1_t<type>;                                                                                                                    \
    using type##2 = vec2_t<type>;                                                                                                                    \
    using type##3 = vec3_t<type>;                                                                                                                    \
    using type##4 = vec4_t<type>;

DEFINE_VECTOR_ALIAS(float)
DEFINE_VECTOR_ALIAS(int)
DEFINE_VECTOR_ALIAS(uint)

#undef DEFINE_VECTOR_ALIAS

// 对矩阵默认使用 **行-列** 顺序

template<int R, int C, typename T> using mat_t = glm::mat<C, R, T>;

template<typename T> using mat1x1_t = mat_t<1, 1, T>;
template<typename T> using mat1x2_t = mat_t<1, 2, T>;
template<typename T> using mat1x3_t = mat_t<1, 3, T>;
template<typename T> using mat1x4_t = mat_t<1, 4, T>;

template<typename T> using mat2x1_t = mat_t<2, 1, T>;
template<typename T> using mat2x2_t = mat_t<2, 2, T>;
template<typename T> using mat2x3_t = mat_t<2, 3, T>;
template<typename T> using mat2x4_t = mat_t<2, 4, T>;

template<typename T> using mat3x1_t = mat_t<3, 1, T>;
template<typename T> using mat3x2_t = mat_t<3, 2, T>;
template<typename T> using mat3x3_t = mat_t<3, 3, T>;
template<typename T> using mat3x4_t = mat_t<3, 4, T>;

template<typename T> using mat4x1_t = mat_t<4, 1, T>;
template<typename T> using mat4x2_t = mat_t<4, 2, T>;
template<typename T> using mat4x3_t = mat_t<4, 3, T>;
template<typename T> using mat4x4_t = mat_t<4, 4, T>;

template<typename T> using mat1_t = mat_t<1, 1, T>;
template<typename T> using mat2_t = mat_t<2, 2, T>;
template<typename T> using mat3_t = mat_t<3, 3, T>;
template<typename T> using mat4_t = mat_t<4, 4, T>;

#define DEFINE_MATRIX_ALIAS(type, suffix)                                                                                                            \
    using mat1x1##suffix = mat1x1_t<type>;                                                                                                           \
    using mat1x2##suffix = mat1x2_t<type>;                                                                                                           \
    using mat1x3##suffix = mat1x3_t<type>;                                                                                                           \
    using mat1x4##suffix = mat1x4_t<type>;                                                                                                           \
    using mat2x1##suffix = mat2x1_t<type>;                                                                                                           \
    using mat2x2##suffix = mat2x2_t<type>;                                                                                                           \
    using mat2x3##suffix = mat2x3_t<type>;                                                                                                           \
    using mat2x4##suffix = mat2x4_t<type>;                                                                                                           \
    using mat3x1##suffix = mat3x1_t<type>;                                                                                                           \
    using mat3x2##suffix = mat3x2_t<type>;                                                                                                           \
    using mat3x3##suffix = mat3x3_t<type>;                                                                                                           \
    using mat3x4##suffix = mat3x4_t<type>;                                                                                                           \
    using mat4x1##suffix = mat4x1_t<type>;                                                                                                           \
    using mat4x2##suffix = mat4x2_t<type>;                                                                                                           \
    using mat4x3##suffix = mat4x3_t<type>;                                                                                                           \
    using mat4x4##suffix = mat4x4_t<type>;

DEFINE_MATRIX_ALIAS(float, )
DEFINE_MATRIX_ALIAS(int, i)
DEFINE_MATRIX_ALIAS(uint, u)

#undef DEFINE_MATRIX_ALIAS

#define DEFINE_MATRIX_ALIAS(type)                                                                                                                    \
    using type##1x1 = mat1x1_t<type>;                                                                                                                \
    using type##1x2 = mat1x2_t<type>;                                                                                                                \
    using type##1x3 = mat1x3_t<type>;                                                                                                                \
    using type##1x4 = mat1x4_t<type>;                                                                                                                \
    using type##2x1 = mat2x1_t<type>;                                                                                                                \
    using type##2x2 = mat2x2_t<type>;                                                                                                                \
    using type##2x3 = mat2x3_t<type>;                                                                                                                \
    using type##2x4 = mat2x4_t<type>;                                                                                                                \
    using type##3x1 = mat3x1_t<type>;                                                                                                                \
    using type##3x2 = mat3x2_t<type>;                                                                                                                \
    using type##3x3 = mat3x3_t<type>;                                                                                                                \
    using type##3x4 = mat3x4_t<type>;                                                                                                                \
    using type##4x1 = mat4x1_t<type>;                                                                                                                \
    using type##4x2 = mat4x2_t<type>;                                                                                                                \
    using type##4x3 = mat4x3_t<type>;                                                                                                                \
    using type##4x4 = mat4x4_t<type>;

DEFINE_MATRIX_ALIAS(float)
DEFINE_MATRIX_ALIAS(int)
DEFINE_MATRIX_ALIAS(uint)

#undef DEFINE_MATRIX_ALIAS

using mat1 = mat1_t<f32>;
using mat2 = mat2_t<f32>;
using mat3 = mat3_t<f32>;
using mat4 = mat4_t<f32>;

#define BEE_INLINE    GLM_INLINE
#define BEE_FUNC      GLM_FUNC_QUALIFIER
#define BEE_CONSTEXPR GLM_CONSTEXPR

// clang-format off
template<typename T> concept BoolType = std::is_same_v<bool, T>;
template<typename T> concept U32Type  = std::is_same_v<u32, T>;
template<typename T> concept U64Type  = std::is_same_v<u64, T>;
template<typename T> concept F32Type  = std::is_same_v<f32, T>;
template<typename T> concept F64Type  = std::is_same_v<f64, T>;

template<typename T> concept SignedType     = std::is_signed_v<T>;
template<typename T> concept UnsignedType   = std::is_unsigned_v<T>;
template<typename T> concept IntegralType   = std::is_integral_v<T>;
template<typename T> concept FloatType      = std::is_floating_point_v<T>;
template<typename T> concept ArithmeticType = std::is_arithmetic_v<T>;
// clang-format on

template<typename T, typename U>
concept Convertible = (not std::is_same_v<T, U>) and std::is_convertible_v<T, U> and std::is_convertible_v<U, T>;

template<typename T, typename U>
concept BothIntegral = IntegralType<T> and IntegralType<U>;

template<class> inline constexpr bool kAlwaysFalse = false;

} // namespace bee