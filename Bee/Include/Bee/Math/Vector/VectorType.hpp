/**
 * @File VectorType.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Container/Tuple.hpp"
#include "Bee/Core/Check.hpp"

namespace bee
{
// clang-format off
template <ArithType T> class Vector2;
template <ArithType T> class Vector3;
template <ArithType T> class Vector4;

template <ArithType T> class Point2;
template <ArithType T> class Point3;

template <ArithType T> class Normal2;
template <ArithType T> class Normal3;
// clang-format on

#define DEFINE_VECTOR_TYPE(type, suffix)  \
    using Vec2##suffix = Vector2<type>;   \
    using Vec3##suffix = Vector3<type>;   \
    using Vec4##suffix = Vector4<type>

DEFINE_VECTOR_TYPE(f32, f);
DEFINE_VECTOR_TYPE(f64, d);
DEFINE_VECTOR_TYPE(i32, i);
DEFINE_VECTOR_TYPE(u32, u);

DEFINE_VECTOR_TYPE(i8, i8);
DEFINE_VECTOR_TYPE(u8, u8);
DEFINE_VECTOR_TYPE(i16, i16);
DEFINE_VECTOR_TYPE(u16, u16);
DEFINE_VECTOR_TYPE(i64, i64);
DEFINE_VECTOR_TYPE(u64, u64);

template <typename T>
using Vec2 = Vector2<T>;
template <typename T>
using Vec3 = Vector3<T>;
template <typename T>
using Vec4 = Vector4<T>;

#undef SKT_DEFINE_VECTOR_TYPE

using Point2f = Point2<f32>;
using Point2d = Point2<f64>;
using Point3f = Point3<f32>;
using Point3d = Point3<f64>;

using Normal2f = Normal2<f32>;
using Normal2d = Normal2<f64>;
using Normal3f = Normal3<f32>;
using Normal3d = Normal3<f64>;

namespace detail
{
    // clang-format off
    template <typename T> struct is_vector : std::false_type {};
    template <ArithType U> struct is_vector<Vector2<U>> : std::true_type {};
    template <ArithType U> struct is_vector<Vector3<U>> : std::true_type {};
    template <ArithType U> struct is_vector<Vector4<U>> : std::true_type {};

    template <typename T> struct is_point : std::false_type {};
    template <ArithType U> struct is_point<Point2<U>> : std::true_type {};
    template <ArithType U> struct is_point<Point3<U>> : std::true_type {};

    template <typename T> struct is_normal : std::false_type {};
    template <ArithType U> struct is_normal<Normal2<U>> : std::true_type {};
    template <ArithType U> struct is_normal<Normal3<U>> : std::true_type {};
    // clang-format on
} // namespace detail

template <typename T>
concept VectorType = detail::is_vector<std::remove_cvref_t<T>>::value;

template <typename T>
concept PointType = detail::is_point<std::remove_cvref_t<T>>::value;

template <typename T>
concept NormalType = detail::is_normal<std::remove_cvref_t<T>>::value;

// ==================== VectorType ====================

template <ArithType T>
class Vector2 : public Tuple2<Vector2, T>
{
public:
    using Base = Tuple2<Vector2, T>;

    using Base::data;
    using Base::operator+;
    using Base::operator-;
    using Base::operator*;
    using Base::operator/;
    using Base::operator+=;
    using Base::operator-=;
    using Base::operator*=;
    using Base::operator/=;

    // clang-format off
    constexpr Vector2() = default;
    constexpr Vector2(T x_, T y_) : Base(x_, y_) {}
    
    template <ArithType U> constexpr Vector2(U x_, U y_) noexcept : Base(To<T>(x_), To<T>(y_)) {}
    
    template <ArithType U> explicit constexpr Vector2(const Vector2<U>& v) noexcept;
    template <ArithType U> explicit constexpr Vector2(const Vector3<U>& v) noexcept;
    template <ArithType U> explicit constexpr Vector2(const Vector4<U>& v) noexcept;
    
    template <ArithType U> explicit constexpr Vector2(const Point2<U>& p) noexcept;
    template <ArithType U> explicit constexpr Vector2(const Normal2<U>& n) noexcept;
    
    template <ArithType U> constexpr Vector2& operator=(const Vector2<U>& v) noexcept;

    constexpr T& operator[](int i)       { return Base::operator[](i); }
    constexpr T  operator[](int i) const { return Base::operator[](i); }
    
    constexpr T x() const noexcept { return data[0]; }
    constexpr T y() const noexcept { return data[1]; }
    
    constexpr static Vector2 Zero() { return {T{}, T{}}; }
    constexpr static Vector2 One() { return {To<T>(1), To<T>(1)}; }
    
    constexpr static Vector2 UnitX() { return {To<T>(1), T{}}; }
    constexpr static Vector2 UnitY() { return {T{}, To<T>(1)}; }

    constexpr Vector2 xx() const { return {x(), x()}; }
    constexpr Vector2 xy() const { return {x(), y()}; }
    constexpr Vector2 yx() const { return {y(), x()}; }
    constexpr Vector2 yy() const { return {y(), y()}; }
    constexpr Vector3<T> xxx()  const { return {x(), x(), x()}; }
    constexpr Vector3<T> xyy()  const { return {x(), y(), y()}; }
    constexpr Vector4<T> xyxy() const { return {x(), y(), x(), y()}; }
    // clang-format on
};

template <ArithType T>
class Vector3 : public Tuple3<Vector3, T>
{
public:
    using Base = Tuple3<Vector3, T>;

    using Base::data;
    using Base::operator+;
    using Base::operator-;
    using Base::operator*;
    using Base::operator/;
    using Base::operator+=;
    using Base::operator-=;
    using Base::operator*=;
    using Base::operator/=;

    // clang-format off
    constexpr Vector3() = default;
    constexpr Vector3(T x_, T y_, T z_) : Base(x_, y_, z_) {}
    
    template <ArithType A, ArithType B> constexpr Vector3(const Vector2<A>& xy_, B z_) noexcept;
    template <ArithType A, ArithType B> constexpr Vector3(A x_, const Vector2<B>& yz_) noexcept;
    template <ArithType U> explicit constexpr Vector3(const Vector2<U>& v) noexcept;
    template <ArithType U> explicit constexpr Vector3(const Vector3<U>& v) noexcept;
    template <ArithType U> explicit constexpr Vector3(const Vector4<U>& v) noexcept;
    
    template <ArithType A, ArithType B> explicit constexpr Vector3(const Point2<A>& p, B z_) noexcept;
    template <ArithType A, ArithType B> explicit constexpr Vector3(const Normal2<A>& n, B z_) noexcept;
    template <ArithType U> explicit constexpr Vector3(const Point3<U>& p) noexcept;
    template <ArithType U> explicit constexpr Vector3(const Normal3<U>& n) noexcept;
    
    template <ArithType U> constexpr Vector3& operator=(const Vector3<U>& v) noexcept;

    constexpr T& operator[](int i)       { return Base::operator[](i); }
    constexpr T  operator[](int i) const { return Base::operator[](i); }
    
    constexpr T x() const noexcept { return data[0]; }
    constexpr T y() const noexcept { return data[1]; }
    constexpr T z() const noexcept { return data[2]; }
    
    constexpr static Vector3 Zero() { return {T{}, T{}, T{}}; }
    constexpr static Vector3 One() { return {To<T>(1), To<T>(1), To<T>(1)}; }
    
    constexpr static Vector3 UnitX() { return {To<T>(1), T{}, T{}}; }
    constexpr static Vector3 UnitY() { return {T{}, To<T>(1), T{}}; }
    constexpr static Vector3 UnitZ() { return {T{}, T{}, To<T>(1)}; }

    constexpr Vector2<T> xx() const { return {x(), x()}; }
    constexpr Vector2<T> xy() const { return {x(), y()}; }
    constexpr Vector2<T> xz() const { return {x(), z()}; }
    constexpr Vector2<T> yx() const { return {y(), x()}; }
    constexpr Vector2<T> yy() const { return {y(), y()}; }
    constexpr Vector2<T> yz() const { return {y(), z()}; }
    constexpr Vector2<T> zx() const { return {z(), x()}; }
    constexpr Vector2<T> zy() const { return {z(), y()}; }
    constexpr Vector2<T> zz() const { return {z(), z()}; }

    constexpr Vector3 xxx() const { return {x(), x(), x()}; }
    constexpr Vector3 xxy() const { return {x(), x(), y()}; }
    constexpr Vector3 xxz() const { return {x(), x(), z()}; }
    constexpr Vector3 xyx() const { return {x(), y(), x()}; }
    constexpr Vector3 xyy() const { return {x(), y(), y()}; }
    constexpr Vector3 xyz() const { return {x(), y(), z()}; }
    constexpr Vector3 xzx() const { return {x(), z(), x()}; }
    constexpr Vector3 xzy() const { return {x(), z(), y()}; }
    constexpr Vector3 xzz() const { return {x(), z(), z()}; }
    constexpr Vector3 yxx() const { return {y(), x(), x()}; }
    constexpr Vector3 yxy() const { return {y(), x(), y()}; }
    constexpr Vector3 yxz() const { return {y(), x(), z()}; }
    constexpr Vector3 yyx() const { return {y(), y(), x()}; }
    constexpr Vector3 yyy() const { return {y(), y(), y()}; }
    constexpr Vector3 yyz() const { return {y(), y(), z()}; }
    constexpr Vector3 yzx() const { return {y(), z(), x()}; }
    constexpr Vector3 yzy() const { return {y(), z(), y()}; }
    constexpr Vector3 yzz() const { return {y(), z(), z()}; }
    constexpr Vector3 zxx() const { return {z(), x(), x()}; }
    constexpr Vector3 zxy() const { return {z(), x(), y()}; }
    constexpr Vector3 zxz() const { return {z(), x(), z()}; }
    constexpr Vector3 zyx() const { return {z(), y(), x()}; }
    constexpr Vector3 zyy() const { return {z(), y(), y()}; }
    constexpr Vector3 zyz() const { return {z(), y(), z()}; }
    constexpr Vector3 zzx() const { return {z(), z(), x()}; }
    constexpr Vector3 zzy() const { return {z(), z(), y()}; }
    constexpr Vector3 zzz() const { return {z(), z(), z()}; }
    // clang-format on
};

template <ArithType T>
class Vector4 : public Tuple4<Vector4, T>
{
public:
    using Base = Tuple4<Vector4, T>;

    using Base::data;
    using Base::operator+;
    using Base::operator-;
    using Base::operator*;
    using Base::operator/;
    using Base::operator+=;
    using Base::operator-=;
    using Base::operator*=;
    using Base::operator/=;

    // clang-format off
    constexpr Vector4() = default;
    constexpr Vector4(T x_, T y_, T z_, T w_) : Base(x_, y_, z_, w_) {}
    
    template <ArithType A, ArithType B, ArithType C> constexpr Vector4(const Vector2<A>& xy_, B z_, C w_) noexcept;
    template <ArithType A, ArithType B, ArithType C> constexpr Vector4(A x_, const Vector2<B>& yz_, C w_) noexcept;
    template <ArithType A, ArithType B, ArithType C> constexpr Vector4(A x_, B y_, const Vector2<C>& zw_) noexcept;
    template <ArithType A, ArithType B> constexpr Vector4(const Vector3<A>& xyz_, B w_) noexcept;
    template <ArithType A, ArithType B> constexpr Vector4(A x_, const Vector3<B>& yzw_) noexcept;
    template <ArithType A, ArithType B> constexpr Vector4(const Vector2<A>& xy_, const Vector2<B>& zw_) noexcept;
    template <ArithType U> explicit constexpr Vector4(const Vector2<U>& v) noexcept;
    template <ArithType U> explicit constexpr Vector4(const Vector3<U>& v) noexcept;
    template <ArithType U> explicit constexpr Vector4(const Vector4<U>& v) noexcept;
    
    template <ArithType A, ArithType B> explicit constexpr Vector4(const Point3<A>& p, B w_) noexcept;
    template <ArithType A, ArithType B> explicit constexpr Vector4(const Normal3<A>& n, B w_) noexcept;
    
    template <ArithType U> constexpr Vector4& operator=(const Vector4<U>& v) noexcept;

    constexpr T& operator[](int i)       { return Base::operator[](i); }
    constexpr T  operator[](int i) const { return Base::operator[](i); }
    
    constexpr T x() const noexcept { return data[0]; }
    constexpr T y() const noexcept { return data[1]; }
    constexpr T z() const noexcept { return data[2]; }
    constexpr T w() const noexcept { return data[3]; }
    
    constexpr static Vector4 Zero() { return {T{}, T{}, T{}, T{}}; }
    constexpr static Vector4 One() { return {To<T>(1), To<T>(1), To<T>(1), To<T>(1)}; }
    
    constexpr static Vector4 UnitX() { return {To<T>(1), T{}, T{}, T{}}; }
    constexpr static Vector4 UnitY() { return {T{}, To<T>(1), T{}, T{}}; }
    constexpr static Vector4 UnitZ() { return {T{}, T{}, To<T>(1), T{}}; }
    constexpr static Vector4 UnitW() { return {T{}, T{}, T{}, To<T>(1)}; }

    constexpr Vector2<T> xx() const { return {x(), x()}; }
    constexpr Vector2<T> xy() const { return {x(), y()}; }
    constexpr Vector2<T> xz() const { return {x(), z()}; }
    constexpr Vector2<T> xw() const { return {x(), w()}; }
    constexpr Vector2<T> yx() const { return {y(), x()}; }
    constexpr Vector2<T> yy() const { return {y(), y()}; }
    constexpr Vector2<T> yz() const { return {y(), z()}; }
    constexpr Vector2<T> yw() const { return {y(), w()}; }
    constexpr Vector2<T> zx() const { return {z(), x()}; }
    constexpr Vector2<T> zy() const { return {z(), y()}; }
    constexpr Vector2<T> zz() const { return {z(), z()}; }
    constexpr Vector2<T> zw() const { return {z(), w()}; }
    constexpr Vector2<T> wx() const { return {w(), x()}; }
    constexpr Vector2<T> wy() const { return {w(), y()}; }
    constexpr Vector2<T> wz() const { return {w(), z()}; }
    constexpr Vector2<T> ww() const { return {w(), w()}; }

    constexpr Vector3<T> xyz() const { return {x(), y(), z()}; }
    constexpr Vector3<T> xzy() const { return {x(), z(), y()}; }
    constexpr Vector3<T> xyw() const { return {x(), y(), w()}; }
    constexpr Vector3<T> xwy() const { return {x(), w(), y()}; }
    constexpr Vector3<T> xzw() const { return {x(), z(), w()}; }
    constexpr Vector3<T> xwz() const { return {x(), w(), z()}; }
    constexpr Vector3<T> yxz() const { return {y(), x(), z()}; }
    constexpr Vector3<T> yzx() const { return {y(), z(), x()}; }
    constexpr Vector3<T> yxw() const { return {y(), x(), w()}; }
    constexpr Vector3<T> ywx() const { return {y(), w(), x()}; }
    constexpr Vector3<T> yzw() const { return {y(), z(), w()}; }
    constexpr Vector3<T> ywz() const { return {y(), w(), z()}; }
    constexpr Vector3<T> zxy() const { return {z(), x(), y()}; }
    constexpr Vector3<T> zyx() const { return {z(), y(), x()}; }
    constexpr Vector3<T> zxw() const { return {z(), x(), w()}; }
    constexpr Vector3<T> zwx() const { return {z(), w(), x()}; }
    constexpr Vector3<T> zyw() const { return {z(), y(), w()}; }
    constexpr Vector3<T> zwy() const { return {z(), w(), y()}; }
    constexpr Vector3<T> wxy() const { return {w(), x(), y()}; }
    constexpr Vector3<T> wyx() const { return {w(), y(), x()}; }
    constexpr Vector3<T> wxz() const { return {w(), x(), z()}; }
    constexpr Vector3<T> wzx() const { return {w(), z(), x()}; }
    constexpr Vector3<T> wyz() const { return {w(), y(), z()}; }
    constexpr Vector3<T> wzy() const { return {w(), z(), y()}; }
    
    constexpr Vector4 xxxx() const noexcept { return {x(), x(), x(), x()}; }
    constexpr Vector4 xxxy() const noexcept { return {x(), x(), x(), y()}; }
    constexpr Vector4 xxyx() const noexcept { return {x(), x(), y(), x()}; }
    constexpr Vector4 xxyy() const noexcept { return {x(), x(), y(), y()}; }
    constexpr Vector4 xyxx() const noexcept { return {x(), y(), x(), x()}; }
    constexpr Vector4 xyxy() const noexcept { return {x(), y(), x(), y()}; }
    constexpr Vector4 xyyx() const noexcept { return {x(), y(), y(), x()}; }
    constexpr Vector4 xyyy() const noexcept { return {x(), y(), y(), y()}; }
    constexpr Vector4 yxxx() const noexcept { return {y(), x(), x(), x()}; }
    constexpr Vector4 yxxy() const noexcept { return {y(), x(), x(), y()}; }
    constexpr Vector4 yxyx() const noexcept { return {y(), x(), y(), x()}; }
    constexpr Vector4 yxyy() const noexcept { return {y(), x(), y(), y()}; }
    constexpr Vector4 yyxx() const noexcept { return {y(), y(), x(), x()}; }
    constexpr Vector4 yyxy() const noexcept { return {y(), y(), x(), y()}; }
    constexpr Vector4 yyyx() const noexcept { return {y(), y(), y(), x()}; }
    constexpr Vector4 yyyy() const noexcept { return {y(), y(), y(), y()}; }
    // clang-format on
};

// ==================== PointType ====================

template <ArithType T>
class Point2 : public Tuple2<Point2, T>
{
public:
    using Base = Tuple2<Point2, T>;
    using Base::Base;
    using Base::data;

    // clang-format off
    constexpr Point2() = default;
    constexpr Point2(T x_, T y_) : Base(x_, y_) {}
    
    constexpr explicit Point2(const Vector2<T>& v);
    template <ArithType U> constexpr explicit Point2(const Vector2<U>& v);

    template <ArithType U> constexpr auto operator+(const Vector2<U>& v) const -> Point2<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator-(const Vector2<U>& v) const -> Point2<decltype(T{} - U{})>;
    template <ArithType U> constexpr auto operator-(const Point2<U>& p) const -> Vector2<decltype(T{} - U{})>;
    
    template <ArithType U> constexpr Point2& operator=(const Point2<U>& p) noexcept;

    template <ArithType U> constexpr auto operator+(Point2<U> p) const = delete;
    template <ArithType U> constexpr auto operator*(U s) const = delete;
    template <ArithType U> constexpr auto operator/(U d) const = delete;
    template <ArithType U> constexpr Point2& operator+=(Point2<U> p) = delete;
    template <ArithType U> constexpr Point2& operator-=(Point2<U> p) = delete;
    template <ArithType U> constexpr Point2& operator*=(U s) = delete;
    template <ArithType U> constexpr Point2& operator/=(U d) = delete;

    constexpr T x() const noexcept { return data[0]; }
    constexpr T y() const noexcept { return data[1]; }

    constexpr static Point2 Origin() { return {T{}, T{}}; }
    // clang-format on
};

template <ArithType T>
class Point3 : public Tuple3<Point3, T>
{
public:
    using Base = Tuple3<Point3, T>;
    using Base::Base;
    using Base::data;

    // clang-format off
    constexpr Point3() = default;
    constexpr Point3(T x_, T y_, T z_) : Base(x_, y_, z_) {}
    
    constexpr explicit Point3(const Vector3<T>& v);
    template <ArithType U> constexpr explicit Point3(const Vector3<U>& v);

    template <ArithType U> constexpr auto operator+(const Vector3<U>& v) const -> Point3<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator-(const Vector3<U>& v) const -> Point3<decltype(T{} - U{})>;
    template <ArithType U> constexpr auto operator-(const Point3<U>& p) const -> Vector3<decltype(T{} - U{})>;
    
    template <ArithType U> constexpr Point3& operator=(const Point3<U>& p) noexcept;

    template <ArithType U> constexpr auto operator+(Point3<U> p) const = delete;
    template <ArithType U> constexpr auto operator*(U s) const = delete;
    template <ArithType U> constexpr auto operator/(U d) const = delete;
    template <ArithType U> constexpr Point3& operator+=(Point3<U> p) = delete;
    template <ArithType U> constexpr Point3& operator-=(Point3<U> p) = delete;
    template <ArithType U> constexpr Point3& operator*=(U s) = delete;
    template <ArithType U> constexpr Point3& operator/=(U d) = delete;
    
    constexpr T x() const noexcept { return data[0]; }
    constexpr T y() const noexcept { return data[1]; }
    constexpr T z() const noexcept { return data[2]; }

    constexpr static Point3 Origin() { return {T{}, T{}, T{}}; }
    // clang-format on
};

// ==================== NormalType ====================

template <ArithType T>
class Normal2 : public Tuple2<Normal2, T>
{
public:
    using Base = Tuple2<Normal2, T>;
    using Base::Base;
    using Base::data;

    // clang-format off
    constexpr Normal2() = default;
    constexpr Normal2(T x_, T y_) : Base(x_, y_) {}
    
    constexpr explicit Normal2(const Vector2<T>& v);
    template <ArithType U> constexpr explicit Normal2(const Vector2<U>& v);
    template <ArithType U> constexpr Normal2& operator=(const Normal2<U>& n) noexcept;
    
    constexpr auto normalize() const -> Normal2<MapFloatType<T>>;
    constexpr bool isNormalized() const;
    
    constexpr T x() const noexcept { return data[0]; }
    constexpr T y() const noexcept { return data[1]; }

    constexpr static Normal2 UnitX() { return {To<T>(1), T{}}; }
    constexpr static Normal2 UnitY() { return {T{}, To<T>(1)}; }
    // clang-format on
};

template <ArithType T>
class Normal3 : public Tuple3<Normal3, T>
{
public:
    using Base = Tuple3<Normal3, T>;
    using Base::Base;
    using Base::data;

    // clang-format off
    constexpr Normal3() = default;
    constexpr Normal3(T x_, T y_, T z_) : Base(x_, y_, z_) {}
    
    constexpr explicit Normal3(const Vector3<T>& v);
    template <ArithType U> constexpr explicit Normal3(const Vector3<U>& v);
    template <ArithType U> constexpr Normal3& operator=(const Normal3<U>& n) noexcept;
    
    constexpr auto normalize() const -> Normal3<MapFloatType<T>>;
    constexpr bool isNormalized() const;
    
    constexpr void set(T x_, T y_, T z_) { data[0] = x_; data[1] = y_, data[2] = z_; }
    
    constexpr T x() const noexcept { return data[0]; }
    constexpr T y() const noexcept { return data[1]; }
    constexpr T z() const noexcept { return data[2]; }

    constexpr static Normal3 UnitX() noexcept { return {To<T>(1), T{}, T{}}; }
    constexpr static Normal3 UnitY() noexcept { return {T{}, To<T>(1), T{}}; }
    constexpr static Normal3 UnitZ() noexcept { return {T{}, T{}, To<T>(1)}; }
    // clang-format on
};

// ========================================

template <ArithType T>
template <ArithType U>
constexpr Vector2<T>::Vector2(const Vector2<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector2<T>::Vector2(const Vector3<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector2<T>::Vector2(const Vector4<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector2<T>::Vector2(const Point2<U>& p) noexcept :
    Base(To<T>(p.x()), To<T>(p.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector2<T>::Vector2(const Normal2<U>& n) noexcept :
    Base(To<T>(n.x()), To<T>(n.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector2<T>& Vector2<T>::operator=(const Vector2<U>& v) noexcept
{
    BEE_DCHECK(!v.hasNaN());
    data[0] = To<T>(v.x());
    data[1] = To<T>(v.y());
    return *this;
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector3<T>::Vector3(const Vector2<A>& xy_, B z_) noexcept :
    Base(To<T>(xy_.x()), To<T>(xy_.y()), To<T>(z_))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector3<T>::Vector3(A x_, const Vector2<B>& yz_) noexcept :
    Base(To<T>(x_), To<T>(yz_.x()), To<T>(yz_.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector3<T>::Vector3(const Vector2<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()), T{})
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector3<T>::Vector3(const Vector3<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()), To<T>(v.z()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector3<T>::Vector3(const Vector4<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()), To<T>(v.z()))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector3<T>::Vector3(const Point2<A>& p, B z_) noexcept :
    Base(To<T>(p.x()), To<T>(p.y()), To<T>(z_))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector3<T>::Vector3(const Normal2<A>& n, B z_) noexcept :
    Base(To<T>(n.x()), To<T>(n.y()), To<T>(z_))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector3<T>::Vector3(const Point3<U>& p) noexcept :
    Base(To<T>(p.x()), To<T>(p.y()), To<T>(p.z()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector3<T>::Vector3(const Normal3<U>& n) noexcept :
    Base(To<T>(n.x()), To<T>(n.y()), To<T>(n.z()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector3<T>& Vector3<T>::operator=(const Vector3<U>& v) noexcept
{
    BEE_DCHECK(!v.hasNaN());
    data[0] = To<T>(v.x());
    data[1] = To<T>(v.y());
    data[2] = To<T>(v.z());
    return *this;
}

template <ArithType T>
template <ArithType A, ArithType B, ArithType C>
constexpr Vector4<T>::Vector4(A x_, const Vector2<B>& yz_, C w_) noexcept :
    Base(To<T>(x_), To<T>(yz_.x()), To<T>(yz_.y()), To<T>(w_))
{
}

template <ArithType T>
template <ArithType A, ArithType B, ArithType C>
constexpr Vector4<T>::Vector4(const Vector2<A>& xy_, B z_, C w_) noexcept :
    Base(To<T>(xy_.x()), To<T>(xy_.y()), To<T>(z_), To<T>(w_))
{
}

template <ArithType T>
template <ArithType A, ArithType B, ArithType C>
constexpr Vector4<T>::Vector4(A x_, B y_, const Vector2<C>& zw_) noexcept :
    Base(To<T>(x_), To<T>(y_), To<T>(zw_.x()), To<T>(zw_.y()))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector4<T>::Vector4(const Vector3<A>& xyz_, B w_) noexcept :
    Base(To<T>(xyz_.x()), To<T>(xyz_.y()), To<T>(xyz_.z()), To<T>(w_))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector4<T>::Vector4(A x_, const Vector3<B>& yzw_) noexcept :
    Base(To<T>(x_), To<T>(yzw_.x()), To<T>(yzw_.y()), To<T>(yzw_.z()))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector4<T>::Vector4(const Vector2<A>& xy_, const Vector2<B>& zw_) noexcept :
    Base(To<T>(xy_.x()), To<T>(xy_.y()), To<T>(zw_.x()), To<T>(zw_.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector4<T>::Vector4(const Vector2<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()), T{}, T{})
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector4<T>::Vector4(const Vector3<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()), To<T>(v.z()), T{})
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector4<T>::Vector4(const Vector4<U>& v) noexcept :
    Base(To<T>(v.x()), To<T>(v.y()), To<T>(v.z()), To<T>(v.w()))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector4<T>::Vector4(const Point3<A>& p, B w_) noexcept :
    Base(To<T>(p.x()), To<T>(p.y()), To<T>(p.z()), To<T>(w_))
{
}

template <ArithType T>
template <ArithType A, ArithType B>
constexpr Vector4<T>::Vector4(const Normal3<A>& n, B w_) noexcept :
    Base(To<T>(n.x()), To<T>(n.y()), To<T>(n.z()), To<T>(w_))
{
}

template <ArithType T>
template <ArithType U>
constexpr Vector4<T>& Vector4<T>::operator=(const Vector4<U>& v) noexcept
{
    BEE_DCHECK(!v.hasNaN());
    data[0] = To<T>(v.x());
    data[1] = To<T>(v.y());
    data[2] = To<T>(v.z());
    data[3] = To<T>(v.w());
    return *this;
}

template <ArithType T>
constexpr Point2<T>::Point2(const Vector2<T>& v) :
    Base(v.x(), v.y())
{
}

template <ArithType T>
template <ArithType U>
constexpr Point2<T>::Point2(const Vector2<U>& v) :
    Base(To<T>(v.x()), To<T>(v.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr auto Point2<T>::operator+(const Vector2<U>& v) const -> Point2<decltype(T{} + U{})>
{
    using R = decltype(T{} + U{});
    return {To<R>(x()) + To<R>(v.x()), To<R>(y()) + To<R>(v.y())};
}

template <ArithType T>
template <ArithType U>
constexpr auto Point2<T>::operator-(const Vector2<U>& v) const -> Point2<decltype(T{} - U{})>
{
    using R = decltype(T{} - U{});
    return {To<R>(x()) - To<R>(v.x()), To<R>(y()) - To<R>(v.y())};
}

template <ArithType T>
template <ArithType U>
constexpr auto Point2<T>::operator-(const Point2<U>& p) const -> Vector2<decltype(T{} - U{})>
{
    using R = decltype(T{} - U{});
    return {To<R>(x()) - To<R>(p.x()), To<R>(y()) - To<R>(p.y())};
}

template <ArithType T>
template <ArithType U>
constexpr Point2<T>& Point2<T>::operator=(const Point2<U>& p) noexcept
{
    BEE_DCHECK(!p.hasNaN());
    data[0] = To<T>(p.x());
    data[1] = To<T>(p.y());
    return *this;
}

template <ArithType T>
constexpr Point3<T>::Point3(const Vector3<T>& v) :
    Base(v.x(), v.y(), v.z())
{
}

template <ArithType T>
template <ArithType U>
constexpr Point3<T>::Point3(const Vector3<U>& v) :
    Base(To<T>(v.x()), To<T>(v.y()), To<T>(v.z()))
{
}

template <ArithType T>
template <ArithType U>
constexpr auto Point3<T>::operator+(const Vector3<U>& v) const -> Point3<decltype(T{} + U{})>
{
    using R = decltype(T{} + U{});
    return {To<R>(x()) + To<R>(v.x()), To<R>(y()) + To<R>(v.y()), To<R>(z()) + To<R>(v.z())};
}

template <ArithType T>
template <ArithType U>
constexpr auto Point3<T>::operator-(const Vector3<U>& v) const -> Point3<decltype(T{} - U{})>
{
    using R = decltype(T{} - U{});
    return {To<R>(x()) - To<R>(v.x()), To<R>(y()) - To<R>(v.y()), To<R>(z()) - To<R>(v.z())};
}

template <ArithType T>
template <ArithType U>
constexpr auto Point3<T>::operator-(const Point3<U>& p) const -> Vector3<decltype(T{} - U{})>
{
    using R = decltype(T{} - U{});
    return {To<R>(x()) - To<R>(p.x()), To<R>(y()) - To<R>(p.y()), To<R>(z()) - To<R>(p.z())};
}

template <ArithType T>
template <ArithType U>
constexpr Point3<T>& Point3<T>::operator=(const Point3<U>& p) noexcept
{
    BEE_DCHECK(!p.hasNaN());
    data[0] = To<T>(p.x());
    data[1] = To<T>(p.y());
    data[2] = To<T>(p.z());
    return *this;
}

template <ArithType T>
constexpr Normal2<T>::Normal2(const Vector2<T>& v) :
    Base(v.x(), v.y())
{
}

template <ArithType T>
template <ArithType U>
constexpr Normal2<T>::Normal2(const Vector2<U>& v) :
    Base(To<T>(v.x()), To<T>(v.y()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Normal2<T>& Normal2<T>::operator=(const Normal2<U>& n) noexcept
{
    BEE_DCHECK(!n.hasNaN());
    data[0] = To<T>(n.x());
    data[1] = To<T>(n.y());
    return *this;
}

template <ArithType T>
constexpr auto Normal2<T>::normalize() const -> Normal2<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = std::sqrt(To<R>(x()) * To<R>(x()) + To<R>(y()) * To<R>(y()));

    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / len;
    return {To<R>(x()) * inv, To<R>(y()) * inv};
}

template <ArithType T>
constexpr bool Normal2<T>::isNormalized() const
{
    using R  = MapFloatType<T>;
    auto len = std::sqrt(To<R>(x()) * To<R>(x()) + To<R>(y()) * To<R>(y()));
    return std::abs(len - To<R>(1)) <= To<R>(1e-6);
}

template <ArithType T>
constexpr Normal3<T>::Normal3(const Vector3<T>& v) :
    Base(v.x(), v.y(), v.z())
{
}

template <ArithType T>
template <ArithType U>
constexpr Normal3<T>::Normal3(const Vector3<U>& v) :
    Base(To<T>(v.x()), To<T>(v.y()), To<T>(v.z()))
{
}

template <ArithType T>
template <ArithType U>
constexpr Normal3<T>& Normal3<T>::operator=(const Normal3<U>& n) noexcept
{
    BEE_DCHECK(!n.hasNaN());
    data[0] = To<T>(n.x());
    data[1] = To<T>(n.y());
    data[2] = To<T>(n.z());
    return *this;
}

template <ArithType T>
constexpr auto Normal3<T>::normalize() const -> Normal3<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = std::sqrt(To<R>(x()) * To<R>(x()) + To<R>(y()) * To<R>(y()) + To<R>(z()) * To<R>(z()));

    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / len;
    return {To<R>(x()) * inv, To<R>(y()) * inv, To<R>(z()) * inv};
}

template <ArithType T>
constexpr bool Normal3<T>::isNormalized() const
{
    using R  = MapFloatType<T>;
    auto len = std::sqrt(To<R>(x()) * To<R>(x()) + To<R>(y()) * To<R>(y()) + To<R>(z()) * To<R>(z()));
    return std::abs(len - To<R>(1)) <= To<R>(1e-6);
}

} // namespace bee
