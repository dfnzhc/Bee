/**
 * @File Vector3.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/30
 * @Brief 
 */

#pragma once

#include "./VectorType.hpp"

namespace bee {

template<typename T>
struct vec<3, T>
{
    using value_type = T;
    using vec_type   = vec3_t<T>;
    using bool_type  = vec3_t<bool>;

    // clang-format off
    union
    {
        struct { T x, y, z; };
        struct { T r, g, b; };
        struct { T s, t, p; };
    };
    // clang-format on

    BEE_FUNC constexpr vec()             = default;
    BEE_FUNC constexpr vec(const vec& v) = default;

    BEE_FUNC constexpr explicit vec(T scalar);
    BEE_FUNC constexpr vec(T a, T b, T c);

    template<typename U>
    BEE_FUNC constexpr explicit vec(const vec1_t<U>& v);

    // clang-format off
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(X x, Y y, Z z);
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(const vec1_t<X>& x, Y y, Z z);
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(X x, const vec1_t<Y>& y, Z z);
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(const vec1_t<X>& x, const vec1_t<Y>& y, Z z);
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(X x, Y y, const vec1_t<Z>& z);
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(const vec1_t<X>& x, Y y, const vec1_t<Z>& z);
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(X x, const vec1_t<Y>& y, const vec1_t<Z>& z);
    template <typename X, typename Y, typename Z> BEE_FUNC constexpr vec(const vec1_t<X>& x, const vec1_t<Y>& y, const vec1_t<Z>& z);
    // clang-format on

    // clang-format off
    template <typename A, typename B> BEE_FUNC constexpr vec(vec2_t<A> const& _xy, B z);
    template <typename A, typename B> BEE_FUNC constexpr vec(vec2_t<A> const& _xy, const vec1_t<B>& z);
    template <typename A, typename B> BEE_FUNC constexpr vec(A x, const vec2_t<B>& _yz);
    template <typename A, typename B> BEE_FUNC constexpr vec(const vec1_t<A>& x, const vec2_t<B>& _yz);
    template <typename U> BEE_FUNC constexpr explicit vec(const vec4_t<U>& v);
    template <typename U> BEE_FUNC constexpr explicit vec(const vec3_t<U>& v);
    // clang-format on

    BEE_FUNC constexpr vec_type& operator=(const vec_type& v) = default;

    BEE_FUNC constexpr vec_type& operator++();
    BEE_FUNC constexpr vec_type& operator--();
    BEE_FUNC constexpr vec_type operator++(int);
    BEE_FUNC constexpr vec_type operator--(int);

    // clang-format off
    BEE_FUNC constexpr auto operator<=>(const vec_type& v) const noexcept;

    template<typename U> BEE_FUNC constexpr auto operator<=>(const vec3_t<U>& v) const noexcept requires Convertible<T, U>;
    template<typename U> BEE_FUNC constexpr bool  operator==(const vec3_t<U>& v) const noexcept requires Convertible<T, U>;
    // clang-format on

    // clang-format off
    template <typename U> BEE_FUNC constexpr vec_type& operator =(const vec3_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator+=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator+=(const vec3_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(const vec3_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(const vec3_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(const vec3_t<U>& v);

    template <typename U> BEE_FUNC constexpr vec_type& operator+=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(U scalar);
    // clang-format on

    // clang-format off
    template <typename U> BEE_FUNC constexpr vec_type&  operator%=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator%=(const vec3_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator&=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator&=(const vec3_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator|=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator|=(const vec3_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator^=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator^=(const vec3_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator<<=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator<<=(const vec3_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator>>=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator>>=(const vec3_t<U>& v) requires BothIntegral<T, U>;

    template <typename U> BEE_FUNC constexpr vec_type&  operator%=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator&=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator|=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator^=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator<<=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator>>=(U scalar) requires BothIntegral<T, U>;
    // clang-format on

    // clang-format off
    BEE_FUNC constexpr       T& operator[](int index)       noexcept { return (&x)[index]; }
    BEE_FUNC constexpr const T& operator[](int index) const noexcept { return (&x)[index]; }

    BEE_FUNC static constexpr int dim() noexcept { return 3; }
    
    #include "Vector3Swizzle.inl"
    // clang-format on
};
} //namespace bee

namespace bee {

template<typename T>
BEE_FUNC constexpr vec3_t<T>::vec(T scalar) : x(scalar), y(scalar), z(scalar)
{
}

template<typename T>
BEE_FUNC constexpr vec3_t<T>::vec(T _x, T _y, T _z) : x(_x), y(_y), z(_z)
{
}

template<typename T>
template<typename U>
BEE_FUNC constexpr vec3_t<T>::vec(const vec1_t<U>& v) : x(cast_to<T>(v.x)), y(cast_to<T>(v.x)), z(cast_to<T>(v.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(X _x, Y _y, Z _z) : x(cast_to<T>(_x)), y(cast_to<T>(_y)), z(cast_to<T>(_z))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(const vec1_t<X>& _x, Y _y, Z _z)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y)), z(cast_to<T>(_z))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(X _x, const vec1_t<Y>& _y, Z _z)
: x(cast_to<T>(_x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(const vec1_t<X>& _x, const vec1_t<Y>& _y, Z _z)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(X _x, Y _y, const vec1_t<Z>& _z)
: x(cast_to<T>(_x)), y(cast_to<T>(_y)), z(cast_to<T>(_z.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(const vec1_t<X>& _x, Y _y, const vec1_t<Z>& _z)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y)), z(cast_to<T>(_z.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(X _x, const vec1_t<Y>& _y, const vec1_t<Z>& _z)
: x(cast_to<T>(_x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z>
BEE_FUNC constexpr vec3_t<T>::vec(const vec1_t<X>& _x, const vec1_t<Y>& _y, const vec1_t<Z>& _z)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z.x))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec3_t<T>::vec(const vec2_t<A>& _xy, B _z)
: x(cast_to<T>(_xy.x)), y(cast_to<T>(_xy.y)), z(cast_to<T>(_z))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec3_t<T>::vec(const vec2_t<A>& _xy, const vec1_t<B>& _z)
: x(cast_to<T>(_xy.x)), y(cast_to<T>(_xy.y)), z(cast_to<T>(_z.x))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec3_t<T>::vec(A _x, const vec2_t<B>& _yz)
: x(cast_to<T>(_x)), y(cast_to<T>(_yz.x)), z(cast_to<T>(_yz.y))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec3_t<T>::vec(const vec1_t<A>& _x, const vec2_t<B>& _yz)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_yz.x)), z(cast_to<T>(_yz.y))
{
}

template<typename T>
template<typename U>
BEE_FUNC constexpr vec3_t<T>::vec(const vec3_t<U>& v) : x(cast_to<T>(v.x)), y(cast_to<T>(v.y)), z(cast_to<T>(v.z))
{
}

template<typename T>
template<typename U>
BEE_FUNC constexpr vec3_t<T>::vec(const vec4_t<U>& v) : x(cast_to<T>(v.x)), y(cast_to<T>(v.y)), z(cast_to<T>(v.z))
{
}

template<typename T>
template<typename U>
BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator=(const vec3_t<U>& v)
{
    this->x = cast_to<T>(v.x);
    this->y = cast_to<T>(v.y);
    this->z = cast_to<T>(v.z);
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator++()
{
    ++this->x;
    ++this->y;
    ++this->z;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator--()
{
    --this->x;
    --this->y;
    --this->z;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec3_t<T> vec3_t<T>::operator++(int)
{
    vec3_t<T> res(*this);
    ++*this;
    return res;
}

template<typename T>
BEE_FUNC constexpr vec3_t<T> vec3_t<T>::operator--(int)
{
    vec3_t<T> res(*this);
    --*this;
    return res;
}

template<typename T>
constexpr auto vec3_t<T>::operator<=>(const vec3_t<T>& v) const noexcept
{
    if (auto cmp = x <=> v.x; cmp != 0)
        return cmp;
    if (auto cmp = y <=> v.y; cmp != 0)
        return cmp;
    return z <=> v.z;
}

template<typename T>
template<typename U>
constexpr auto vec3_t<T>::operator<=>(const vec3_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return this <=> vec_type{cast_to<T>(v.x), cast_to<T>(v.y), cast_to<T>(v.z)};
}

template<typename T>
template<typename U>
constexpr bool vec3_t<T>::operator==(const vec3_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return x == cast_to<T>(v.x) && y == cast_to<T>(v.y) && z == cast_to<T>(v.z);
}

#define DEFINE_VECTOR3_ARITHMETIC_OP(op)                                                                               \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator op(U scalar)                                                     \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        this->y op cast_to<T>(scalar);                                                                                 \
        this->z op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator op(const vec1_t<U>& v)                                           \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.x);                                                                                    \
        this->z op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator op(const vec3_t<U>& v)                                           \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.y);                                                                                    \
        this->z op cast_to<T>(v.z);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR3_ARITHMETIC_OP(+=)
DEFINE_VECTOR3_ARITHMETIC_OP(-=)
DEFINE_VECTOR3_ARITHMETIC_OP(*=)
DEFINE_VECTOR3_ARITHMETIC_OP(/=)
#undef DEFINE_VECTOR3_ARITHMETIC_OP

#define DEFINE_VECTOR3_BIT_OP(op)                                                                                      \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator op(U scalar)                                                     \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        this->y op cast_to<T>(scalar);                                                                                 \
        this->z op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator op(const vec1_t<U>& v)                                           \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.x);                                                                                    \
        this->z op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec3_t<T>& vec3_t<T>::operator op(const vec3_t<U>& v)                                           \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.y);                                                                                    \
        this->z op cast_to<T>(v.y);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR3_BIT_OP(%=)
DEFINE_VECTOR3_BIT_OP(&=)
DEFINE_VECTOR3_BIT_OP(|=)
DEFINE_VECTOR3_BIT_OP(^=)
DEFINE_VECTOR3_BIT_OP(<<=)
DEFINE_VECTOR3_BIT_OP(>>=)
#undef DEFINE_VECTOR3_BIT_OP

template<typename T>
BEE_FUNC constexpr vec3_t<T> operator+(const vec3_t<T>& v)
{
    return v;
}

template<typename T>
BEE_FUNC constexpr vec3_t<T> operator-(const vec3_t<T>& v)
{
    return vec3_t<T>(0) -= v;
}

template<IntegralType T>
BEE_FUNC constexpr vec3_t<T> operator~(const vec3_t<T>& v)
{
    return vec3_t<T>(~v.x, ~v.y, ~v.z);
}

template<typename T>
BEE_FUNC constexpr bool operator==(const vec3_t<T>& v1, const vec3_t<T>& v2)
{
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

template<typename T>
BEE_FUNC constexpr bool operator!=(const vec3_t<T>& v1, const vec3_t<T>& v2)
{
    return !(v1 == v2);
}

#define DEFINE_VECTOR3_BINARY_OP(ValType, opName, op)                                                                          \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec3_t<T> operator opName(const vec3_t<T>& v, T scalar)                                             \
    {                                                                                                                  \
        return vec3_t<T>(v) op scalar;                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec3_t<T> operator opName(const vec3_t<T>& v1, const vec1_t<T>& v2)                                 \
    {                                                                                                                  \
        return vec3_t<T>(v1) op v2;                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec3_t<T> operator opName(T scalar, const vec3_t<T>& v)                                             \
    {                                                                                                                  \
        return vec3_t<T>(v) op scalar;                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec3_t<T> operator opName(const vec1_t<T>& v1, const vec3_t<T>& v2)                                 \
    {                                                                                                                  \
        return vec3_t<T>(v2) op v1;                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec3_t<T> operator opName(const vec3_t<T>& v1, const vec3_t<T>& v2)                                 \
    {                                                                                                                  \
        return vec3_t<T>(v1) op v2;                                                                                    \
    }

DEFINE_VECTOR3_BINARY_OP(ArithmeticType, +, +=)
DEFINE_VECTOR3_BINARY_OP(ArithmeticType, -, -=)
DEFINE_VECTOR3_BINARY_OP(ArithmeticType, *, *=)
DEFINE_VECTOR3_BINARY_OP(ArithmeticType, /, /=)

DEFINE_VECTOR3_BINARY_OP(IntegralType, %, %=)
DEFINE_VECTOR3_BINARY_OP(IntegralType, &, &=)
DEFINE_VECTOR3_BINARY_OP(IntegralType, |, |=)
DEFINE_VECTOR3_BINARY_OP(IntegralType, ^, ^=)
DEFINE_VECTOR3_BINARY_OP(IntegralType, <<, <<=)
DEFINE_VECTOR3_BINARY_OP(IntegralType, >>, >>=)
#undef DEFINE_VECTOR3_BINARY_OP

BEE_FUNC constexpr vec3_t<bool> operator&&(const vec3_t<bool>& v1, const vec3_t<bool>& v2)
{
    return {v1.x && v2.x, v1.y && v2.y, v1.z && v2.z};
}

BEE_FUNC constexpr vec3_t<bool> operator||(const vec3_t<bool>& v1, const vec3_t<bool>& v2)
{
    return {v1.x || v2.x, v1.y || v2.y, v1.z || v2.z};
}

} //namespace bee