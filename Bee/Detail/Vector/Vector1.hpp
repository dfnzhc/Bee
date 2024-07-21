/**
 * @File Vector1.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/30
 * @Brief 
 */
#pragma once

#include "./VectorType.hpp"

namespace bee {

template<typename T>
struct vec<1, T>
{
    using value_type = T;
    using vec_type   = vec1_t<T>;
    using bool_type  = vec1_t<bool>;

    union
    {
        T x = {};
        T r;
        T s;
    };

    BEE_FUNC constexpr vec()             = default;
    BEE_FUNC constexpr vec(const vec& v) = default;

    BEE_FUNC constexpr explicit vec(T scalar) : x(scalar) { }

    // clang-format off
    template <typename U> BEE_FUNC constexpr explicit vec(const vec1_t<U>& v) : x(cast_to<T>(v.x)) {}
    template <typename U> BEE_FUNC constexpr explicit vec(const vec2_t<U>& v) : x(cast_to<T>(v.x)) {}
    template <typename U> BEE_FUNC constexpr explicit vec(const vec3_t<U>& v) : x(cast_to<T>(v.x)) {}
    template <typename U> BEE_FUNC constexpr explicit vec(const vec4_t<U>& v) : x(cast_to<T>(v.x)) {}
    // clang-format on

    BEE_FUNC constexpr vec_type& operator=(const vec& v) = default;

    BEE_FUNC constexpr vec_type& operator++();
    BEE_FUNC constexpr vec_type& operator--();
    BEE_FUNC constexpr vec_type operator++(int);
    BEE_FUNC constexpr vec_type operator--(int);

    // clang-format off
    BEE_FUNC constexpr auto operator<=>(const vec_type& v) const noexcept { return x <=> v.x; }

    template<typename U> BEE_FUNC constexpr auto operator<=>(const vec1_t<U>& v) const noexcept requires Convertible<T, U>;
    template<typename U> BEE_FUNC constexpr bool  operator==(const vec1_t<U>& v) const noexcept requires Convertible<T, U>;
    // clang-format on

    // clang-format off
    template <typename U> BEE_FUNC constexpr vec_type& operator =(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator+=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(const vec1_t<U>& v);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(const vec1_t<U>& v);

    template <typename U> BEE_FUNC constexpr vec_type& operator+=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator-=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator*=(U scalar);
    template <typename U> BEE_FUNC constexpr vec_type& operator/=(U scalar);
    // clang-format on

    // clang-format off
    template <typename U> BEE_FUNC constexpr vec_type&  operator%=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator&=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator|=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator^=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator<<=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator>>=(const vec1_t<U>& v) requires BothIntegral<T, U>;

    template <typename U> BEE_FUNC constexpr vec_type&  operator%=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator&=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator|=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type&  operator^=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator<<=(U scalar) requires BothIntegral<T, U>;
    template <typename U> BEE_FUNC constexpr vec_type& operator>>=(U scalar) requires BothIntegral<T, U>;
    // clang-format on

    // clang-format off
    BEE_FUNC constexpr       T& operator[](int)       { return x; }
    BEE_FUNC constexpr const T& operator[](int) const { return x; }

    BEE_FUNC static constexpr int dim() { return 1; }
    // clang-format on
};
} //namespace bee

namespace bee {

template<typename T>
template<typename U>
BEE_FUNC constexpr vec1_t<T>& vec1_t<T>::operator=(const vec1_t<U>& v)
{
    this->x = cast_to<T>(v.x);
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec1_t<T>& vec1_t<T>::operator++()
{
    ++this->x;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec1_t<T>& vec1_t<T>::operator--()
{
    --this->x;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec1_t<T> vec1_t<T>::operator++(int)
{
    vec1_t<T> res(*this);
    ++*this;
    return res;
}

template<typename T>
BEE_FUNC constexpr vec1_t<T> vec1_t<T>::operator--(int)
{
    vec1_t<T> res(*this);
    --*this;
    return res;
}

template<typename T>
template<typename U>
BEE_FUNC constexpr auto vec1_t<T>::operator<=>(const vec1_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return x <=> cast_to<T>(v.x);
}

template<typename T>
template<typename U>
BEE_FUNC constexpr bool vec1_t<T>::operator==(const vec1_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return x == cast_to<T>(v.x);
}

#define DEFINE_VECTOR1_ARITHMETIC_OP(op)                                                                               \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec1_t<T>& vec1_t<T>::operator op(U scalar)                                                     \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec1_t<T>& vec1_t<T>::operator op(const vec1_t<U>& v)                                           \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR1_ARITHMETIC_OP(+=)
DEFINE_VECTOR1_ARITHMETIC_OP(-=)
DEFINE_VECTOR1_ARITHMETIC_OP(*=)
DEFINE_VECTOR1_ARITHMETIC_OP(/=)
#undef DEFINE_VECTOR1_ARITHMETIC_OP

#define DEFINE_VECTOR1_BIT_OP(op)                                                                                      \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec1_t<T>& vec1_t<T>::operator op(U scalar)                                                     \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec1_t<T>& vec1_t<T>::operator op(const vec1_t<U>& v)                                           \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR1_BIT_OP(%=)
DEFINE_VECTOR1_BIT_OP(&=)
DEFINE_VECTOR1_BIT_OP(|=)
DEFINE_VECTOR1_BIT_OP(^=)
DEFINE_VECTOR1_BIT_OP(<<=)
DEFINE_VECTOR1_BIT_OP(>>=)
#undef DEFINE_VECTOR1_BIT_OP

template<typename T>
BEE_FUNC constexpr vec1_t<T> operator+(const vec1_t<T>& v)
{
    return v;
}

template<typename T>
BEE_FUNC constexpr vec1_t<T> operator-(const vec1_t<T>& v)
{
    return vec1_t<T>(-v.x);
}

template<IntegralType T>
BEE_FUNC constexpr vec1_t<T> operator~(const vec1_t<T>& v)
{
    return vec1_t<T>(~v.x);
}

template<typename T>
BEE_FUNC constexpr bool operator==(const vec1_t<T>& v1, const vec1_t<T>& v2)
{
    return v1.x == v2.x;
}

template<typename T>
BEE_FUNC constexpr bool operator!=(const vec1_t<T>& v1, const vec1_t<T>& v2)
{
    return !(v1 == v2);
}

#define DEFINE_VECTOR1_ARITHMETIC_OP(VecType, op)                                                                      \
    template<VecType T>                                                                                                \
    BEE_FUNC constexpr vec1_t<T> operator op(const vec1_t<T>& v, T scalar)                                             \
    {                                                                                                                  \
        return vec1_t<T>(cast_to<T>(v.x op scalar));                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    template<VecType T>                                                                                                \
    BEE_FUNC constexpr vec1_t<T> operator op(T scalar, const vec1_t<T>& v)                                             \
    {                                                                                                                  \
        return vec1_t<T>(cast_to<T>(scalar op v.x));                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    template<VecType T>                                                                                                \
    BEE_FUNC constexpr vec1_t<T> operator op(const vec1_t<T>& v1, const vec1_t<T>& v2)                                 \
    {                                                                                                                  \
        return vec1_t<T>(cast_to<T>(v1.x op v2.x));                                                                    \
    }

DEFINE_VECTOR1_ARITHMETIC_OP(ArithmeticType, +)
DEFINE_VECTOR1_ARITHMETIC_OP(ArithmeticType, -)
DEFINE_VECTOR1_ARITHMETIC_OP(ArithmeticType, *)
DEFINE_VECTOR1_ARITHMETIC_OP(ArithmeticType, /)

DEFINE_VECTOR1_ARITHMETIC_OP(IntegralType, %)
DEFINE_VECTOR1_ARITHMETIC_OP(IntegralType, &)
DEFINE_VECTOR1_ARITHMETIC_OP(IntegralType, |)
DEFINE_VECTOR1_ARITHMETIC_OP(IntegralType, ^)
DEFINE_VECTOR1_ARITHMETIC_OP(IntegralType, <<)
DEFINE_VECTOR1_ARITHMETIC_OP(IntegralType, >>)
#undef DEFINE_VECTOR1_ARITHMETIC_OP

BEE_FUNC constexpr vec1_t<bool> operator&&(const vec1_t<bool>& v1, const vec1_t<bool>& v2)
{
    return vec1_t<bool>(v1.x && v2.x);
}

BEE_FUNC constexpr vec1_t<bool> operator||(const vec1_t<bool>& v1, const vec1_t<bool>& v2)
{
    return vec1_t<bool>(v1.x || v2.x);
}

} //namespace bee
