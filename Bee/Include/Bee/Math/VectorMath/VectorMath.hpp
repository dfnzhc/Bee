/**
 * @File VectorMath.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "VectorType.hpp"
#include "Bee/Math/Constants.hpp"
#include "Bee/Math/Common.hpp"

// TODO: 补偿浮点数精度，Dot 等方法

namespace bee
{
template <ArithType T>
constexpr auto LengthSqr(const Vector2<T>& v)
{
    using R = MapFloatType<T>;
    return To<R>(v.x) * To<R>(v.x) + To<R>(v.y) * To<R>(v.y);
}

template <ArithType T>
constexpr auto LengthSqr(const Vector3<T>& v)
{
    using R = MapFloatType<T>;
    return To<R>(v.x) * To<R>(v.x) + To<R>(v.y) * To<R>(v.y) + To<R>(v.z) * To<R>(v.z);
}

template <ArithType T>
constexpr auto LengthSqr(const Vector4<T>& v)
{
    using R = MapFloatType<T>;
    return To<R>(v.x) * To<R>(v.x) + To<R>(v.y) * To<R>(v.y) + To<R>(v.z) * To<R>(v.z) + To<R>(v.w) * To<R>(v.w);
}

template <ArithType T>
constexpr auto Length(const Vector2<T>& v)
{
    return bee::Sqrt(LengthSqr(v));
}

template <ArithType T>
constexpr auto Length(const Vector3<T>& v)
{
    return bee::Sqrt(LengthSqr(v));
}

template <ArithType T>
constexpr auto Length(const Vector4<T>& v)
{
    return bee::Sqrt(LengthSqr(v));
}

template <ArithType T>
constexpr auto Length(const Normal2<T>& n)
{
    return bee::Sqrt(To<MapFloatType<T>>(n.x) * To<MapFloatType<T>>(n.x) + To<MapFloatType<T>>(n.y) * To<MapFloatType<T>>(n.y));
}

template <ArithType T>
constexpr auto Length(const Normal3<T>& n)
{
    return bee::Sqrt(
            To<MapFloatType<T>>(n.x) * To<MapFloatType<T>>(n.x) +
            To<MapFloatType<T>>(n.y) * To<MapFloatType<T>>(n.y) +
            To<MapFloatType<T>>(n.z) * To<MapFloatType<T>>(n.z));
}

template <ArithType T, ArithType U>
constexpr auto Distance(const Point2<T>& p1, const Point2<U>& p2)
{
    return Length(p1 - p2);
}

template <ArithType T, ArithType U>
constexpr auto DistanceSquared(const Point2<T>& p1, const Point2<U>& p2)
{
    return LengthSqr(p1 - p2);
}

template <ArithType T, ArithType U>
constexpr auto Distance(const Point3<T>& p1, const Point3<U>& p2)
{
    return Length(p1 - p2);
}

template <ArithType T, ArithType U>
constexpr auto DistanceSquared(const Point3<T>& p1, const Point3<U>& p2)
{
    return LengthSqr(p1 - p2);
}

// ========== Dot Product - 2D ==========

template <ArithType T, ArithType U>
constexpr auto Dot(const Vector2<T>& a, const Vector2<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y);
}

template <ArithType T, ArithType U>
constexpr auto Dot(const Vector2<T>& a, const Normal2<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y);
}

template <ArithType T, ArithType U>
constexpr auto Dot(const Normal2<T>& a, const Vector2<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y);
}

template <ArithType T, ArithType U>
constexpr auto Dot(const Normal2<T>& a, const Normal2<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y);
}

// ========== Dot Product - 3D ==========

template <ArithType T, ArithType U>
constexpr auto Dot(const Vector3<T>& a, const Vector3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y) + To<R>(a.z) * To<R>(b.z);
}

template <ArithType T, ArithType U>
constexpr auto Dot(const Vector3<T>& a, const Normal3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y) + To<R>(a.z) * To<R>(b.z);
}

template <ArithType T, ArithType U>
constexpr auto Dot(const Normal3<T>& a, const Vector3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y) + To<R>(a.z) * To<R>(b.z);
}

template <ArithType T, ArithType U>
constexpr auto Dot(const Normal3<T>& a, const Normal3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y) + To<R>(a.z) * To<R>(b.z);
}

// ========== Dot Product - 4D ==========

template <ArithType T, ArithType U>
constexpr auto Dot(const Vector4<T>& a, const Vector4<U>& b)
{
    using R = CommonFloatType<T, U>;
    return To<R>(a.x) * To<R>(b.x) + To<R>(a.y) * To<R>(b.y) + To<R>(a.z) * To<R>(b.z) + To<R>(a.w) * To<R>(b.w);
}

// ========== AbsDot - 2D ==========

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Vector2<T>& a, const Vector2<U>& b)
{
    return std::abs(Dot(a, b));
}

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Vector2<T>& a, const Normal2<U>& b)
{
    return std::abs(Dot(a, b));
}

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Normal2<T>& a, const Vector2<U>& b)
{
    return std::abs(Dot(a, b));
}

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Normal2<T>& a, const Normal2<U>& b)
{
    return std::abs(Dot(a, b));
}

// ========== AbsDot - 3D ==========

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Vector3<T>& a, const Vector3<U>& b)
{
    return std::abs(Dot(a, b));
}

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Vector3<T>& a, const Normal3<U>& b)
{
    return std::abs(Dot(a, b));
}

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Normal3<T>& a, const Vector3<U>& b)
{
    return std::abs(Dot(a, b));
}

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Normal3<T>& a, const Normal3<U>& b)
{
    return std::abs(Dot(a, b));
}

// ========== AbsDot - 4D ==========

template <ArithType T, ArithType U>
constexpr auto AbsDot(const Vector4<T>& a, const Vector4<U>& b)
{
    return std::abs(Dot(a, b));
}

// ========== Cross Product ==========

template <ArithType T, ArithType U>
constexpr auto Cross(const Vector3<T>& a, const Vector3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y) * To<R>(b.z) - To<R>(a.z) * To<R>(b.y),
                      To<R>(a.z) * To<R>(b.x) - To<R>(a.x) * To<R>(b.z),
                      To<R>(a.x) * To<R>(b.y) - To<R>(a.y) * To<R>(b.x));
}

template <ArithType T, ArithType U>
constexpr auto Cross(const Vector3<T>& a, const Normal3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y) * To<R>(b.z) - To<R>(a.z) * To<R>(b.y),
                      To<R>(a.z) * To<R>(b.x) - To<R>(a.x) * To<R>(b.z),
                      To<R>(a.x) * To<R>(b.y) - To<R>(a.y) * To<R>(b.x));
}

template <ArithType T, ArithType U>
constexpr auto Cross(const Normal3<T>& a, const Vector3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y) * To<R>(b.z) - To<R>(a.z) * To<R>(b.y),
                      To<R>(a.z) * To<R>(b.x) - To<R>(a.x) * To<R>(b.z),
                      To<R>(a.x) * To<R>(b.y) - To<R>(a.y) * To<R>(b.x));
}

template <ArithType T, ArithType U>
constexpr auto Cross(const Normal3<T>& a, const Normal3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y) * To<R>(b.z) - To<R>(a.z) * To<R>(b.y),
                      To<R>(a.z) * To<R>(b.x) - To<R>(a.x) * To<R>(b.z),
                      To<R>(a.x) * To<R>(b.y) - To<R>(a.y) * To<R>(b.x));
}

// ========== FaceForward - 2D ==========

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Normal2<T>& n, const Vector2<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal2<T>{-n.x, -n.y} : n;
}

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Normal2<T>& n, const Normal2<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal2<T>{-n.x, -n.y} : n;
}

// ========== FaceForward - 3D ==========

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Normal3<T>& n, const Vector3<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal3<T>{-n.x, -n.y, -n.z} : n;
}

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Normal3<T>& n, const Normal3<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal3<T>{-n.x, -n.y, -n.z} : n;
}

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Vector3<T>& v, const Vector3<U>& v2)
{
    return (Dot(v, v2) < To<CommonFloatType<T, U>>(0)) ? Vector3<T>{-v.x, -v.y, -v.z} : v;
}

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Vector3<T>& v, const Normal3<U>& n2)
{
    return (Dot(v, n2) < To<CommonFloatType<T, U>>(0)) ? Vector3<T>{-v.x, -v.y, -v.z} : v;
}

// ========== Normalize ==========

template <ArithType T>
constexpr auto Normalize(const Vector2<T>& v) -> Vector2<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv};
}

template <ArithType T>
constexpr auto Normalize(const Vector3<T>& v) -> Vector3<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv, To<R>(v.z) * inv};
}

template <ArithType T>
constexpr auto Normalize(const Vector4<T>& v) -> Vector4<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv, To<R>(v.z) * inv, To<R>(v.w) * inv};
}

template <ArithType T>
constexpr auto Normalize(const Normal2<T>& v) -> Normal2<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv};
}

template <ArithType T>
constexpr auto Normalize(const Normal3<T>& v) -> Normal3<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv, To<R>(v.z) * inv};
}

// ========== SafeNormalize ==========
// 数值稳定的归一化函数，处理接近零的向量

template <ArithType T>
constexpr auto SafeNormalize(const Vector3<T>& v, MapFloatType<T> epsilon = MapFloatType<T>(1e-8)) -> Vector3<MapFloatType<T>>
{
    using R = MapFloatType<T>;
    // 先检查长度平方，避免不必要的 sqrt
    auto lenSq = LengthSqr(v);
    if (!IsSafeLengthSqr(To<R>(lenSq), To<R>(epsilon) * To<R>(epsilon)))
    {
        return Vector3<R>::UnitY();
    }
    auto len = Sqrt(lenSq);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv, To<R>(v.z) * inv};
}

template <ArithType T>
constexpr auto SafeNormalize(const Vector2<T>& v, MapFloatType<T> epsilon = MapFloatType<T>(1e-8)) -> Vector2<MapFloatType<T>>
{
    using R    = MapFloatType<T>;
    auto lenSq = LengthSqr(v);
    if (!IsSafeLengthSqr(To<R>(lenSq), To<R>(epsilon) * To<R>(epsilon)))
    {
        return Vector2<R>::UnitY();
    }
    auto len = Sqrt(lenSq);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv};
}

template <ArithType T>
constexpr auto SafeNormalize(const Vector4<T>& v, MapFloatType<T> epsilon = MapFloatType<T>(1e-8)) -> Vector4<MapFloatType<T>>
{
    using R    = MapFloatType<T>;
    auto lenSq = LengthSqr(v);
    if (!IsSafeLengthSqr(To<R>(lenSq), To<R>(epsilon) * To<R>(epsilon)))
    {
        return Vector4<R>::UnitY();
    }
    auto len = Sqrt(lenSq);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x) * inv, To<R>(v.y) * inv, To<R>(v.z) * inv, To<R>(v.w) * inv};
}

// ========== AngleBetween ==========

template <ArithType T, ArithType U>
constexpr auto AngleBetween(const Vector3<T>& a, const Vector3<U>& b)
{
    using R    = CommonFloatType<T, U>;
    auto denom = To<R>(Length(a)) * To<R>(Length(b));
    BEE_DCHECK(denom > 0);
    auto c = To<R>(Dot(a, b)) / denom;
    return bee::Acos(std::clamp(c, To<R>(-1), To<R>(1)));
}

template <ArithType T, ArithType U>
constexpr auto AngleBetween(const Normal3<T>& a, const Normal3<U>& b)
{
    using R    = CommonFloatType<T, U>;
    auto denom = To<R>(Length(a)) * To<R>(Length(b));
    BEE_DCHECK(denom > 0);
    auto c = To<R>(Dot(a, b)) / denom;
    return bee::Acos(std::clamp(c, To<R>(-1), To<R>(1)));
}

// ========== GramSchmidt ==========

template <ArithType T, ArithType U>
constexpr auto GramSchmidt(const Vector3<T>& v, const Vector3<U>& w)
{
    using R   = CommonFloatType<T, U>;
    auto proj = To<R>(Dot(v, w));
    return Vector3<R>(To<R>(v.x) - proj * To<R>(w.x),
                      To<R>(v.y) - proj * To<R>(w.y),
                      To<R>(v.z) - proj * To<R>(w.z));
}

// ========== CoordinateSystem ==========

template <ArithType T>
constexpr void CoordinateSystem(Vector3<T> v1, Vector3<T>* v2, Vector3<T>* v3)
{
    using R   = MapFloatType<T>;
    auto sign = std::copysign(To<R>(1), To<R>(v1.z));
    auto a    = -To<R>(1) / (sign + To<R>(v1.z));
    auto b    = To<R>(v1.x) * To<R>(v1.y) * a;
    *v2       = Vector3<T>(To<T>(1 + sign * To<R>(v1.x) * To<R>(v1.x) * a), To<T>(sign * b), To<T>(-sign * To<R>(v1.x)));
    *v3       = Vector3<T>(To<T>(b), To<T>(sign + To<R>(v1.y) * To<R>(v1.y) * a), To<T>(-To<R>(v1.y)));
}

template <ArithType T>
constexpr void CoordinateSystem(Normal3<T> v1, Vector3<T>* v2, Vector3<T>* v3)
{
    using R   = MapFloatType<T>;
    auto sign = std::copysign(To<R>(1), To<R>(v1.z));
    auto a    = -To<R>(1) / (sign + To<R>(v1.z));
    auto b    = To<R>(v1.x) * To<R>(v1.y) * a;
    *v2       = Vector3<T>(To<T>(1 + sign * To<R>(v1.x) * To<R>(v1.x) * a), To<T>(sign * b), To<T>(-sign * To<R>(v1.x)));
    *v3       = Vector3<T>(To<T>(b), To<T>(sign + To<R>(v1.y) * To<R>(v1.y) * a), To<T>(-To<R>(v1.y)));
}

// ========== Lerp for Points ==========

template <FloatType T, FloatType U>
constexpr auto Lerp(const Point2<T>& p0, const Point2<T>& p1, U x)
{
    auto tx = To<T>(x);
    Point2<T> p;
    p.x = Lerp(p0.x, p1.x, tx);
    p.y = Lerp(p0.y, p1.y, tx);
    return p;
}

template <FloatType T, FloatType U>
constexpr auto Lerp(const Point3<T>& p0, const Point3<T>& p1, U x)
{
    auto tx = To<T>(x);
    Point3<T> p;
    p.x = Lerp(p0.x, p1.x, tx);
    p.y = Lerp(p0.y, p1.y, tx);
    p.z = Lerp(p0.z, p1.z, tx);
    return p;
}

// ========== Spherical Coordinates ==========

template <FloatType F>
constexpr F SphericalTheta(const Vector3<F>& v)
{
    BEE_DCHECK(v.z >= To<F>(-1.0001) && v.z <= To<F>(1.0001));
    return bee::Acos(std::clamp(v.z, To<F>(-1), To<F>(1)));
}

template <FloatType F>
constexpr F SphericalPhi(const Vector3<F>& v)
{
    auto p = bee::Atan2(v.y, v.x);
    return (p < 0) ? (p + TwoPi<F>()) : p;
}

template <FloatType F>
constexpr F CosTheta(const Vector3<F>& w)
{
    return w.z;
}

template <FloatType F>
constexpr F Cos2Theta(const Vector3<F>& w)
{
    return w.z * w.z;
}

template <FloatType F>
constexpr F AbsCosTheta(const Vector3<F>& w)
{
    return std::abs(w.z);
}

template <FloatType F>
constexpr F Sin2Theta(const Vector3<F>& w)
{
    return std::max(To<F>(0), To<F>(1) - Cos2Theta(w));
}

template <FloatType F>
constexpr F SinTheta(const Vector3<F>& w)
{
    return bee::Sqrt(Sin2Theta(w));
}

template <FloatType F>
constexpr F TanTheta(const Vector3<F>& w)
{
    return SinTheta(w) / CosTheta(w);
}

template <FloatType F>
constexpr F Tan2Theta(const Vector3<F>& w)
{
    return Sin2Theta(w) / Cos2Theta(w);
}

template <FloatType F>
constexpr F CosPhi(const Vector3<F>& w)
{
    auto sinTheta = SinTheta(w);
    return NearZero(sinTheta) ? To<F>(1) : std::clamp(w.x / sinTheta, To<F>(-1), To<F>(1));
}

template <FloatType F>
constexpr F SinPhi(const Vector3<F>& w)
{
    auto sinTheta = SinTheta(w);
    return NearZero(sinTheta) ? To<F>(0) : std::clamp(w.y / sinTheta, To<F>(-1), To<F>(1));
}

template <ArithType T, ArithType U>
constexpr auto CosDPhi(const Vector3<T>& wa, const Vector3<U>& wb) -> CommonFloatType<T, U>
{
    using R = CommonFloatType<T, U>;

    auto waxy = To<R>(wa.x * wa.x + wa.y * wa.y);
    auto wbxy = To<R>(wb.x * wb.x + wb.y * wb.y);

    if (NearZero(waxy) || NearZero(wbxy))
        return To<R>(1);

    return std::clamp(To<R>(wa.x * wb.x + wa.y * wb.y) / bee::Sqrt(waxy * wbxy), To<R>(-1), To<R>(1));
}

// ========== SameHemisphere ==========

template <ArithType T, ArithType U>
constexpr bool SameHemisphere(const Vector3<T>& w, const Vector3<U>& wp)
{
    return w.z * wp.z > 0;
}

template <ArithType T, ArithType U>
constexpr bool SameHemisphere(const Vector3<T>& w, const Normal3<U>& wp)
{
    return w.z * wp.z > 0;
}

// ========== Orthogonal ==========

/// 返回一个与输入向量正交的向量
template <ArithType T>
constexpr Vector3<T> Orthogonal(const Vector3<T>& v)
{
    // 如果向量不与 Z 轴平行，使用 v x Z
    // 否则使用 v x X
    if (std::abs(v.x) > std::abs(v.y) || std::abs(v.z) > std::abs(v.y))
    {
        // v x Z = (v.y * 1 - v.z * 0, v.z * 0 - v.x * 1, v.x * 0 - v.y * 0) = (v.y, -v.x, 0)
        return Normalize(Vector3<T>(v.y, -v.x, T{}));
    }
    else
    {
        // v x X = (v.y * 0 - v.z * 0, v.z * 1 - v.x * 0, v.x * 0 - v.y * 1) = (0, v.z, -v.y)
        return Normalize(Vector3<T>(T{}, v.z, -v.y));
    }
}

// ========== Reflection and Refraction ==========

template <ArithType T, ArithType U>
constexpr auto Reflect(const Vector3<T>& v, const Normal3<U>& n) -> Vector3<decltype(T{} - U{})>
{
    using R = decltype(T{} - U{});
    auto nv = Vector3<R>(n.x, n.y, n.z);
    return Vector3<R>(v) - nv * Dot(v, nv) * To<R>(2);
}

template <ArithType T>
constexpr auto Refract(const Vector3<T>& v, const Normal3<T>& n, T eta) -> std::optional<Vector3<T>>
{
    using F    = MapFloatType<T>;
    auto nv    = Vector3<F>(n.x, n.y, n.z);
    auto vv    = Vector3<F>(v);
    auto cosI  = -Dot(nv, vv);
    auto sinT2 = eta * eta * (F(1) - cosI * cosI);

    // 全反射检测
    if (sinT2 > F(1))
        return std::nullopt;

    auto cosT   = Sqrt(F(1) - sinT2);
    auto result = eta * vv + (eta * cosI - cosT) * nv;
    return Normalize(result); // 折射向量需要归一化
}

// ========== Vector Projection and Rejection ==========
template <ArithType T, ArithType U>
constexpr auto Project(const Vector3<T>& v, const Vector3<U>& onto) -> Vector3<CommonFloatType<T, U>>
{
    using R         = CommonFloatType<T, U>;
    auto dotProduct = Dot(v, onto);
    auto lengthSq   = Dot(onto, onto);
    BEE_DCHECK(lengthSq > R{});
    return Vector3<R>(onto) * (To<R>(dotProduct) / To<R>(lengthSq));
}

template <ArithType T, ArithType U>
constexpr auto Reject(const Vector3<T>& v, const Vector3<U>& onto) -> Vector3<CommonFloatType<T, U>>
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(v) - Project(v, onto);
}

// ========== Triple Product ==========

// 计算三向量的混合积 (a · (b × c))
// 等价于以三个向量为边的平行六面体的有向体积
template <ArithType T, ArithType U, ArithType V>
constexpr auto TripleProduct(const Vector3<T>& a, const Vector3<U>& b, const Vector3<V>& c) -> CommonFloatType<T, U, V>
{
    return Dot(a, Cross(b, c));
}

} // namespace bee
