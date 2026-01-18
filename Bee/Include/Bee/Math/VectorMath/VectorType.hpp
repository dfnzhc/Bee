/**
 * @File VectorType.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Core/Arithmetic.hpp"
#include "Bee/Core/Check.hpp"
#include "Bee/Core/Concepts.hpp"
#include "Bee/Container/Tuple.hpp"

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

#undef DEFINE_VECTOR_TYPE

// 向量类型别名
using Vector2f = Vector2<f32>;
using Vector2d = Vector2<f64>;
using Vector3f = Vector3<f32>;
using Vector3d = Vector3<f64>;
using Vector4f = Vector4<f32>;
using Vector4d = Vector4<f64>;

// 简短别名
using Vec2f = Vector2<f32>;
using Vec2d = Vector2<f64>;
using Vec3f = Vector3<f32>;
using Vec3d = Vector3<f64>;
using Vec4f = Vector4<f32>;
using Vec4d = Vector4<f64>;

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
class Vector2 : public Tuple<Vector2, T, 2>
{
public:
    using Base = Tuple<Vector2, T, 2>;

    // clang-format off
    // ========== 数据成员 ==========
    T x, y;

    // ========== 构造函数 ==========
    constexpr Vector2() noexcept : x{}, y{} {}

    constexpr Vector2(T x_, T y_) noexcept : x(x_), y(y_)
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr Vector2(U x_, U y_) noexcept : x(To<T>(x_)), y(To<T>(y_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // 类型转换构造
    template <ArithType U>
    constexpr explicit Vector2(const Vector2<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector2(const Vector3<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector2(const Vector4<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector2(const Point2<U>& p) noexcept : x(To<T>(p.x)), y(To<T>(p.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector2(const Normal2<U>& n) noexcept : x(To<T>(n.x)), y(To<T>(n.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // ========== 赋值 ==========
    template <ArithType U>
    constexpr Vector2& operator=(const Vector2<U>& v) noexcept
    {
        BEE_DCHECK(!v.hasNaN());
        x = To<T>(v.x);
        y = To<T>(v.y);
        return *this;
    }

    // ========== 访问接口 ==========
    constexpr T& operator[](int i) noexcept
    {
        BEE_DCHECK(i >= 0 && i < 2);
        return (&x)[i];
    }

    constexpr T operator[](int i) const noexcept
    {
        BEE_DCHECK(i >= 0 && i < 2);
        return (&x)[i];
    }

    constexpr       T* data()       noexcept { return &x; }
    constexpr const T* data() const noexcept { return &x; }

    // ========== 工具方法 ==========
    constexpr bool isZero(MapFloatType<T> epsilon = To<MapFloatType<T>>((kEpsilonF + kEpsilonD) * 0.5)) const noexcept
    {
        using R = MapFloatType<T>;
        return std::abs(To<R>(x)) < epsilon && std::abs(To<R>(y)) < epsilon;
    }

    constexpr bool hasValidLength() const noexcept
    {
        using R = MapFloatType<T>;
        const R lenSq = To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y);
        return !IsNaN(lenSq) && !IsInf(lenSq);
    }

    // ========== 导入 Tuple 运算符 ==========
    using Base::operator+;
    using Base::operator-;
    using Base::operator*;
    using Base::operator/;
    using Base::operator+=;
    using Base::operator-=;
    using Base::operator*=;
    using Base::operator/=;

    // ========== 静态工厂 ==========
    constexpr static Vector2 Zero() noexcept { return {T{}, T{}}; }
    constexpr static Vector2 One() noexcept { return {To<T>(1), To<T>(1)}; }
    constexpr static Vector2 UnitX() noexcept { return {To<T>(1), T{}}; }
    constexpr static Vector2 UnitY() noexcept { return {T{}, To<T>(1)}; }

    // ========== Swizzle 方法 ==========
    // Vector2 -> Vector2
    constexpr Vector2 xx() const noexcept { return {x, x}; }
    constexpr Vector2 xy() const noexcept { return {x, y}; }
    constexpr Vector2 yx() const noexcept { return {y, x}; }
    constexpr Vector2 yy() const noexcept { return {y, y}; }

    // Vector2 -> Vector3
    constexpr Vector3<T> xxx() const noexcept { return {x, x, x}; }
    constexpr Vector3<T> xyy() const noexcept { return {x, y, y}; }

    // Vector2 -> Vector4
    constexpr Vector4<T> xyxy() const noexcept { return {x, y, x, y}; }
    constexpr Vector4<T> yxxy() const noexcept { return {y, x, x, y}; }
    // clang-format on
};

template <ArithType T>
class Vector3 : public Tuple<Vector3, T, 3>
{
public:
    using Base = Tuple<Vector3, T, 3>;

    // clang-format off
    // ========== 数据成员 ==========
    T x, y, z;

    // ========== 构造函数 ==========
    constexpr Vector3() noexcept : x{}, y{}, z{} {}

    constexpr Vector3(T x_, T y_, T z_) noexcept : x(x_), y(y_), z(z_)
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // 从 Vector2 扩展
    template <ArithType A, ArithType B>
    constexpr Vector3(const Vector2<A>& xy_, B z_) noexcept
        : x(To<T>(xy_.x)), y(To<T>(xy_.y)), z(To<T>(z_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr Vector3(A x_, const Vector2<B>& yz_) noexcept
        : x(To<T>(x_)), y(To<T>(yz_.x)), z(To<T>(yz_.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // 类型转换
    template <ArithType U>
    constexpr explicit Vector3(const Vector2<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y)), z(T{})
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector3(const Vector3<U>& v) noexcept
        : x(To<T>(v.x)), y(To<T>(v.y)), z(To<T>(v.z))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector3(const Vector4<U>& v) noexcept
        : x(To<T>(v.x)), y(To<T>(v.y)), z(To<T>(v.z))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr explicit Vector3(const Point2<A>& p, B z_) noexcept
        : x(To<T>(p.x)), y(To<T>(p.y)), z(To<T>(z_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr explicit Vector3(const Normal2<A>& n, B z_) noexcept
        : x(To<T>(n.x)), y(To<T>(n.y)), z(To<T>(z_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector3(const Point3<U>& p) noexcept
        : x(To<T>(p.x)), y(To<T>(p.y)), z(To<T>(p.z))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector3(const Normal3<U>& n) noexcept
        : x(To<T>(n.x)), y(To<T>(n.y)), z(To<T>(n.z))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // ========== 赋值 ==========
    template <ArithType U>
    constexpr Vector3& operator=(const Vector3<U>& v) noexcept
    {
        BEE_DCHECK(!v.hasNaN());
        x = To<T>(v.x);
        y = To<T>(v.y);
        z = To<T>(v.z);
        return *this;
    }

    // ========== 访问接口 ==========
    constexpr T& operator[](int i) noexcept
    {
        BEE_DCHECK(i >= 0 && i < 3);
        return (&x)[i];
    }

    constexpr T operator[](int i) const noexcept
    {
        BEE_DCHECK(i >= 0 && i < 3);
        return (&x)[i];
    }

    constexpr       T* data()       noexcept { return &x; }
    constexpr const T* data() const noexcept { return &x; }

    // ========== 工具方法 ==========
    constexpr bool isZero(MapFloatType<T> epsilon = To<MapFloatType<T>>((kEpsilonF + kEpsilonD) * 0.5)) const noexcept
    {
        using R = MapFloatType<T>;
        return std::abs(To<R>(x)) < epsilon && std::abs(To<R>(y)) < epsilon && std::abs(To<R>(z)) < epsilon;
    }

    constexpr bool hasValidLength() const noexcept
    {
        using R = MapFloatType<T>;
        const R lenSq = To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y) + To<R>(z) * To<R>(z);
        return !IsNaN(lenSq) && !IsInf(lenSq);
    }

    // ========== 导入 Tuple 运算符 ==========
    using Base::operator+;
    using Base::operator-;
    using Base::operator*;
    using Base::operator/;
    using Base::operator+=;
    using Base::operator-=;
    using Base::operator*=;
    using Base::operator/=;

    // ========== 静态工厂 ==========
    constexpr static Vector3 Zero() noexcept { return {T{}, T{}, T{}}; }
    constexpr static Vector3 One() noexcept { return {To<T>(1), To<T>(1), To<T>(1)}; }
    constexpr static Vector3 UnitX() noexcept { return {To<T>(1), T{}, T{}}; }
    constexpr static Vector3 UnitY() noexcept { return {T{}, To<T>(1), T{}}; }
    constexpr static Vector3 UnitZ() noexcept { return {T{}, T{}, To<T>(1)}; }

    // ========== Swizzle 方法  ==========
    // Vector3 -> Vector2
    constexpr Vector2<T> xx() const noexcept { return {x, x}; }
    constexpr Vector2<T> xy() const noexcept { return {x, y}; }
    constexpr Vector2<T> xz() const noexcept { return {x, z}; }
    constexpr Vector2<T> yx() const noexcept { return {y, x}; }
    constexpr Vector2<T> yy() const noexcept { return {y, y}; }
    constexpr Vector2<T> yz() const noexcept { return {y, z}; }
    constexpr Vector2<T> zx() const noexcept { return {z, x}; }
    constexpr Vector2<T> zy() const noexcept { return {z, y}; }
    constexpr Vector2<T> zz() const noexcept { return {z, z}; }

    // Vector3 -> Vector3
    constexpr Vector3 xxx() const noexcept { return {x, x, x}; }
    constexpr Vector3 xxy() const noexcept { return {x, x, y}; }
    constexpr Vector3 xxz() const noexcept { return {x, x, z}; }
    constexpr Vector3 xyx() const noexcept { return {x, y, x}; }
    constexpr Vector3 xyy() const noexcept { return {x, y, y}; }
    constexpr Vector3 xyz() const noexcept { return {x, y, z}; }
    constexpr Vector3 xzx() const noexcept { return {x, z, x}; }
    constexpr Vector3 xzy() const noexcept { return {x, z, y}; }
    constexpr Vector3 xzz() const noexcept { return {x, z, z}; }
    constexpr Vector3 yxx() const noexcept { return {y, x, x}; }
    constexpr Vector3 yxy() const noexcept { return {y, x, y}; }
    constexpr Vector3 yxz() const noexcept { return {y, x, z}; }
    constexpr Vector3 yyx() const noexcept { return {y, y, x}; }
    constexpr Vector3 yyy() const noexcept { return {y, y, y}; }
    constexpr Vector3 yyz() const noexcept { return {y, y, z}; }
    constexpr Vector3 yzx() const noexcept { return {y, z, x}; }
    constexpr Vector3 yzy() const noexcept { return {y, z, y}; }
    constexpr Vector3 yzz() const noexcept { return {y, z, z}; }
    constexpr Vector3 zxx() const noexcept { return {z, x, x}; }
    constexpr Vector3 zxy() const noexcept { return {z, x, y}; }
    constexpr Vector3 zxz() const noexcept { return {z, x, z}; }
    constexpr Vector3 zyx() const noexcept { return {z, y, x}; }
    constexpr Vector3 zyy() const noexcept { return {z, y, y}; }
    constexpr Vector3 zyz() const noexcept { return {z, y, z}; }
    constexpr Vector3 zzx() const noexcept { return {z, z, x}; }
    constexpr Vector3 zzy() const noexcept { return {z, z, y}; }
    constexpr Vector3 zzz() const noexcept { return {z, z, z}; }
    // clang-format on
};

template <ArithType T>
class Vector4 : public Tuple<Vector4, T, 4>
{
public:
    using Base = Tuple<Vector4, T, 4>;

    // ========== 数据成员 ==========
    T x, y, z, w;

    // clang-format off
    // ========== 构造函数 ==========
    constexpr Vector4() noexcept : x{}, y{}, z{}, w{} {}

    constexpr Vector4(T x_, T y_, T z_, T w_) noexcept : x(x_), y(y_), z(z_), w(w_)
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // 从 Vector2/3 扩展
    template <ArithType A, ArithType B, ArithType C>
    constexpr Vector4(const Vector2<A>& xy_, B z_, C w_) noexcept
        : x(To<T>(xy_.x)), y(To<T>(xy_.y)), z(To<T>(z_)), w(To<T>(w_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B, ArithType C>
    constexpr Vector4(A x_, const Vector2<B>& yz_, C w_) noexcept
        : x(To<T>(x_)), y(To<T>(yz_.x)), z(To<T>(yz_.y)), w(To<T>(w_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B, ArithType C>
    constexpr Vector4(A x_, B y_, const Vector2<C>& zw_) noexcept
        : x(To<T>(x_)), y(To<T>(y_)), z(To<T>(zw_.x)), w(To<T>(zw_.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr Vector4(const Vector3<A>& xyz_, B w_) noexcept
        : x(To<T>(xyz_.x)), y(To<T>(xyz_.y)), z(To<T>(xyz_.z)), w(To<T>(w_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr Vector4(A x_, const Vector3<B>& yzw_) noexcept
        : x(To<T>(x_)), y(To<T>(yzw_.x)), z(To<T>(yzw_.y)), w(To<T>(yzw_.z))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr Vector4(const Vector2<A>& xy_, const Vector2<B>& zw_) noexcept
        : x(To<T>(xy_.x)), y(To<T>(xy_.y)), z(To<T>(zw_.x)), w(To<T>(zw_.y))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // 类型转换
    template <ArithType U>
    constexpr explicit Vector4(const Vector2<U>& v) noexcept
        : x(To<T>(v.x)), y(To<T>(v.y)), z(T{}), w(T{})
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector4(const Vector3<U>& v) noexcept
        : x(To<T>(v.x)), y(To<T>(v.y)), z(To<T>(v.z)), w(T{})
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType U>
    constexpr explicit Vector4(const Vector4<U>& v) noexcept
        : x(To<T>(v.x)), y(To<T>(v.y)), z(To<T>(v.z)), w(To<T>(v.w))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr explicit Vector4(const Point3<A>& p, B w_) noexcept
        : x(To<T>(p.x)), y(To<T>(p.y)), z(To<T>(p.z)), w(To<T>(w_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    template <ArithType A, ArithType B>
    constexpr explicit Vector4(const Normal3<A>& n, B w_) noexcept
        : x(To<T>(n.x)), y(To<T>(n.y)), z(To<T>(n.z)), w(To<T>(w_))
    {
        BEE_DCHECK(!Base::hasNaN());
    }

    // ========== 赋值 ==========
    template <ArithType U>
    constexpr Vector4& operator=(const Vector4<U>& v) noexcept
    {
        BEE_DCHECK(!v.hasNaN());
        x = To<T>(v.x);
        y = To<T>(v.y);
        z = To<T>(v.z);
        w = To<T>(v.w);
        return *this;
    }

    // ========== 访问接口 ==========
    constexpr T& operator[](int i) noexcept
    {
        BEE_DCHECK(i >= 0 && i < 4);
        return (&x)[i];
    }

    constexpr T operator[](int i) const noexcept
    {
        BEE_DCHECK(i >= 0 && i < 4);
        return (&x)[i];
    }

    constexpr       T* data()       noexcept { return &x; }
    constexpr const T* data() const noexcept { return &x; }

    // ========== 工具方法 ==========
    constexpr bool isZero(MapFloatType<T> epsilon = To<MapFloatType<T>>((kEpsilonF + kEpsilonD) * 0.5)) const noexcept
    {
        using R = MapFloatType<T>;
        return std::abs(To<R>(x)) < epsilon && std::abs(To<R>(y)) < epsilon && std::abs(To<R>(z)) < epsilon && std::abs(To<R>(w)) < epsilon;
    }

    constexpr bool hasValidLength() const noexcept
    {
        using R = MapFloatType<T>;
        const R lenSq = To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y) + To<R>(z) * To<R>(z) + To<R>(w) * To<R>(w);
        return !IsNaN(lenSq) && !IsInf(lenSq);
    }

    // ========== 导入 Tuple 运算符 ==========
    using Base::operator+;
    using Base::operator-;
    using Base::operator*;
    using Base::operator/;
    using Base::operator+=;
    using Base::operator-=;
    using Base::operator*=;
    using Base::operator/=;

    // ========== 静态工厂 ==========
    constexpr static Vector4 Zero() noexcept { return {T{}, T{}, T{}, T{}}; }
    constexpr static Vector4 One() noexcept { return {To<T>(1), To<T>(1), To<T>(1), To<T>(1)}; }
    constexpr static Vector4 UnitX() noexcept { return {To<T>(1), T{}, T{}, T{}}; }
    constexpr static Vector4 UnitY() noexcept { return {T{}, To<T>(1), T{}, T{}}; }
    constexpr static Vector4 UnitZ() noexcept { return {T{}, T{}, To<T>(1), T{}}; }
    constexpr static Vector4 UnitW() noexcept { return {T{}, T{}, T{}, To<T>(1)}; }

    // ========== Swizzle 方法  ==========
    // Vector4 -> Vector2
    constexpr Vector2<T> xx() const noexcept { return {x, x}; }
    constexpr Vector2<T> xy() const noexcept { return {x, y}; }
    constexpr Vector2<T> xz() const noexcept { return {x, z}; }
    constexpr Vector2<T> xw() const noexcept { return {x, w}; }
    constexpr Vector2<T> yx() const noexcept { return {y, x}; }
    constexpr Vector2<T> yy() const noexcept { return {y, y}; }
    constexpr Vector2<T> yz() const noexcept { return {y, z}; }
    constexpr Vector2<T> yw() const noexcept { return {y, w}; }
    constexpr Vector2<T> zx() const noexcept { return {z, x}; }
    constexpr Vector2<T> zy() const noexcept { return {z, y}; }
    constexpr Vector2<T> zz() const noexcept { return {z, z}; }
    constexpr Vector2<T> zw() const noexcept { return {z, w}; }
    constexpr Vector2<T> wx() const noexcept { return {w, x}; }
    constexpr Vector2<T> wy() const noexcept { return {w, y}; }
    constexpr Vector2<T> wz() const noexcept { return {w, z}; }
    constexpr Vector2<T> ww() const noexcept { return {w, w}; }

    // Vector4 -> Vector3
    constexpr Vector3<T> xyz() const noexcept { return {x, y, z}; }
    constexpr Vector3<T> xzy() const noexcept { return {x, z, y}; }
    constexpr Vector3<T> xyw() const noexcept { return {x, y, w}; }
    constexpr Vector3<T> xwy() const noexcept { return {x, w, y}; }
    constexpr Vector3<T> xzw() const noexcept { return {x, z, w}; }
    constexpr Vector3<T> xwz() const noexcept { return {x, w, z}; }
    constexpr Vector3<T> yxz() const noexcept { return {y, x, z}; }
    constexpr Vector3<T> yzx() const noexcept { return {y, z, x}; }
    constexpr Vector3<T> yxw() const noexcept { return {y, x, w}; }
    constexpr Vector3<T> ywx() const noexcept { return {y, w, x}; }
    constexpr Vector3<T> yzw() const noexcept { return {y, z, w}; }
    constexpr Vector3<T> ywz() const noexcept { return {y, w, z}; }
    constexpr Vector3<T> zxy() const noexcept { return {z, x, y}; }
    constexpr Vector3<T> zyx() const noexcept { return {z, y, x}; }
    constexpr Vector3<T> zxw() const noexcept { return {z, x, w}; }
    constexpr Vector3<T> zwx() const noexcept { return {z, w, x}; }
    constexpr Vector3<T> zyw() const noexcept { return {z, y, w}; }
    constexpr Vector3<T> zwy() const noexcept { return {z, w, y}; }
    constexpr Vector3<T> wxy() const noexcept { return {w, x, y}; }
    constexpr Vector3<T> wyx() const noexcept { return {w, y, x}; }
    constexpr Vector3<T> wxz() const noexcept { return {w, x, z}; }
    constexpr Vector3<T> wzx() const noexcept { return {w, z, x}; }
    constexpr Vector3<T> wyz() const noexcept { return {w, y, z}; }
    constexpr Vector3<T> wzy() const noexcept { return {w, z, y}; }

    // Vector4 -> Vector4
    constexpr Vector4 xxxx() const noexcept { return {x, x, x, x}; }
    constexpr Vector4 xxxy() const noexcept { return {x, x, x, y}; }
    constexpr Vector4 xxyx() const noexcept { return {x, x, y, x}; }
    constexpr Vector4 xxyy() const noexcept { return {x, x, y, y}; }
    constexpr Vector4 xyxx() const noexcept { return {x, y, x, x}; }
    constexpr Vector4 xyxy() const noexcept { return {x, y, x, y}; }
    constexpr Vector4 xyyx() const noexcept { return {x, y, y, x}; }
    constexpr Vector4 xyyy() const noexcept { return {x, y, y, y}; }
    constexpr Vector4 yxxx() const noexcept { return {y, x, x, x}; }
    constexpr Vector4 yxxy() const noexcept { return {y, x, x, y}; }
    constexpr Vector4 yxyx() const noexcept { return {y, x, y, x}; }
    constexpr Vector4 yxyy() const noexcept { return {y, x, y, y}; }
    constexpr Vector4 yyxx() const noexcept { return {y, y, x, x}; }
    constexpr Vector4 yyxy() const noexcept { return {y, y, x, y}; }
    constexpr Vector4 yyyx() const noexcept { return {y, y, y, x}; }
    constexpr Vector4 yyyy() const noexcept { return {y, y, y, y}; }
    constexpr Vector4 zzzz() const noexcept { return {z, z, z, z}; }
    constexpr Vector4 wwww() const noexcept { return {w, w, w, w}; }
    // clang-format on
};

// ==================== PointType ====================

template <ArithType T>
class Point2 : public Tuple<Point2, T, 2>
{
public:
    using Base = Tuple<Point2, T, 2>;

    // ========== 数据成员 ==========
    T x, y;

    // clang-format off
    // ========== 构造函数 ==========
    constexpr Point2() noexcept : x{}, y{} {}

    constexpr Point2(T x_, T y_) noexcept : x(x_), y(y_) {}

    constexpr explicit Point2(const Vector2<T>& v) noexcept : x(v.x), y(v.y) {}

    template <ArithType U>
    constexpr explicit Point2(const Vector2<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y)) {}

    // ========== 访问接口 ==========
    constexpr T& operator[](int i)       noexcept { return (&x)[i]; }
    constexpr T  operator[](int i) const noexcept { return (&x)[i]; }

    // ========== 选择性导入/删除运算符 ==========
    using Base::operator==;
    using Base::operator!=;

    template <ArithType U> constexpr auto operator+(Point2<U> p) const = delete;
    template <ArithType U> constexpr auto operator*(U s) const = delete;
    template <ArithType U> constexpr auto operator/(U d) const = delete;

    template <ArithType U> constexpr Point2& operator+=(Point2<U> p) = delete;
    template <ArithType U> constexpr Point2& operator-=(Point2<U> p) = delete;
    template <ArithType U> constexpr Point2& operator*=(U s) = delete;
    template <ArithType U> constexpr Point2& operator/=(U d) = delete;

    // ========== 特殊语义运算符 ==========

    // Point + Vector = Point
    template <ArithType U>
    constexpr auto operator+(const Vector2<U>& v) const -> Point2<decltype(T{} + U{})>
    {
        using R = decltype(T{} + U{});
        return {To<R>(x) + To<R>(v.x), To<R>(y) + To<R>(v.y)};
    }

    // Point - Vector = Point
    template <ArithType U>
    constexpr auto operator-(const Vector2<U>& v) const -> Point2<decltype(T{} - U{})>
    {
        using R = decltype(T{} - U{});
        return {To<R>(x) - To<R>(v.x), To<R>(y) - To<R>(v.y)};
    }

    // Point - Point = Vector
    template <ArithType U>
    constexpr auto operator-(const Point2<U>& p) const -> Vector2<decltype(T{} - U{})>
    {
        using R = decltype(T{} - U{});
        return {To<R>(x) - To<R>(p.x), To<R>(y) - To<R>(p.y)};
    }

    constexpr static Point2 Origin() noexcept { return {T{}, T{}}; }
    // clang-format on
};

template <ArithType T>
class Point3 : public Tuple<Point3, T, 3>
{
public:
    using Base = Tuple<Point3, T, 3>;

    // ========== 数据成员 ==========
    T x, y, z;

    // clang-format off
    // ========== 构造函数 ==========
    constexpr Point3() noexcept : x{}, y{}, z{} {}

    constexpr Point3(T x_, T y_, T z_) noexcept : x(x_), y(y_), z(z_) {}

    constexpr explicit Point3(const Vector3<T>& v) noexcept : x(v.x), y(v.y), z(v.z) {}

    template <ArithType U>
    constexpr explicit Point3(const Vector3<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y)), z(To<T>(v.z)) {}

    // ========== 访问接口 ==========
    constexpr T& operator[](int i) noexcept { return (&x)[i]; }
    constexpr T operator[](int i) const noexcept { return (&x)[i]; }

    // ========== 选择性导入/删除运算符 ==========
    using Base::operator==;
    using Base::operator!=;

    template <ArithType U> constexpr auto operator+(Point3<U> p) const = delete;
    template <ArithType U> constexpr auto operator*(U s) const = delete;
    template <ArithType U> constexpr auto operator/(U d) const = delete;

    template <ArithType U> constexpr Point3& operator+=(Point3<U> p) = delete;
    template <ArithType U> constexpr Point3& operator-=(Point3<U> p) = delete;
    template <ArithType U> constexpr Point3& operator*=(U s) = delete;
    template <ArithType U> constexpr Point3& operator/=(U d) = delete;

    // ========== 特殊语义运算符 ==========

    // Point + Vector = Point
    template <ArithType U>
    constexpr auto operator+(const Vector3<U>& v) const -> Point3<decltype(T{} + U{})>
    {
        using R = decltype(T{} + U{});
        return {To<R>(x) + To<R>(v.x), To<R>(y) + To<R>(v.y), To<R>(z) + To<R>(v.z)};
    }

    // Point - Vector = Point
    template <ArithType U>
    constexpr auto operator-(const Vector3<U>& v) const -> Point3<decltype(T{} - U{})>
    {
        using R = decltype(T{} - U{});
        return {To<R>(x) - To<R>(v.x), To<R>(y) - To<R>(v.y), To<R>(z) - To<R>(v.z)};
    }

    // Point - Point = Vector
    template <ArithType U>
    constexpr auto operator-(const Point3<U>& p) const -> Vector3<decltype(T{} - U{})>
    {
        using R = decltype(T{} - U{});
        return {To<R>(x) - To<R>(p.x), To<R>(y) - To<R>(p.y), To<R>(z) - To<R>(p.z)};
    }

    constexpr static Point3 Origin() noexcept { return {T{}, T{}, T{}}; }
    // clang-format on
};

// ==================== NormalType ====================

template <ArithType T>
class Normal2 : public Tuple<Normal2, T, 2>
{
public:
    using Base = Tuple<Normal2, T, 2>;

    // ========== 数据成员 ==========
    T x, y;

    // clang-format off
    // ========== 构造函数 ==========
    constexpr Normal2() noexcept : x{}, y{} {}

    constexpr Normal2(T x_, T y_) noexcept : x(x_), y(y_) {}

    constexpr explicit Normal2(const Vector2<T>& v) noexcept : x(v.x), y(v.y) {}

    template <ArithType U>
    constexpr explicit Normal2(const Vector2<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y)) {}

    // ========== 访问接口 ==========
    constexpr T& operator[](int i)       noexcept { return (&x)[i]; }
    constexpr T  operator[](int i) const noexcept { return (&x)[i]; }

    // ========== 选择性导入 ==========
    using Base::operator==;
    using Base::operator!=;

    // ========== 特定方法 ==========
    constexpr auto normalize() const -> Normal2<MapFloatType<T>>;
    constexpr bool isNormalized() const;
    constexpr bool isNormalized(MapFloatType<T> epsilon) const;

    constexpr static Normal2 UnitX() noexcept { return {To<T>(1), T{}}; }
    constexpr static Normal2 UnitY() noexcept { return {T{}, To<T>(1)}; }
    // clang-format on
};

template <ArithType T>
class Normal3 : public Tuple<Normal3, T, 3>
{
public:
    using Base = Tuple<Normal3, T, 3>;

    // ========== 数据成员 ==========
    T x, y, z;

    // clang-format off
    // ========== 构造函数 ==========
    constexpr Normal3() noexcept : x{}, y{}, z{} {}

    constexpr Normal3(T x_, T y_, T z_) noexcept : x(x_), y(y_), z(z_) {}

    constexpr explicit Normal3(const Vector3<T>& v) noexcept : x(v.x), y(v.y), z(v.z) {}

    template <ArithType U>
    constexpr explicit Normal3(const Vector3<U>& v) noexcept : x(To<T>(v.x)), y(To<T>(v.y)), z(To<T>(v.z)) {}

    // ========== 访问接口 ==========
    constexpr T& operator[](int i)       noexcept { return (&x)[i]; }
    constexpr T  operator[](int i) const noexcept { return (&x)[i]; }

    // ========== 选择性导入 ==========
    using Base::operator==;
    using Base::operator!=;

    // ========== 特定方法 ==========
    constexpr auto normalize() const -> Normal3<MapFloatType<T>>;
    constexpr bool isNormalized() const;
    constexpr bool isNormalized(MapFloatType<T> epsilon) const;

    constexpr void set(T x_, T y_, T z_) noexcept { x = x_; y = y_; z = z_; }

    constexpr static Normal3 UnitX() noexcept { return {To<T>(1), T{}, T{}}; }
    constexpr static Normal3 UnitY() noexcept { return {T{}, To<T>(1), T{}}; }
    constexpr static Normal3 UnitZ() noexcept { return {T{}, T{}, To<T>(1)}; }
    // clang-format on
};

// ========================================
// Normal 方法实现
// ========================================

template <ArithType T>
constexpr auto Normal2<T>::normalize() const -> Normal2<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = std::sqrt(To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y));

    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / len;
    return {To<R>(x) * inv, To<R>(y) * inv};
}

template <ArithType T>
constexpr bool Normal2<T>::isNormalized() const
{
    using R = MapFloatType<T>;
    R lenSq = To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y);
    // 使用项目定义的默认容差常量
    R tol = std::is_same_v<R, f32> ? R(kDefaultEpsilonF) : R(kDefaultEpsilonD);
    return std::abs(lenSq - R(1)) < tol * R(2);
}

template <ArithType T>
constexpr bool Normal2<T>::isNormalized(MapFloatType<T> epsilon) const
{
    using R = MapFloatType<T>;
    R lenSq = To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y);
    return std::abs(lenSq - R(1)) < epsilon * R(2);
}

template <ArithType T>
constexpr auto Normal3<T>::normalize() const -> Normal3<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = std::sqrt(To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y) + To<R>(z) * To<R>(z));

    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / len;
    return {To<R>(x) * inv, To<R>(y) * inv, To<R>(z) * inv};
}

template <ArithType T>
constexpr bool Normal3<T>::isNormalized() const
{
    using R = MapFloatType<T>;
    R lenSq = To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y) + To<R>(z) * To<R>(z);
    // 使用项目定义的默认容差常量
    R tol = std::is_same_v<R, f32> ? R(kDefaultEpsilonF) : R(kDefaultEpsilonD);
    return std::abs(lenSq - R(1)) < tol * R(2);
}

template <ArithType T>
constexpr bool Normal3<T>::isNormalized(MapFloatType<T> epsilon) const
{
    using R = MapFloatType<T>;
    R lenSq = To<R>(x) * To<R>(x) + To<R>(y) * To<R>(y) + To<R>(z) * To<R>(z);
    return std::abs(lenSq - R(1)) < epsilon * R(2);
}

} // namespace bee
