/**
 * @File VectorCommon.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/1
 * @Brief 
 */

#pragma once

#include "./Vector1.hpp"
#include "./Vector2.hpp"
#include "./Vector3.hpp"
#include "./Vector4.hpp"

#include "../Bit.hpp"
#include "../Common.hpp"
#include "../FloatingPoint.hpp"

namespace bee {

// -------------------------
// 基础创建方法
// -------------------------

template<typename... Args>
    requires(sizeof...(Args) >= 1 and sizeof...(Args) <= 4)
auto make_vec(Args... args) -> vec<sizeof...(Args), std::common_type_t<Args...>>
{
    return vec<sizeof...(Args), std::common_type_t<Args...>>(args...);
}

// -------------------------
// 布尔类型向量操作
// -------------------------
// clang-format off
BEE_FUNC constexpr bool any(const bool1 v) noexcept { return v.x; }
BEE_FUNC constexpr bool any(const bool2 v) noexcept { return v.x || v.y; }
BEE_FUNC constexpr bool any(const bool3 v) noexcept { return v.x || v.y || v.z; }
BEE_FUNC constexpr bool any(const bool4 v) noexcept { return v.x || v.y || v.z || v.w; }

BEE_FUNC constexpr bool all(const bool1 v) noexcept { return v.x; }
BEE_FUNC constexpr bool all(const bool2 v) noexcept { return v.x && v.y; }
BEE_FUNC constexpr bool all(const bool3 v) noexcept { return v.x && v.y && v.z; }
BEE_FUNC constexpr bool all(const bool4 v) noexcept { return v.x && v.y && v.z && v.w; }

BEE_FUNC constexpr bool none(const bool1 v) noexcept { return !any(v); }
BEE_FUNC constexpr bool none(const bool2 v) noexcept { return !any(v); }
BEE_FUNC constexpr bool none(const bool3 v) noexcept { return !any(v); }
BEE_FUNC constexpr bool none(const bool4 v) noexcept { return !any(v); }

BEE_FUNC constexpr bool1 negative(const bool1 v) noexcept { return bool1{not v.x}; }
BEE_FUNC constexpr bool2 negative(const bool2 v) noexcept { return bool2{not v.x, not v.y}; }
BEE_FUNC constexpr bool3 negative(const bool3 v) noexcept { return bool3{not v.x, not v.y, not v.z}; }
BEE_FUNC constexpr bool4 negative(const bool4 v) noexcept { return bool4{not v.x, not v.y, not v.z, not v.w}; }

// clang-format on

// -------------------------
// 关系比较
// -------------------------

#define DEFINE_VECTOR_COMPARE_OP(func_name, op, logic_op)                                                              \
    template<i32 L, ArithmeticType T>                                                                                  \
    BEE_FUNC constexpr bool func_name(const vec<L, T>& lhs, const vec<L, T>& rhs)                                      \
    {                                                                                                                  \
        if constexpr (L == 1)                                                                                          \
            return lhs.x op rhs.x;                                                                                     \
        else if constexpr (L == 2)                                                                                     \
            return (lhs.x op rhs.x)logic_op(lhs.y op rhs.y);                                                           \
        else if constexpr (L == 3)                                                                                     \
            return (lhs.x op rhs.x)logic_op(lhs.y op rhs.y) logic_op(lhs.z op rhs.z);                                  \
        else if constexpr (L == 4)                                                                                     \
            return (lhs.x op rhs.x)logic_op(lhs.y op rhs.y) logic_op(lhs.z op rhs.z) logic_op(lhs.w op rhs.w);         \
    }

DEFINE_VECTOR_COMPARE_OP(any_gt, >, or)
DEFINE_VECTOR_COMPARE_OP(any_ge, >=, or)
DEFINE_VECTOR_COMPARE_OP(any_lt, <, or)
DEFINE_VECTOR_COMPARE_OP(any_le, <=, or)
DEFINE_VECTOR_COMPARE_OP(any_eq, ==, or)

DEFINE_VECTOR_COMPARE_OP(all_gt, >, and)
DEFINE_VECTOR_COMPARE_OP(all_ge, >=, and)
DEFINE_VECTOR_COMPARE_OP(all_lt, <, and)
DEFINE_VECTOR_COMPARE_OP(all_le, <=, and)
DEFINE_VECTOR_COMPARE_OP(all_eq, ==, and)
#undef DEFINE_VECTOR_COMPARE_OP

/**
 * @brief Compares two vectors component-wise and returns a boolean vector.
 * 
 * @tparam L The length of the vectors to compare.
 * @tparam T The type of the vector components.
 * @param x The first vector to compare.
 * @param y The second vector to compare.
 * @return A vector of booleans indicating the equality of corresponding components.
 */
template<i32 L, typename T>
BEE_FUNC constexpr vec<L, bool> cwEqual(const vec<L, T>& x, const vec<L, T>& y)
{
    if constexpr (L == 1)
        return bool1{x.x == y.x};
    else if constexpr (L == 2)
        return bool2{x.x == y.x, x.y == y.y};
    else if constexpr (L == 3)
        return bool3{x.x == y.x, x.y == y.y, x.z == y.z};
    else if constexpr (L == 4)
        return bool4{x.x == y.x, x.y == y.y, x.z == y.z, x.w == y.w};
}

/**
 * @brief Compares two vectors component-wise with a tolerance for floating-point errors.
 * 
 * @tparam L The length of the vectors to compare.
 * @tparam T The type of the vector components, expected to support subtraction and absolute value operations.
 * @param x The first vector to compare.
 * @param y The second vector to compare.
 * @param epsilon The tolerance value for comparison.
 * @return A vector of booleans indicating the approximate equality of corresponding components.
 */
template<i32 L, typename T>
BEE_FUNC constexpr vec<L, bool> cwEqual(const vec<L, T>& x, const vec<L, T>& y, T epsilon)
{
    vec<L, bool> ret;
    for (int i = 0; i < L; ++i)
        ret[i] = Abs(x[i] - y[i]) <= epsilon;
    return ret;
}

/**
 * @brief Compares two vectors component-wise with a tolerance for floating-point errors.
 * 
 * @tparam L The length of the vectors to compare.
 * @tparam T The type of the vector components, expected to support subtraction and absolute value operations.
 * @param x The first vector to compare.
 * @param y The second vector to compare.
 * @param epsilon The tolerance vector value for comparison.
 * @return A vector of booleans indicating the approximate equality of corresponding components.
 */
template<i32 L, typename T>
BEE_FUNC constexpr vec<L, bool> cwEqual(const vec<L, T>& x, const vec<L, T>& y, const vec<L, T>& epsilon)
{
    vec<L, bool> ret;
    for (int i = 0; i < L; ++i)
        ret[i] = Abs(x[i] - y[i]) <= epsilon[i];
    return ret;
}

template<i32 L, typename T>
BEE_FUNC constexpr vec<L, bool> cwNotEqual(const vec<L, T>& x, const vec<L, T>& y)
{
    return negative(cwEqual(x, y));
}

template<i32 L, typename T>
BEE_FUNC constexpr vec<L, bool> cwNotEqual(const vec<L, T>& x, const vec<L, T>& y, T epsilon)
{
    return negative(cwEqual(x, y, epsilon));
}

template<i32 L, typename T>
BEE_FUNC constexpr vec<L, bool> cwNotEqual(const vec<L, T>& x, const vec<L, T>& y, const vec<L, T>& epsilon)
{
    return negative(cwEqual(x, y, epsilon));
}

template<i32 L, typename T>
BEE_FUNC constexpr bool Equal(const vec<L, T>& x, const vec<L, T>& y)
{
    return all(cwEqual(x, y));
}

template<i32 L, typename T>
BEE_FUNC constexpr bool Equal(const vec<L, T>& x, const vec<L, T>& y, T epsilon)
{
    return all(cwEqual(x, y, epsilon));
}

template<i32 L, typename T>
BEE_FUNC constexpr bool Equal(const vec<L, T>& x, const vec<L, T>& y, const vec<L, T>& epsilon)
{
    return all(cwEqual(x, y, epsilon));
}

template<i32 L, typename T>
BEE_FUNC constexpr bool NotEqual(const vec<L, T>& x, const vec<L, T>& y)
{
    return not Equal(x, y);
}

template<i32 L, typename T>
BEE_FUNC constexpr bool NotEqual(const vec<L, T>& x, const vec<L, T>& y, T epsilon)
{
    return not Equal(x, y, epsilon);
}

template<i32 L, typename T>
BEE_FUNC constexpr bool NotEqual(const vec<L, T>& x, const vec<L, T>& y, const vec<L, T>& epsilon)
{
    return not Equal(x, y, epsilon);
}

namespace internal {

template<template<i32 L, typename T> class vec, i32 L, typename R, typename T>
struct vec_func_uni
{
};

template<template<i32 L, typename T> class vec, typename R, typename T>
struct vec_func_uni<vec, 1, R, T>
{
    BEE_FUNC constexpr static vec1_t<R> call(R (*Func)(T x), const vec1_t<T>& v)
    {
        return vec1_t<R>(Func(v.x));
    }
};

template<template<i32 L, typename T> class vec, typename R, typename T>
struct vec_func_uni<vec, 2, R, T>
{
    BEE_FUNC constexpr static vec2_t<R> call(R (*Func)(T x), const vec2_t<T>& v)
    {
        return vec2_t<R>(Func(v.x), Func(v.y));
    }
};

template<template<i32 L, typename T> class vec, typename R, typename T>
struct vec_func_uni<vec, 3, R, T>
{
    BEE_FUNC constexpr static vec3_t<R> call(R (*Func)(T x), const vec3_t<T>& v)
    {
        return vec3_t<R>(Func(v.x), Func(v.y), Func(v.z));
    }
};

template<template<i32 L, typename T> class vec, typename R, typename T>
struct vec_func_uni<vec, 4, R, T>
{
    BEE_FUNC constexpr static vec4_t<R> call(R (*Func)(T x), const vec4_t<T>& v)
    {
        return vec4_t<R>(Func(v.x), Func(v.y), Func(v.z), Func(v.w));
    }
};

template<template<i32 L, typename T> class vec, i32 L, typename T>
struct vec_func_bin
{
};

template<template<i32 L, typename T> class vec, typename T>
struct vec_func_bin<vec, 1, T>
{
    BEE_FUNC constexpr static vec1_t<T> call(T (*Func)(T x, T y), const vec1_t<T>& a, const vec1_t<T>& b)
    {
        return vec1_t<T>(Func(a.x, b.x));
    }

    template<typename F>
    BEE_FUNC constexpr static vec1_t<T> call(F&& Func, const vec1_t<T>& a, const vec1_t<T>& b)
    {
        return vec1_t<T>(Func(a.x, b.x));
    }
};

template<template<i32 L, typename T> class vec, typename T>
struct vec_func_bin<vec, 2, T>
{
    BEE_FUNC constexpr static vec2_t<T> call(T (*Func)(T x, T y), const vec2_t<T>& a, const vec2_t<T>& b)
    {
        return vec2_t<T>(Func(a.x, b.x), Func(a.y, b.y));
    }

    template<typename F>
    BEE_FUNC constexpr static vec2_t<T> call(F&& Func, const vec2_t<T>& a, const vec2_t<T>& b)
    {
        return vec2_t<T>(Func(a.x, b.x), Func(a.y, b.y));
    }
};

template<template<i32 L, typename T> class vec, typename T>
struct vec_func_bin<vec, 3, T>
{
    BEE_FUNC constexpr static vec3_t<T> call(T (*Func)(T x, T y), const vec3_t<T>& a, const vec3_t<T>& b)
    {
        return vec3_t<T>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z));
    }

    template<typename F>
    BEE_FUNC constexpr static vec3_t<T> call(F&& Func, const vec3_t<T>& a, const vec3_t<T>& b)
    {
        return vec3_t<T>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z));
    }
};

template<template<i32 L, typename T> class vec, typename T>
struct vec_func_bin<vec, 4, T>
{
    BEE_FUNC constexpr static vec4_t<T> call(T (*Func)(T x, T y), const vec4_t<T>& a, const vec4_t<T>& b)
    {
        return vec4_t<T>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z), Func(a.z, b.z));
    }

    template<typename F>
    BEE_FUNC constexpr static vec4_t<T> call(F&& Func, const vec4_t<T>& a, const vec4_t<T>& b)
    {
        return vec4_t<T>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z), Func(a.z, b.z));
    }
};

template<i32 L, typename T>
struct vec_reduce
{
    template<int c>
    BEE_FUNC constexpr static vec<L, T> call(const vec<L, T>& a)
    {
        vec<L, T> v;
        for (int i = 0; i < L; ++i)
            v[i] = a[c];
        return v;
    }
};

} // namespace internal

template<i32 L, typename T>
BEE_FUNC vec<L, T> ReduceX(const vec<L, T>& a)
{
    return internal::vec_reduce<L, T>::template call<0>(a);
}

template<i32 L, typename T>
BEE_FUNC vec<L, T> ReduceY(const vec<L, T>& a)
{
    return internal::vec_reduce<L, T>::template call<1>(a);
}

template<i32 L, typename T>
BEE_FUNC vec<L, T> ReduceZ(const vec<L, T>& a)
{
    return internal::vec_reduce<L, T>::template call<2>(a);
}

template<i32 L, typename T>
BEE_FUNC vec<L, T> ReduceW(const vec<L, T>& a)
{
    return internal::vec_reduce<L, T>::template call<3>(a);
}

template<i32 L, FloatType T>
BEE_FUNC_DECL T IsNaN(const vec<L, T>& v)
{
    return internal::vec_func_uni<vec, L, bool, T>::call(IsNaN, v);
}

template<i32 L, FloatType T>
BEE_FUNC_DECL T IsInf(const vec<L, T>& v)
{
    return internal::vec_func_uni<vec, L, bool, T>::call(IsInf, v);
}

template<i32 L, FloatType T>
BEE_FUNC_DECL T IsFinite(const vec<L, T>& v)
{
    return internal::vec_func_uni<vec, L, bool, T>::call(IsFinite, v);
}

#define DEFINE_VECTOR_ELEMENT_UNARY_OP(ValType, op)                                                                    \
    template<i32 L, ValType T>                                                                                         \
    BEE_FUNC_DECL constexpr vec<L, T> op(const vec<L, T>& v)                                                           \
    {                                                                                                                  \
        return internal::vec_func_uni<vec, L, T, T>::call(op, v);                                                      \
    }

// 基础操作
DEFINE_VECTOR_ELEMENT_UNARY_OP(SignedType, Abs)
DEFINE_VECTOR_ELEMENT_UNARY_OP(SignedType, Sign)
// Round 操作
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Floor)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Ceil)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Trunc)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Round)
// 开方、指数、对数
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Sqrt)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, rSqrt)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Exp)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Exp2)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Log)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Log2)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Log10)
// 三角函数
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Sin)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Cos)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Tan)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, aSin)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, aCos)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, aTan)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Sinh)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Cosh)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Tanh)
// 其他常规操作
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Radians)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Degrees)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Frac)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Rcp)
DEFINE_VECTOR_ELEMENT_UNARY_OP(FloatType, Saturate)
#undef DEFINE_VECTOR_ELEMENT_UNARY_OP

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr vec<L, T> cwMin(const vec<L, T>& a, const vec<L, T>& b)
{
    return internal::vec_func_bin<vec, L, T>::call(Min, a, b);
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr vec<L, T> cwMax(const vec<L, T>& a, const vec<L, T>& b)
{
    return internal::vec_func_bin<vec, L, T>::call(Max, a, b);
}

#define DEFINE_VECTOR_ELEMENT_BINARY_OP(ValType, op)                                                                   \
    template<i32 L, ValType T>                                                                                         \
    BEE_FUNC constexpr vec<L, T> op(const vec<L, T>& a, const vec<L, T>& b)                                            \
    {                                                                                                                  \
        return internal::vec_func_bin<vec, L, T>::call(op, a, b);                                                      \
    }

DEFINE_VECTOR_ELEMENT_BINARY_OP(ArithmeticType, Min)
DEFINE_VECTOR_ELEMENT_BINARY_OP(ArithmeticType, Max)

DEFINE_VECTOR_ELEMENT_BINARY_OP(FloatType, Pow)
DEFINE_VECTOR_ELEMENT_BINARY_OP(FloatType, fMod)
DEFINE_VECTOR_ELEMENT_BINARY_OP(FloatType, Step)
DEFINE_VECTOR_ELEMENT_BINARY_OP(FloatType, aTan2)
#undef DEFINE_VECTOR_ELEMENT_BINARY_OP

template<i32 L, FloatType T>
BEE_FUNC constexpr auto Lerp(const vec<L, T>& a, const vec<L, T>& b, T s) -> vec<L, T>
{
    if constexpr (L == 1)
        return vec1_t<T>{Lerp(a.x, b.x, s)};
    else if constexpr (L == 2)
        return vec2_t<T>{Lerp(a.x, b.x, s), Lerp(a.y, b.y, s)};
    else if constexpr (L == 3)
        return vec3_t<T>{Lerp(a.x, b.x, s), Lerp(a.y, b.y, s), Lerp(a.z, b.z, s)};
    else if constexpr (L == 4)
        return vec4_t<T>{Lerp(a.x, b.x, s), Lerp(a.y, b.y, s), Lerp(a.z, b.z, s), Lerp(a.w, b.w, s)};
}

// clang-format off
template<i32 L, FloatType T>
BEE_FUNC constexpr auto BiLerp(const vec<L, T>& v00, const vec<L, T>& v01, const vec<L, T>& v10, const vec<L, T>& v11, T u, T v) -> vec<L, T>
{
    if constexpr (L == 1)
        return vec1_t<T>{BiLerp(v00.x, v01.x, v10.x, v11.x, u, v)};
    else if constexpr (L == 2)
        return vec2_t<T>{BiLerp(v00.x, v01.x, v10.x, v11.x, u, v), BiLerp(v00.y, v01.y, v10.y, v11.y, u, v)};
    else if constexpr (L == 3)
        return vec3_t<T>{BiLerp(v00.x, v01.x, v10.x, v11.x, u, v), BiLerp(v00.y, v01.y, v10.y, v11.y, u, v), 
                         BiLerp(v00.z, v01.z, v10.z, v11.z, u, v)};
    else if constexpr (L == 4)
        return vec4_t<T>{BiLerp(v00.x, v01.x, v10.x, v11.x, u, v), BiLerp(v00.y, v01.y, v10.y, v11.y, u, v),
                         BiLerp(v00.z, v01.z, v10.z, v11.z, u, v), BiLerp(v00.w, v01.w, v10.w, v11.w, u, v)};
}

template<i32 L, FloatType T>
BEE_FUNC constexpr auto SmoothStep(const vec<L, T>& v, const vec<L, T>& lo, const vec<L, T>& hi) -> vec<L, T>
{
    if constexpr (L == 1)
        return vec1_t<T>{SmoothStep(v.x, lo.x, hi.x)};
    else if constexpr (L == 2)
        return vec2_t<T>{SmoothStep(v.x, lo.x, hi.x), SmoothStep(v.y, lo.y, hi.y)};
    else if constexpr (L == 3)
        return vec3_t<T>{SmoothStep(v.x, lo.x, hi.x), SmoothStep(v.y, lo.y, hi.y), SmoothStep(v.z, lo.z, hi.z)};
    else if constexpr (L == 4)
        return vec4_t<T>{SmoothStep(v.x, lo.x, hi.x), SmoothStep(v.y, lo.y, hi.y),
                         SmoothStep(v.z, lo.z, hi.z), SmoothStep(v.w, lo.w, hi.w)};
}
// clang-format on

template<i32 L, FloatType T>
BEE_FUNC constexpr auto Fma(const vec<L, T>& a, const vec<L, T>& b, const vec<L, T>& c) -> vec<L, T>
{
    if constexpr (L == 1)
        return vec1_t<T>{Fma(a.x, b.x, c.x)};
    else if constexpr (L == 2)
        return vec2_t<T>{Fma(a.x, b.x, c.x), Fma(a.y, b.y, c.y)};
    else if constexpr (L == 3)
        return vec3_t<T>{Fma(a.x, b.x, c.x), Fma(a.y, b.y, c.y), Fma(a.z, b.z, c.z)};
    else if constexpr (L == 4)
        return vec4_t<T>{Fma(a.x, b.x, c.x), Fma(a.y, b.y, c.y), Fma(a.z, b.z, c.z), Fma(a.w, b.w, c.w)};
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr i32 MinIndex(const vec<L, T>& v)
{
    if constexpr (L == 1)
        return 0;
    if constexpr (L == 2)
        return (v.x < v.y) ? 0 : 1;
    else if constexpr (L == 3) {
        auto i = (v.x < v.y) ? 0 : 1;
        return v[i] < v.z ? i : 2;
    }
    else if constexpr (L == 4) {
        auto i = (v.x < v.y) ? 0 : 1;
        i      = v[i] < v.z ? i : 2;
        return v[i] < v.w ? i : 3;
    }
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr i32 MaxIndex(const vec<L, T>& v)
{
    if constexpr (L == 1)
        return 0;
    if constexpr (L == 2)
        return (v.x > v.y) ? 0 : 1;
    else if constexpr (L == 3) {
        auto i = (v.x > v.y) ? 0 : 1;
        return v[i] > v.z ? i : 2;
    }
    else if constexpr (L == 4) {
        auto i = (v.x > v.y) ? 0 : 1;
        i      = v[i] > v.z ? i : 2;
        return v[i] > v.w ? i : 3;
    }
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto MinValue(const vec<L, T>& v)
{
    if constexpr (L == 1)
        return v.x;
    if constexpr (L == 2)
        return Min(v.x, v.y);
    else if constexpr (L == 3)
        return Min(v.x, v.y, v.z);
    else if constexpr (L == 4)
        return Min(v.x, v.y, v.z, v.w);
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto MaxValue(const vec<L, T>& v)
{
    if constexpr (L == 1)
        return v.x;
    if constexpr (L == 2)
        return Max(v.x, v.y);
    else if constexpr (L == 3)
        return Max(v.x, v.y, v.z);
    else if constexpr (L == 4)
        return Max(v.x, v.y, v.z, v.w);
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto ReduceMin(const vec<L, T>& v) -> vec<L, T>
{
    auto val = MinValue(v);
    if constexpr (L == 1)
        return vec1_t<T>{val};
    else if constexpr (L == 2)
        return vec2_t<T>{val, val};
    else if constexpr (L == 3)
        return vec3_t<T>{val, val, val};
    else if constexpr (L == 4)
        return vec4_t<T>{val, val, val, val};
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto ReduceMax(const vec<L, T>& v) -> vec<L, T>
{
    auto val = MaxValue(v);
    if constexpr (L == 1)
        return vec1_t<T>{val};
    else if constexpr (L == 2)
        return vec2_t<T>{val, val};
    else if constexpr (L == 3)
        return vec3_t<T>{val, val, val};
    else if constexpr (L == 4)
        return vec4_t<T>{val, val, val, val};
}

// clang-format off
template<i32 L, FloatType T>
BEE_FUNC constexpr auto Clamp(const vec<L, T>& v, const vec<L, T>& lo, const vec<L, T>& hi) -> vec<L, T>
{
    if constexpr (L == 1)
        return vec1_t<T>{Clamp(v.x, lo.x, hi.x)};
    else if constexpr (L == 2)
        return vec2_t<T>{Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y)};
    else if constexpr (L == 3)
        return vec3_t<T>{Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y), Clamp(v.z, lo.z, hi.z)};
    else if constexpr (L == 4)
        return vec4_t<T>{Clamp(v.x, lo.x, hi.x), Clamp(v.y, lo.y, hi.y), Clamp(v.z, lo.z, hi.z), Clamp(v.w, lo.w, hi.w)};
}
// clang-format on

template<i32 L, FloatType T>
BEE_FUNC constexpr auto Clamp(const vec<L, T>& v, const vec<L, T>& hi) -> vec<L, T>
{
    if constexpr (L == 1)
        return vec1_t<T>{Clamp(v.x, hi.x)};
    else if constexpr (L == 2)
        return vec2_t<T>{Clamp(v.x, hi.x), Clamp(v.y, hi.y)};
    else if constexpr (L == 3)
        return vec3_t<T>{Clamp(v.x, hi.x), Clamp(v.y, hi.y), Clamp(v.z, hi.z)};
    else if constexpr (L == 4)
        return vec4_t<T>{Clamp(v.x, hi.x), Clamp(v.y, hi.y), Clamp(v.z, hi.z), Clamp(v.w, hi.w)};
}

template<i32 L, ArithmeticType T, ArithmeticType U>
BEE_FUNC constexpr auto Approx(const vec<L, T>& v, const vec<L, U>& hi) -> bool
{
    if constexpr (L == 1)
        return Approx(v.x, hi.x);
    else if constexpr (L == 2)
        return Approx(v.x, hi.x) and Approx(v.y, hi.y);
    else if constexpr (L == 3)
        return Approx(v.x, hi.x) and Approx(v.y, hi.y) and Approx(v.z, hi.z);
    else if constexpr (L == 4)
        return Approx(v.x, hi.x) and Approx(v.y, hi.y) and Approx(v.z, hi.z) and Approx(v.w, hi.w);
}

// -------------------------
// 向量方法
// -------------------------
template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto Dot(const vec<L, T>& lhs, const vec<L, T>& rhs) -> T
{
    T res = Zero<T>();
    for (i32 i = 0; i < L; ++i)
        res += lhs[i] * rhs[i];

    return res;
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto AbsDot(const vec<L, T>& lhs, const vec<L, T>& rhs) -> T
{
    return Abs(Dot(lhs, rhs));
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto NormalizeDot(const vec<L, T>& lhs, const vec<L, T>& rhs)
{
    if constexpr (sizeof(T) == 4) {
        return f32(Dot(lhs, rhs)) * rSqrt(f32(Dot(lhs, lhs) * Dot(rhs, rhs)));
    }
    else {
        return f64(Dot(lhs, rhs)) * rSqrt(f64(Dot(lhs, lhs) * Dot(rhs, rhs)));
    }
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto Length(const vec<L, T>& v)
{
    if constexpr (sizeof(T) == 4) {
        return Sqrt((f32)Dot(v, v));
    }
    else {
        return Sqrt((f64)Dot(v, v));
    }
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto LengthSqr(const vec<L, T>& v) -> T
{
    return Dot(v, v);
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto Distance(const vec<L, T>& v1, const vec<L, T>& v2)
{
    return Length(v1 - v2);
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto DistanceSqr(const vec<L, T>& v1, const vec<L, T>& v2)
{
    return LengthSqr(v1 - v2);
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto Normalize(const vec<L, T>& v)
{
    if constexpr (sizeof(T) == 4) {
        return vec<L, f32>(v) * rSqrt((f32)Dot(v, v));
    }
    else {
        return vec<L, f64>(v) * rSqrt((f64)Dot(v, v));
    }
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto Prod(const vec<L, T>& v) -> T
{
    T res = One<T>();
    for (i32 i = 0; i < L; ++i)
        res *= v[i];

    return res;
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr auto AbsProd(const vec<L, T>& v) -> T
{
    return Abs(prod(v));
}

template<typename T>
BEE_FUNC constexpr T Cross(const vec2_t<T>& v1, const vec2_t<T>& v2)
{
    return v1.x * v2.y - v2.x * v1.y;
}

template<typename T>
BEE_FUNC constexpr auto Cross(const vec3_t<T>& v1, const vec3_t<T>& v2)
{
    return vec3_t<T>{v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

template<typename T>
BEE_FUNC constexpr auto FromVector(const vec3_t<T>& v) -> vec2_t<T>
{
    return vec2_t<T>{v.x, v.y};
}

template<typename T>
BEE_FUNC constexpr auto FromPoint(const vec3_t<T>& v) -> vec2_t<T>
{
    BEE_CHECK(v.w != 0);
    return vec2_t<T>{v.xy() / v.w};
}

template<typename T>
BEE_FUNC constexpr auto FromVector(const vec4_t<T>& v) -> vec3_t<T>
{
    return vec3_t<T>{v.xyz()};
}

template<typename T>
BEE_FUNC constexpr auto FromPoint(const vec4_t<T>& v) -> vec3_t<T>
{
    BEE_CHECK(v.w != 0);
    return vec3_t<T>{v.xyz() / v.w};
}

template<typename T>
BEE_FUNC constexpr auto ToVector(const vec2_t<T>& v) -> vec3_t<T>
{
    return vec3_t<T>{v, Zero<T>()};
}

template<typename T>
BEE_FUNC constexpr auto ToPoint(const vec2_t<T>& v) -> vec3_t<T>
{
    return vec3_t<T>{v, One<T>()};
}

template<typename T>
BEE_FUNC constexpr auto ToVector(const vec3_t<T>& v) -> vec4_t<T>
{
    return vec4_t<T>{v, Zero<T>()};
}

template<typename T>
BEE_FUNC constexpr auto ToPoint(const vec3_t<T>& v) -> vec4_t<T>
{
    return vec4_t<T>{v, One<T>()};
}

template<i32 L, FloatType T>
BEE_FUNC vec<L, T> Proj(const vec<L, T>& x, const vec<L, T>& Normal)
{
    return Dot(x, Normal) / Dot(Normal, Normal) * Normal;
}

template<i32 L, FloatType T>
BEE_FUNC vec<L, T> Perp(const vec<L, T>& x, const vec<L, T>& Normal)
{
    return x - Proj(x, Normal);
}

template<ArithmeticType T>
BEE_FUNC constexpr auto GramSchmidt(const vec3_t<T>& v1, const vec3_t<T>& v2) -> vec3_t<T>
{
    return v2 - Proj(v2, v1);
}

template<ArithmeticType T>
BEE_FUNC constexpr void CoordinateSystem(const vec3_t<T>& v1, vec3_t<T>& v2, vec3_t<T>& v3)
{
    auto sign = CopySign(Float(1), Float(v1.z));
    auto a    = -One<T>() / Float(sign + v1.z);
    auto b    = Float(v1.x * v1.y * a);

    v2 = vec3_t<T>(T(1 + sign * Sqr(v1.x) * a), T(sign * b), T(-sign * v1.x));
    v3 = vec3_t<T>(T(b), T(sign + Sqr(v1.y) * a), T(-v1.y));
}

template<i32 L, ArithmeticType T>
BEE_FUNC constexpr Float AngleBetween(const vec<L, T>& v1, const vec<L, T>& v2)
{
    if constexpr (L == 1) {
        return aCos(Clamp(Dot(Normalize(v1), Normalize(v2)), T(-1), T(1)));
    }
    else {
        vec3_t<T> nv1, nv2;

        if constexpr (L == 2) {
            nv1 = vec3_t<T>(v1, 0), nv2 = vec3_t<T>(v2, 0);
        }
        else {
            nv1 = vec3_t<T>(v1), nv2 = vec3_t<T>(v2);
        }

        const auto dot     = Dot(nv1, nv2);
        const auto lenSqr1 = Dot(nv1, nv1);
        const auto lenSqr2 = Dot(nv2, nv2);

        return (Float)aCos(dot * rSqrt(lenSqr1 * lenSqr2));
        //        if (Dot(v1, v2) < 0)
        //            return Pi<Float>() - Two<Float>() * (Float)SafeASin(Length(v1 + v2) / 2);
        //        else
        //            return Two<Float>() * (Float)SafeASin(Length(v2 - v1) / 2);
    }
}

template<FloatType T>
BEE_FUNC T OrientedAngle(const vec<2, T>& x, const vec<2, T>& y)
{
    const T res(aCos(Clamp(Dot(x, y), T(-1), T(1))));

    const T partialCross = x.x * y.y - y.x * x.y;

    if (partialCross > T(0))
        return res;
    else
        return -res;
}

template<FloatType T>
BEE_FUNC T OrientedAngle(const vec<3, T>& x, const vec<3, T>& y, const vec<3, T>& ref)
{
    const T res(aCos(Clamp(Dot(x, y), T(-1), T(1))));

    if (Dot(ref, Cross(x, y)) > T(0))
        return res;
    else
        return -res;
}

/// 在 a，b 直线上，找到离点 p 最近的点
template<i32 L, ArithmeticType T>
    requires(L == 2 or L == 3)
BEE_FUNC constexpr vec<L, T> ClosestPoint(const vec<L, T>& a, const vec<L, T>& b, const vec<L, T>& p)
{
    auto lineLength = Distance(a, b);
    BEE_CHECK_GE(lineLength, 0);

    auto vector  = p - a;
    auto lineDir = (b - a) / lineLength;
    auto dist    = Dot(vector, lineDir);

    if (dist <= T(0))
        return a;
    if (dist >= lineLength)
        return b;

    return a + lineDir * dist;
}

/// 二维高斯
template<ArithmeticType T>
BEE_FUNC constexpr T Gauss(const vec2_t<T>& coord, const vec2_t<T>& expectedValue, const vec2_t<T>& standardDeviation)
{
    const auto squared =
        ((coord - expectedValue) * (coord - expectedValue)) / (Two<T>() * standardDeviation * standardDeviation);

    return Exp(-(squared.x + squared.y));
}

template<typename T>
BEE_FUNC T RadialGradient(const vec2_t<T>& center, const T& radius, const vec2_t<T>& focal, const vec2_t<T>& position)
{
    vec2_t<T> F = focal - center;
    vec2_t<T> D = position - focal;
    T rad2      = Pow2(radius);
    T Fx2       = Pow2(F.x);
    T Fy2       = Pow2(F.y);

    T numerator   = (D.x * F.x + D.y * F.y) + Sqrt(rad2 * (Pow2(D.x) + Pow2(D.y)) - Pow2(D.x * F.y - D.y * F.x));
    T denominator = rad2 - (Fx2 + Fy2);
    return numerator / denominator;
}

template<typename T>
BEE_FUNC T LinearGradient(const vec2_t<T>& point0, const vec2_t<T>& point1, const vec2_t<T>& position)
{
    vec2_t<T> dist = point1 - point0;
    return (dist.x * (position.x - point0.x) + dist.y * (position.y - point0.y)) / Dot(dist, dist);
}

template<ArithmeticType T>
BEE_FUNC constexpr bool isRightHanded(const vec3_t<T>& tangent, const vec3_t<T>& binormal, const vec3_t<T>& normal)
{
    return Dot(Cross(normal, tangent), binormal) > T(0);
}

template<ArithmeticType T>
BEE_FUNC constexpr bool isLeftHanded(const vec3_t<T>& tangent, const vec3_t<T>& binormal, const vec3_t<T>& normal)
{
    return Dot(Cross(normal, tangent), binormal) < T(0);
}

template<i32 L, FloatType T>
BEE_FUNC constexpr bool isNormalized(const vec<L, T>& v, const T& epsilon)
{
    return Abs(Length(v) - One<T>()) < epsilon;
}

// -------------------------
// 向量几何操作
// -------------------------

template<i32 L, FloatType T>
BEE_FUNC vec<L, T> FaceForward(const vec<L, T>& N, const vec<L, T>& I, const vec<L, T>& refN)
{
    return Dot(refN, I) < Zero<T>() ? N : -N;
}

template<i32 L, FloatType T>
BEE_FUNC vec<L, T> Reflect(const vec<L, T>& I, const vec<L, T>& N)
{
    return I - Normalize(N) * Dot(Normalize(N), I) * Two<T>();
}

//template<i32 L, FloatType T>
//BEE_FUNC vec<L, T> Refract(const vec<L, T>& I, const vec<L, T>& N, T eta)
//{
//    const T dot_val(Dot(N, I));
//    const T k(cast_to<T>(1) - eta * eta * (cast_to<T>(1) - dot_val * dot_val));
//
//    const auto res = (k >= cast_to<T>(0)) ? (eta * I - (eta * dot_val + std::sqrt(k)) * N) : vec<L, T>(0);
//    return res;
//}

template<typename T>
BEE_FUNC vec3_t<T> Polar(const vec3_t<T>& euclidean)
{
    const T len     = Length(euclidean);
    const auto& p   = euclidean / len;
    const T xy_dist = Sqrt(p.x * p.x + p.y * p.y);

    return vec3_t<T>(aCos(p.z),                                // latitude
                     aCos(p.x / xy_dist) * (p.y < 0 ? -1 : 1), // longitude
                     len);                                     // xz Distance
}

template<typename T>
BEE_FUNC vec3_t<T> Euclidean(const vec2_t<T>& polar)
{
    const T latitude(polar.x);
    const T longitude(polar.y);

    return vec3_t<T>(Cos(longitude) * Sin(latitude), Sin(longitude) * Sin(latitude), Cos(latitude));
}

} // namespace bee