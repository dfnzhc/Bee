/**
 * @File Vector2.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/30
 * @Brief 
 */

#pragma once

#include "./VectorType.hpp"

namespace bee {

template<typename T>
struct vec<2, T>
{
    using value_type = T;
    using vec_type   = vec2_t<T>;
    using bool_type  = vec2_t<bool>;

    // clang-format off
    union
    {
        struct { T x = {}, y = {}; };
        struct { T r, g; };
        struct { T s, t; };
    };
    // clang-format on

    BEE_FUNC constexpr vec()             = default;
    BEE_FUNC constexpr vec(const vec& v) = default;

    BEE_FUNC constexpr explicit vec(T scalar) : x(scalar), y(scalar) { }
    BEE_FUNC constexpr vec(T x, T y) : x(x), y(y) { }

    // clang-format off
    template<typename U>
    BEE_FUNC constexpr explicit vec(const vec1_t<U>& v) : x(cast_to<T>(v.x)), y(cast_to<T>(v.x)) {}

    template <typename A, typename B> BEE_FUNC constexpr vec(A x, B y)                : x(cast_to<T>(x)),   y(cast_to<T>(y)) {}
    template <typename A, typename B> BEE_FUNC constexpr vec(const vec1_t<A>& x, B y) : x(cast_to<T>(x.x)), y(cast_to<T>(y)) {}
    template <typename A, typename B> BEE_FUNC constexpr vec(A x, const vec1_t<B>& y) : x(cast_to<T>(x)),   y(cast_to<T>(y.x)) {}
    template <typename A, typename B> BEE_FUNC constexpr vec(const vec1_t<A>& x, const vec1_t<B>& y): x(cast_to<T>(x.x)), y(cast_to<T>(y.x)) {}
    // clang-format on

    // clang-format off
    template <typename U> BEE_FUNC constexpr explicit vec(const vec2_t<U>& v) : x(cast_to<T>(v.x)), y(cast_to<T>(v.y)) {}
    template <typename U> BEE_FUNC constexpr explicit vec(const vec3_t<U>& v) : x(cast_to<T>(v.x)), y(cast_to<T>(v.y)) {}
    template <typename U> BEE_FUNC constexpr explicit vec(const vec4_t<U>& v) : x(cast_to<T>(v.x)), y(cast_to<T>(v.y)) {}
    // clang-format on

    BEE_FUNC constexpr vec_type& operator=(const vec& v) = default;

    BEE_FUNC constexpr vec_type& operator++();
    BEE_FUNC constexpr vec_type& operator--();
    BEE_FUNC constexpr vec_type operator++(int);
    BEE_FUNC constexpr vec_type operator--(int);

    // clang-format off
    BEE_FUNC constexpr auto operator<=>(const vec_type& v) const noexcept;

    template<typename U> BEE_FUNC constexpr auto operator<=>(const vec2_t<U>& v) const noexcept requires Convertible<T, U>;
    template<typename U> BEE_FUNC constexpr bool  operator==(const vec2_t<U>& v) const noexcept requires Convertible<T, U>;
    // clang-format on

    // clang-format off
    template <typename U> BEE_FUNC constexpr vec_type& operator =(const vec2_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator+=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator+=(const vec2_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(const vec2_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(const vec2_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(const vec2_t<U>& v);

    template <typename U> BEE_FUNC constexpr vec_type& operator+=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(U scalar);
    // clang-format on

    // clang-format off
    template <typename U> BEE_FUNC constexpr vec_type&  operator%=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator%=(const vec2_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator&=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator&=(const vec2_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator|=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator|=(const vec2_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator^=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator^=(const vec2_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator<<=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator<<=(const vec2_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator>>=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator>>=(const vec2_t<U>& v) requires BothIntegral<T, U>;

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

    BEE_FUNC static constexpr int dim() noexcept { return 2; }

    #include "Vector2Swizzle.inl"
    // clang-format on
};

} //namespace bee

namespace bee {

template<typename T>
template<typename U>
BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator=(const vec2_t<U>& v)
{
    this->x = cast_to<T>(v.x);
    this->y = cast_to<T>(v.y);
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator++()
{
    ++this->x;
    ++this->y;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator--()
{
    --this->x;
    --this->y;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec2_t<T> vec2_t<T>::operator++(int)
{
    vec2_t<T> res(*this);
    ++*this;
    return res;
}

template<typename T>
BEE_FUNC constexpr vec2_t<T> vec2_t<T>::operator--(int)
{
    vec2_t<T> res(*this);
    --*this;
    return res;
}

template<typename T>
BEE_FUNC constexpr auto vec2_t<T>::operator<=>(const vec2_t<T>& v) const noexcept
{
    if (auto cmp = x <=> v.x; cmp != 0)
        return cmp;
    return y <=> v.y;
}

template<typename T>
template<typename U>
BEE_FUNC constexpr auto vec2_t<T>::operator<=>(const vec2_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return this <=> vec_type{cast_to<T>(v.x), cast_to<T>(v.y)};
}

template<typename T>
template<typename U>
BEE_FUNC constexpr bool vec2_t<T>::operator==(const vec2_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return x == cast_to<T>(v.x) && y == cast_to<T>(v.y);
}

#define DEFINE_VECTOR2_ARITHMETIC_OP(op)                                                                               \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator op(U scalar)                                                     \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        this->y op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator op(const vec1_t<U>& v)                                           \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator op(const vec2_t<U>& v)                                           \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.y);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR2_ARITHMETIC_OP(+=)
DEFINE_VECTOR2_ARITHMETIC_OP(-=)
DEFINE_VECTOR2_ARITHMETIC_OP(*=)
DEFINE_VECTOR2_ARITHMETIC_OP(/=)
#undef DEFINE_VECTOR2_ARITHMETIC_OP

#define DEFINE_VECTOR2_BIT_OP(op)                                                                                      \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator op(U scalar)                                                     \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        this->y op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator op(const vec1_t<U>& v)                                           \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec2_t<T>& vec2_t<T>::operator op(const vec2_t<U>& v)                                           \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.y);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR2_BIT_OP(%=)
DEFINE_VECTOR2_BIT_OP(&=)
DEFINE_VECTOR2_BIT_OP(|=)
DEFINE_VECTOR2_BIT_OP(^=)
DEFINE_VECTOR2_BIT_OP(<<=)
DEFINE_VECTOR2_BIT_OP(>>=)
#undef DEFINE_VECTOR2_BIT_OP

template<typename T>
BEE_FUNC constexpr vec2_t<T> operator+(const vec2_t<T>& v)
{
    return v;
}

template<typename T>
BEE_FUNC constexpr vec2_t<T> operator-(const vec2_t<T>& v)
{
    return vec2_t<T>(-v.x, -v.y);
}

template<IntegralType T>
BEE_FUNC constexpr vec2_t<T> operator~(const vec2_t<T>& v)
{
    return vec2_t<T>(~v.x, ~v.y);
}

template<typename T>
BEE_FUNC constexpr bool operator==(const vec2_t<T>& v1, const vec2_t<T>& v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

template<typename T>
BEE_FUNC constexpr bool operator!=(const vec2_t<T>& v1, const vec2_t<T>& v2)
{
    return !(v1 == v2);
}

#define DEFINE_VECTOR2_BINARY_OP(ValType, opName, op)                                                                  \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec2_t<T> operator opName(const vec2_t<T>& v, T scalar)                                         \
    {                                                                                                                  \
        return vec2_t<T>(v) op scalar;                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec2_t<T> operator opName(const vec2_t<T>& v1, const vec1_t<T>& v2)                             \
    {                                                                                                                  \
        return vec2_t<T>(v1) op v2;                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec2_t<T> operator opName(T scalar, const vec2_t<T>& v)                                         \
    {                                                                                                                  \
        return vec2_t<T>(v) op scalar;                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec2_t<T> operator opName(const vec1_t<T>& v1, const vec2_t<T>& v2)                             \
    {                                                                                                                  \
        return vec2_t<T>(v2) op v1;                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec2_t<T> operator opName(const vec2_t<T>& v1, const vec2_t<T>& v2)                             \
    {                                                                                                                  \
        return vec2_t<T>(v1) op v2;                                                                                    \
    }

DEFINE_VECTOR2_BINARY_OP(ArithmeticType, +, +=)
DEFINE_VECTOR2_BINARY_OP(ArithmeticType, -, -=)
DEFINE_VECTOR2_BINARY_OP(ArithmeticType, *, *=)
DEFINE_VECTOR2_BINARY_OP(ArithmeticType, /, /=)

DEFINE_VECTOR2_BINARY_OP(IntegralType, %, %=)
DEFINE_VECTOR2_BINARY_OP(IntegralType, &, &=)
DEFINE_VECTOR2_BINARY_OP(IntegralType, |, |=)
DEFINE_VECTOR2_BINARY_OP(IntegralType, ^, ^=)
DEFINE_VECTOR2_BINARY_OP(IntegralType, <<, <<=)
DEFINE_VECTOR2_BINARY_OP(IntegralType, >>, >>=)
#undef DEFINE_VECTOR2_BINARY_OP

BEE_FUNC constexpr vec2_t<bool> operator&&(const vec2_t<bool>& v1, const vec2_t<bool>& v2)
{
    return {v1.x && v2.x, v1.y && v2.y};
}

BEE_FUNC constexpr vec2_t<bool> operator||(const vec2_t<bool>& v1, const vec2_t<bool>& v2)
{
    return {v1.x || v2.x, v1.y || v2.y};
}
} //namespace bee