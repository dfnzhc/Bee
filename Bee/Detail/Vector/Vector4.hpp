/**
 * @File Vector4.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/30
 * @Brief 
 */

#pragma once

#include "./VectorType.hpp"

namespace bee {
template<typename T>
struct vec<4, T>
{
    using value_type = T;
    using vec_type   = vec4_t<T>;
    using bool_type  = vec4_t<bool>;

    // clang-format off
    union
    {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };
        struct { T s, t, p, q; };
    };
    // clang-format on

    BEE_FUNC constexpr vec()                   = default;
    BEE_FUNC constexpr vec(const vec4_t<T>& v) = default;

    BEE_FUNC constexpr explicit vec(T scalar);
    BEE_FUNC constexpr vec(T x, T y, T z, T w);

    template<typename U>
    BEE_FUNC constexpr explicit vec(const vec1_t<U>& v);

    // clang-format off
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(X x, Y y, Z z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, Y y, Z z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(X x, const vec1_t<Y>& y, Z z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, const vec1_t<Y>& y, Z z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(X x, Y y, const vec1_t<Z>& z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, Y y, const vec1_t<Z>& z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(X x, const vec1_t<Y>& y, const vec1_t<Z>& z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, const vec1_t<Y>& y, const vec1_t<Z>& z, W w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, Y y, Z z, const vec1_t<W>& w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(X x, const vec1_t<Y>& y, Z z, const vec1_t<W>& w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, const vec1_t<Y>& y, Z z, const vec1_t<W>& w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(X x, Y y, const vec1_t<Z>& z, const vec1_t<W>& w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, Y y, const vec1_t<Z>& z, const vec1_t<W>& w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(X x, const vec1_t<Y>& y, const vec1_t<Z>& z, const vec1_t<W>& w);
    template <typename X, typename Y, typename Z, typename W>
    BEE_FUNC constexpr vec(const vec1_t<X>& x, const vec1_t<Y>& y, const vec1_t<Z>& z, const vec1_t<W>& w);

    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec2_t<A>& xy, B z, C w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec2_t<A>& xy, const vec1_t<B>& z, C w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec2_t<A>& xy, B z, const vec1_t<C>& w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec2_t<A>& xy, const vec1_t<B>& z, const vec1_t<C>& w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(A x, const vec2_t<B>& yz, C w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec1_t<A>& x, const vec2_t<B>& yz, C w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(A x, const vec2_t<B>& yz, const vec1_t<C>& w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec1_t<A>& x, const vec2_t<B>& yz, const vec1_t<C>& w);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(A x, B y, const vec2_t<C>& zw);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(A x, const vec1_t<B>& y, const vec2_t<C>& zw);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec1_t<A>& x, B y, const vec2_t<C>& zw);
    template <typename A, typename B, typename C> BEE_FUNC constexpr vec(const vec1_t<A>& x, const vec1_t<B>& y, const vec2_t<C>& zw);
    template <typename A, typename B> BEE_FUNC constexpr vec(const vec3_t<A>& xyz, B w);
    template <typename A, typename B> BEE_FUNC constexpr vec(const vec3_t<A>& xyz, const vec1_t<B>& w);
    template <typename A, typename B> BEE_FUNC constexpr vec(A x, const vec3_t<B>& yzw);
    template <typename A, typename B> BEE_FUNC constexpr vec(const vec1_t<A>& x, const vec3_t<B>& yzw);
    template <typename A, typename B> BEE_FUNC constexpr vec(const vec2_t<A>& xy, const vec2_t<B>& zw);
    template <typename U> BEE_FUNC constexpr explicit vec(const vec4_t<U>& v);
    // clang-format on

    // -- Unary arithmetic operators --
    BEE_FUNC constexpr vec4_t<T>& operator=(const vec4_t<T>& v) = default;

    BEE_FUNC constexpr vec4_t<T>& operator++();
    BEE_FUNC constexpr vec4_t<T>& operator--();
    BEE_FUNC constexpr vec4_t<T> operator++(int);
    BEE_FUNC constexpr vec4_t<T> operator--(int);

    // clang-format off
    BEE_FUNC constexpr auto operator<=>(const vec_type& v) const noexcept;

    template<typename U> BEE_FUNC constexpr auto operator<=>(const vec4_t<U>& v) const noexcept requires Convertible<T, U>;
    template<typename U> BEE_FUNC constexpr bool  operator==(const vec4_t<U>& v) const noexcept requires Convertible<T, U>;
    // clang-format on

    // clang-format off
    template<typename U> BEE_FUNC constexpr vec4_t<T>&  operator=(const vec4_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator+=(const vec1_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator+=(const vec4_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator-=(const vec1_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator-=(const vec4_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator*=(const vec1_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator*=(const vec4_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator/=(const vec1_t<U>& v);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator/=(const vec4_t<U>& v);

    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator+=(U scalar);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator-=(U scalar);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator*=(U scalar);
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator/=(U scalar);
    // clang-format on

    // clang-format off
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator %=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator %=(const vec4_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator &=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator &=(const vec4_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator |=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator |=(const vec4_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator ^=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator ^=(const vec4_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator<<=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator<<=(const vec4_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator>>=(const vec1_t<U>& v) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator>>=(const vec4_t<U>& v) requires BothIntegral<T, U>;

    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator %=(U scalar) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator &=(U scalar) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator |=(U scalar) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator ^=(U scalar) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator<<=(U scalar) requires BothIntegral<T, U>;
    template<typename U> BEE_FUNC constexpr vec4_t<T>& operator>>=(U scalar) requires BothIntegral<T, U>;
    // clang-format on

    // clang-format off
    BEE_FUNC constexpr       T& operator[](int index)       noexcept { return (&x)[index]; }
    BEE_FUNC constexpr const T& operator[](int index) const noexcept { return (&x)[index]; }

    BEE_FUNC static constexpr int dim() noexcept { return 4; }
    
    #include "Vector4Swizzle.inl"
    // clang-format on
};
} //namespace bee

namespace bee {

template<typename T>
BEE_FUNC constexpr vec4_t<T>::vec(T scalar) : x(scalar), y(scalar), z(scalar), w(scalar)
{
}

template<typename T>
BEE_FUNC constexpr vec4_t<T>::vec(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w)
{
}

template<typename T>
template<typename U>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<U>& v)
: x(cast_to<T>(v.x)), y(cast_to<T>(v.x)), z(cast_to<T>(v.x)), w(cast_to<T>(v.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(X _x, Y _y, Z _z, W _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_y)), z(cast_to<T>(_z)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, Y _y, Z _z, W _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y)), z(cast_to<T>(_z)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(X _x, const vec1_t<Y>& _y, Z _z, W _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, const vec1_t<Y>& _y, Z _z, W _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(X _x, Y _y, const vec1_t<Z>& _z, W _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_y)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, Y _y, const vec1_t<Z>& _z, W _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(X _x, const vec1_t<Y>& _y, const vec1_t<Z>& _z, W _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, const vec1_t<Y>& _y, const vec1_t<Z>& _z, W _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, Y _y, Z _z, const vec1_t<W>& _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y)), z(cast_to<T>(_z)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(X _x, const vec1_t<Y>& _y, Z _z, const vec1_t<W>& _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, const vec1_t<Y>& _y, Z _z, const vec1_t<W>& _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(X _x, Y _y, const vec1_t<Z>& _z, const vec1_t<W>& _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_y)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, Y _y, const vec1_t<Z>& _z, const vec1_t<W>& _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(X _x, const vec1_t<Y>& _y, const vec1_t<Z>& _z, const vec1_t<W>& _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename X, typename Y, typename Z, typename W>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<X>& _x, const vec1_t<Y>& _y, const vec1_t<Z>& _z, const vec1_t<W>& _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec2_t<A>& _xy, B _z, C _w)
: x(cast_to<T>(_xy.x)), y(cast_to<T>(_xy.y)), z(cast_to<T>(_z)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec2_t<A>& _xy, const vec1_t<B>& _z, C _w)
: x(cast_to<T>(_xy.x)), y(cast_to<T>(_xy.y)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec2_t<A>& _xy, B _z, const vec1_t<C>& _w)
: x(cast_to<T>(_xy.x)), y(cast_to<T>(_xy.y)), z(cast_to<T>(_z)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec2_t<A>& _xy, const vec1_t<B>& _z, const vec1_t<C>& _w)
: x(cast_to<T>(_xy.x)), y(cast_to<T>(_xy.y)), z(cast_to<T>(_z.x)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(A _x, const vec2_t<B>& _yz, C _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_yz.x)), z(cast_to<T>(_yz.y)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<A>& _x, const vec2_t<B>& _yz, C _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_yz.x)), z(cast_to<T>(_yz.y)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(A _x, const vec2_t<B>& _yz, const vec1_t<C>& _w)
: x(cast_to<T>(_x)), y(cast_to<T>(_yz.x)), z(cast_to<T>(_yz.y)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<A>& _x, const vec2_t<B>& _yz, const vec1_t<C>& _w)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_yz.x)), z(cast_to<T>(_yz.y)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(A _x, B _y, const vec2_t<C>& _zw)
: x(cast_to<T>(_x)), y(cast_to<T>(_y)), z(cast_to<T>(_zw.x)), w(cast_to<T>(_zw.y))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<A>& _x, B _y, const vec2_t<C>& _zw)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y)), z(cast_to<T>(_zw.x)), w(cast_to<T>(_zw.y))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(A _x, const vec1_t<B>& _y, const vec2_t<C>& _zw)
: x(cast_to<T>(_x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_zw.x)), w(cast_to<T>(_zw.y))
{
}

template<typename T>
template<typename A, typename B, typename C>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<A>& _x, const vec1_t<B>& _y, const vec2_t<C>& _zw)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_y.x)), z(cast_to<T>(_zw.x)), w(cast_to<T>(_zw.y))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec4_t<T>::vec(const vec3_t<A>& _xyz, B _w)
: x(cast_to<T>(_xyz.x)), y(cast_to<T>(_xyz.y)), z(cast_to<T>(_xyz.z)), w(cast_to<T>(_w))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec4_t<T>::vec(const vec3_t<A>& _xyz, const vec1_t<B>& _w)
: x(cast_to<T>(_xyz.x)), y(cast_to<T>(_xyz.y)), z(cast_to<T>(_xyz.z)), w(cast_to<T>(_w.x))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec4_t<T>::vec(A _x, const vec3_t<B>& _yzw)
: x(cast_to<T>(_x)), y(cast_to<T>(_yzw.x)), z(cast_to<T>(_yzw.y)), w(cast_to<T>(_yzw.z))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec4_t<T>::vec(const vec1_t<A>& _x, const vec3_t<B>& _yzw)
: x(cast_to<T>(_x.x)), y(cast_to<T>(_yzw.x)), z(cast_to<T>(_yzw.y)), w(cast_to<T>(_yzw.z))
{
}

template<typename T>
template<typename A, typename B>
BEE_FUNC constexpr vec4_t<T>::vec(const vec2_t<A>& _xy, const vec2_t<B>& _zw)
: x(cast_to<T>(_xy.x)), y(cast_to<T>(_xy.y)), z(cast_to<T>(_zw.x)), w(cast_to<T>(_zw.y))
{
}

template<typename T>
template<typename U>
BEE_FUNC constexpr vec4_t<T>::vec(const vec4_t<U>& v)
: x(cast_to<T>(v.x)), y(cast_to<T>(v.y)), z(cast_to<T>(v.z)), w(cast_to<T>(v.w))
{
}

template<typename T>
template<typename U>
BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator=(const vec4_t<U>& v)
{
    this->x = cast_to<T>(v.x);
    this->y = cast_to<T>(v.y);
    this->z = cast_to<T>(v.z);
    this->w = cast_to<T>(v.w);
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator++()
{
    ++this->x;
    ++this->y;
    ++this->z;
    ++this->w;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator--()
{
    --this->x;
    --this->y;
    --this->z;
    --this->w;
    return *this;
}

template<typename T>
BEE_FUNC constexpr vec4_t<T> vec4_t<T>::operator++(int)
{
    vec4_t<T> res(*this);
    ++*this;
    return res;
}

template<typename T>
BEE_FUNC constexpr vec4_t<T> vec4_t<T>::operator--(int)
{
    vec4_t<T> res(*this);
    --*this;
    return res;
}

template<typename T>
constexpr auto vec4_t<T>::operator<=>(const vec4_t<T>& v) const noexcept
{
    if (auto cmp = x <=> v.x; cmp)
        return cmp;
    if (auto cmp = y <=> v.y; cmp)
        return cmp;
    if (auto cmp = z <=> v.z; cmp)
        return cmp;
    return w <=> v.w;
}

template<typename T>
template<typename U>
constexpr auto vec4_t<T>::operator<=>(const vec4_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return this <=> vec_type{cast_to<T>(v.x), cast_to<T>(v.y), cast_to<T>(v.z), cast_to<T>(v.w)};
}

template<typename T>
template<typename U>
constexpr bool vec4_t<T>::operator==(const vec4_t<U>& v) const noexcept
    requires Convertible<T, U>
{
    return x == cast_to<T>(v.x) && y == cast_to<T>(v.y) && z == cast_to<T>(v.z) && w == cast_to<T>(v.w);
}

#define DEFINE_VECTOR4_ARITHMETIC_OP(op)                                                                               \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator op(U scalar)                                                     \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        this->y op cast_to<T>(scalar);                                                                                 \
        this->z op cast_to<T>(scalar);                                                                                 \
        this->w op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator op(const vec1_t<U>& v)                                           \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.x);                                                                                    \
        this->z op cast_to<T>(v.x);                                                                                    \
        this->w op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator op(const vec4_t<U>& v)                                           \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.y);                                                                                    \
        this->z op cast_to<T>(v.z);                                                                                    \
        this->w op cast_to<T>(v.w);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR4_ARITHMETIC_OP(+=)
DEFINE_VECTOR4_ARITHMETIC_OP(-=)
DEFINE_VECTOR4_ARITHMETIC_OP(*=)
DEFINE_VECTOR4_ARITHMETIC_OP(/=)
#undef DEFINE_VECTOR4_ARITHMETIC_OP

#define DEFINE_VECTOR4_BIT_OP(op)                                                                                      \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator op(U scalar)                                                     \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(scalar);                                                                                 \
        this->y op cast_to<T>(scalar);                                                                                 \
        this->z op cast_to<T>(scalar);                                                                                 \
        this->w op cast_to<T>(scalar);                                                                                 \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator op(const vec1_t<U>& v)                                           \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.x);                                                                                    \
        this->z op cast_to<T>(v.x);                                                                                    \
        this->w op cast_to<T>(v.x);                                                                                    \
        return *this;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template<typename T>                                                                                               \
    template<typename U>                                                                                               \
    BEE_FUNC constexpr vec4_t<T>& vec4_t<T>::operator op(const vec4_t<U>& v)                                           \
        requires BothIntegral<T, U>                                                                                    \
    {                                                                                                                  \
        this->x op cast_to<T>(v.x);                                                                                    \
        this->y op cast_to<T>(v.y);                                                                                    \
        this->z op cast_to<T>(v.y);                                                                                    \
        this->w op cast_to<T>(v.y);                                                                                    \
        return *this;                                                                                                  \
    }

DEFINE_VECTOR4_BIT_OP(%=)
DEFINE_VECTOR4_BIT_OP(&=)
DEFINE_VECTOR4_BIT_OP(|=)
DEFINE_VECTOR4_BIT_OP(^=)
DEFINE_VECTOR4_BIT_OP(<<=)
DEFINE_VECTOR4_BIT_OP(>>=)
#undef DEFINE_VECTOR4_BIT_OP

template<typename T>
BEE_FUNC constexpr vec4_t<T> operator+(const vec4_t<T>& v)
{
    return v;
}

template<typename T>
BEE_FUNC constexpr vec4_t<T> operator-(const vec4_t<T>& v)
{
    return vec4_t<T>(0) -= v;
}

template<IntegralType T>
BEE_FUNC constexpr vec4_t<T> operator~(const vec4_t<T>& v)
{
    return vec4_t<T>{~v.x, ~v.y, ~v.z, ~v.w};
}

template<typename T>
BEE_FUNC constexpr bool operator==(const vec4_t<T>& v1, const vec4_t<T>& v2)
{
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
}

template<typename T>
BEE_FUNC constexpr bool operator!=(const vec4_t<T>& v1, const vec4_t<T>& v2)
{
    return !(v1 == v2);
}

#define DEFINE_VECTOR4_BINARY_OP(ValType, opName, op)                                                                  \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec4_t<T> operator opName(const vec4_t<T>& v, T scalar)                                         \
    {                                                                                                                  \
        return vec4_t<T>(v) op scalar;                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec4_t<T> operator opName(const vec4_t<T>& v1, const vec1_t<T>& v2)                             \
    {                                                                                                                  \
        return vec4_t<T>(v1) op v2;                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec4_t<T> operator opName(T scalar, const vec4_t<T>& v)                                         \
    {                                                                                                                  \
        return vec4_t<T>(v) op scalar;                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec4_t<T> operator opName(const vec1_t<T>& v1, const vec4_t<T>& v2)                             \
    {                                                                                                                  \
        return vec4_t<T>(v2) op v1;                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    template<ValType T>                                                                                                \
    BEE_FUNC constexpr vec4_t<T> operator opName(const vec4_t<T>& v1, const vec4_t<T>& v2)                             \
    {                                                                                                                  \
        return vec4_t<T>(v1) op v2;                                                                                    \
    }

DEFINE_VECTOR4_BINARY_OP(ArithmeticType, +, +=)
DEFINE_VECTOR4_BINARY_OP(ArithmeticType, -, -=)
DEFINE_VECTOR4_BINARY_OP(ArithmeticType, *, *=)
DEFINE_VECTOR4_BINARY_OP(ArithmeticType, /, /=)

DEFINE_VECTOR4_BINARY_OP(IntegralType, %, %=)
DEFINE_VECTOR4_BINARY_OP(IntegralType, &, &=)
DEFINE_VECTOR4_BINARY_OP(IntegralType, |, |=)
DEFINE_VECTOR4_BINARY_OP(IntegralType, ^, ^=)
DEFINE_VECTOR4_BINARY_OP(IntegralType, <<, <<=)
DEFINE_VECTOR4_BINARY_OP(IntegralType, >>, >>=)
#undef DEFINE_VECTOR4_BINARY_OP

BEE_FUNC constexpr vec4_t<bool> operator&&(const vec4_t<bool>& v1, const vec4_t<bool>& v2)
{
    return {v1.x && v2.x, v1.y && v2.y, v1.z && v2.z, v1.w && v2.w};
}

BEE_FUNC constexpr vec4_t<bool> operator||(const vec4_t<bool>& v1, const vec4_t<bool>& v2)
{
    return {v1.x || v2.x, v1.y || v2.y, v1.z || v2.z, v1.w || v2.w};
}

} //namespace bee