/**
 * @File VectorMath.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "VectorType.hpp"
#include "Bee/Math/Constants.hpp"

// TODO: 补偿浮点数精度，Dot 等方法

namespace bee
{
template <ArithType T>
constexpr auto LengthSqr(const Vector2<T>& v)
{
    using R = MapFloatType<T>;
    return To<R>(v.x()) * To<R>(v.x()) + To<R>(v.y()) * To<R>(v.y());
}

template <ArithType T>
constexpr auto LengthSqr(const Vector3<T>& v)
{
    using R = MapFloatType<T>;
    return To<R>(v.x()) * To<R>(v.x()) + To<R>(v.y()) * To<R>(v.y()) + To<R>(v.z()) * To<R>(v.z());
}

template <ArithType T>
constexpr auto LengthSqr(const Vector4<T>& v)
{
    using R = MapFloatType<T>;
    return To<R>(v.x()) * To<R>(v.x()) + To<R>(v.y()) * To<R>(v.y()) + To<R>(v.z()) * To<R>(v.z()) + To<R>(v.w()) * To<R>(v.w());
}

template <ArithType T>
constexpr auto Length(const Vector2<T>& v)
{
    return std::sqrt(LengthSqr(v));
}

template <ArithType T>
constexpr auto Length(const Vector3<T>& v)
{
    return std::sqrt(LengthSqr(v));
}

template <ArithType T>
constexpr auto Length(const Vector4<T>& v)
{
    return std::sqrt(LengthSqr(v));
}

template <ArithType T>
constexpr auto Length(const Normal2<T>& n)
{
    return std::sqrt(
            To<MapFloatType<T>>(n.x()) * To<MapFloatType<T>>(n.x()) +
            To<MapFloatType<T>>(n.y()) * To<MapFloatType<T>>(n.y()));
}

template <ArithType T>
constexpr auto Length(const Normal3<T>& n)
{
    return std::sqrt(
            To<MapFloatType<T>>(n.x()) * To<MapFloatType<T>>(n.x()) +
            To<MapFloatType<T>>(n.y()) * To<MapFloatType<T>>(n.y()) +
            To<MapFloatType<T>>(n.z()) * To<MapFloatType<T>>(n.z()));
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

template <template <typename> class C1, template <typename> class C2, ArithType T, ArithType U>
    requires (!PointType<C1<T>> && !PointType<C2<T>>)
constexpr auto Dot(const Tuple2<C1, T>& a, const Tuple2<C2, U>& b)
{
    using R = CommonFloatType<T, U>;
    R acc = To<R>(a[0]) * To<R>(b[0]);
    acc += To<R>(a[1]) * To<R>(b[1]);
    return acc;
}

template <template <typename> class C1, template <typename> class C2, ArithType T, ArithType U>
    requires (!PointType<C1<T>> && !PointType<C2<T>>)
constexpr auto Dot(const Tuple3<C1, T>& a, const Tuple3<C2, U>& b)
{
    using R = CommonFloatType<T, U>;
    R acc = To<R>(a[0]) * To<R>(b[0]);
    acc += To<R>(a[1]) * To<R>(b[1]);
    acc += To<R>(a[2]) * To<R>(b[2]);
    return acc;
}

template <template <typename> class C1, template <typename> class C2, ArithType T, ArithType U>
    requires (!PointType<C1<T>> && !PointType<C2<T>>)
constexpr auto Dot(const Tuple4<C1, T>& a, const Tuple4<C2, U>& b)
{
    using R = CommonFloatType<T, U>;
    R acc = To<R>(a[0]) * To<R>(b[0]);
    acc += To<R>(a[1]) * To<R>(b[1]);
    acc += To<R>(a[2]) * To<R>(b[2]);
    acc += To<R>(a[3]) * To<R>(b[3]);
    return acc;
}

template <template <typename> class C1, template <typename> class C2, ArithType T, ArithType U>
    requires (!PointType<C1<T>> && !PointType<C2<T>>)
constexpr auto AbsDot(const Tuple2<C1, T>& a, const Tuple2<C2, U>& b)
{
    using R = decltype(Dot(a, b));
    return std::abs(To<R>(Dot(a, b)));
}

template <template <typename> class C1, template <typename> class C2, ArithType T, ArithType U>
    requires (!PointType<C1<T>> && !PointType<C2<T>>)
constexpr auto AbsDot(const Tuple3<C1, T>& a, const Tuple3<C2, U>& b)
{
    using R = decltype(Dot(a, b));
    return std::abs(To<R>(Dot(a, b)));
}

template <template <typename> class C1, template <typename> class C2, ArithType T, ArithType U>
    requires (!PointType<C1<T>> && !PointType<C2<T>>)
constexpr auto AbsDot(const Tuple4<C1, T>& a, const Tuple4<C2, U>& b)
{
    using R = decltype(Dot(a, b));
    return std::abs(To<R>(Dot(a, b)));
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto Cross(const Vector3<T>& a, const Vector3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y()) * To<R>(b.z()) - To<R>(a.z()) * To<R>(b.y()),
                      To<R>(a.z()) * To<R>(b.x()) - To<R>(a.x()) * To<R>(b.z()),
                      To<R>(a.x()) * To<R>(b.y()) - To<R>(a.y()) * To<R>(b.x()));
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto Cross(const Vector3<T>& a, const Normal3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y()) * To<R>(b.z()) - To<R>(a.z()) * To<R>(b.y()),
                      To<R>(a.z()) * To<R>(b.x()) - To<R>(a.x()) * To<R>(b.z()),
                      To<R>(a.x()) * To<R>(b.y()) - To<R>(a.y()) * To<R>(b.x()));
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto Cross(const Normal3<T>& a, const Vector3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y()) * To<R>(b.z()) - To<R>(a.z()) * To<R>(b.y()),
                      To<R>(a.z()) * To<R>(b.x()) - To<R>(a.x()) * To<R>(b.z()),
                      To<R>(a.x()) * To<R>(b.y()) - To<R>(a.y()) * To<R>(b.x()));
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto Cross(const Normal3<T>& a, const Normal3<U>& b)
{
    using R = CommonFloatType<T, U>;
    return Vector3<R>(To<R>(a.y()) * To<R>(b.z()) - To<R>(a.z()) * To<R>(b.y()),
                      To<R>(a.z()) * To<R>(b.x()) - To<R>(a.x()) * To<R>(b.z()),
                      To<R>(a.x()) * To<R>(b.y()) - To<R>(a.y()) * To<R>(b.x()));
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto FaceForward(const Normal2<T>& n, const Vector2<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal2<T>{-n.x(), -n.y()} : n;
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto FaceForward(const Normal2<T>& n, const Normal2<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal2<T>{-n.x(), -n.y()} : n;
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto FaceForward(const Normal3<T>& n, const Vector3<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal3<T>{-n.x(), -n.y(), -n.z()} : n;
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto FaceForward(const Normal3<T>& n, const Normal3<U>& v)
{
    return (Dot(n, v) < To<CommonFloatType<T, U>>(0)) ? Normal3<T>{-n.x(), -n.y(), -n.z()} : n;
}

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Vector3<T>& v, const Vector3<U>& v2)
{
    return (Dot(v, v2) < To<CommonFloatType<T, U>>(0)) ? Vector3<T>{-v.x(), -v.y(), -v.z()} : v;
}

template <ArithType T, ArithType U>
constexpr auto FaceForward(const Vector3<T>& v, const Normal3<U>& n2)
{
    return (Dot(v, n2) < To<CommonFloatType<T, U>>(0)) ? Vector3<T>{-v.x(), -v.y(), -v.z()} : v;
}

template <ArithType T>
constexpr auto Normalize(const Vector2<T>& v) -> Vector2<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x()) * inv, To<R>(v.y()) * inv};
}

template <ArithType T>
constexpr auto Normalize(const Vector3<T>& v) -> Vector3<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x()) * inv, To<R>(v.y()) * inv, To<R>(v.z()) * inv};
}

template <ArithType T>
constexpr auto Normalize(const Vector4<T>& v) -> Vector4<MapFloatType<T>>
{
    using R  = MapFloatType<T>;
    auto len = Length(v);
    BEE_DCHECK(len > 0);
    auto inv = To<R>(1) / To<R>(len);
    return {To<R>(v.x()) * inv, To<R>(v.y()) * inv, To<R>(v.z()) * inv, To<R>(v.w()) * inv};
}

template <ArithType T, ArithType U>
constexpr auto AngleBetween(const Vector3<T>& a, const Vector3<U>& b)
{
    using R    = CommonFloatType<T, U>;
    auto denom = To<R>(Length(a)) * To<R>(Length(b));
    BEE_DCHECK(denom > 0);
    auto c = To<R>(Dot(a, b)) / denom;
    return std::acos(std::clamp(c, To<R>(-1), To<R>(1)));
}

template <ArithType T, ArithType U>
constexpr auto AngleBetween(const Normal3<T>& a, const Normal3<U>& b)
{
    using R    = CommonFloatType<T, U>;
    auto denom = To<R>(Length(a)) * To<R>(Length(b));
    BEE_DCHECK(denom > 0);
    auto c = To<R>(Dot(a, b)) / denom;
    return std::acos(std::clamp(c, To<R>(-1), To<R>(1)));
}

template <ArithType T, ArithType U>
constexpr auto GramSchmidt(const Vector3<T>& v, const Vector3<U>& w)
{
    using R   = CommonFloatType<T, U>;
    auto proj = To<R>(Dot(v, w));
    return Vector3<R>(To<R>(v.x()) - proj * To<R>(w.x()),
                      To<R>(v.y()) - proj * To<R>(w.y()),
                      To<R>(v.z()) - proj * To<R>(w.z()));
}

template <ArithType T>
constexpr void CoordinateSystem(Vector3<T> v1, Vector3<T>* v2, Vector3<T>* v3)
{
    using R   = MapFloatType<T>;
    auto sign = std::copysign(To<R>(1), To<R>(v1.z()));
    auto a    = -To<R>(1) / (sign + To<R>(v1.z()));
    auto b    = To<R>(v1.x()) * To<R>(v1.y()) * a;
    *v2       = Vector3<T>(To<T>(1 + sign * To<R>(v1.x()) * To<R>(v1.x()) * a), To<T>(sign * b), To<T>(-sign * To<R>(v1.x())));
    *v3       = Vector3<T>(To<T>(b), To<T>(sign + To<R>(v1.y()) * To<R>(v1.y()) * a), To<T>(-To<R>(v1.y())));
}

template <ArithType T>
constexpr void CoordinateSystem(Normal3<T> v1, Vector3<T>* v2, Vector3<T>* v3)
{
    using R   = MapFloatType<T>;
    auto sign = std::copysign(To<R>(1), To<R>(v1.z()));
    auto a    = -To<R>(1) / (sign + To<R>(v1.z()));
    auto b    = To<R>(v1.x()) * To<R>(v1.y()) * a;
    *v2       = Vector3<T>(To<T>(1 + sign * To<R>(v1.x()) * To<R>(v1.x()) * a), To<T>(sign * b), To<T>(-sign * To<R>(v1.x())));
    *v3       = Vector3<T>(To<T>(b), To<T>(sign + To<R>(v1.y()) * To<R>(v1.y()) * a), To<T>(-To<R>(v1.y())));
}

template <FloatType T, FloatType U>
constexpr auto Lerp(const Point2<T>& p0, const Point2<T>& p1, U x)
{
    auto tx = To<T>(x);
    Point2<T> p;
    p.x() = Lerp(p0.x(), p1.x(), tx);
    p.y() = Lerp(p0.y(), p1.y(), tx);
    return p;
}

template <FloatType T, FloatType U>
constexpr auto Lerp(const Point3<T>& p0, const Point3<T>& p1, U x)
{
    auto tx = To<T>(x);
    Point3<T> p;
    p.x() = Lerp(p0.x(), p1.x(), tx);
    p.y() = Lerp(p0.y(), p1.y(), tx);
    p.z() = Lerp(p0.z(), p1.z(), tx);
    return p;
}

template <FloatType F>
constexpr F SphericalTheta(const Vector3<F>& v)
{
    BEE_DCHECK(v.z() >= To<F>(-1.0001) && v.z() <= To<F>(1.0001));
    return std::acos(std::clamp(v.z(), To<F>(-1), To<F>(1)));
}

template <FloatType F>
constexpr F SphericalPhi(const Vector3<F>& v)
{
    auto p = std::atan2(v.y(), v.x());
    return (p < 0) ? (p + TwoPi<F>()) : p;
}

template <FloatType F>
constexpr F CosTheta(const Vector3<F>& w)
{
    return w.z();
}

template <FloatType F>
constexpr F Cos2Theta(const Vector3<F>& w)
{
    return w.z() * w.z();
}

template <FloatType F>
constexpr F AbsCosTheta(const Vector3<F>& w)
{
    return std::abs(w.z());
}

template <FloatType F>
constexpr F Sin2Theta(const Vector3<F>& w)
{
    return std::max(To<F>(0), To<F>(1) - Cos2Theta(w));
}

template <FloatType F>
constexpr F SinTheta(const Vector3<F>& w)
{
    return std::sqrt(Sin2Theta(w));
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
    return (sinTheta == 0) ? To<F>(1) : std::clamp(w.x() / sinTheta, To<F>(-1), To<F>(1));
}

template <FloatType F>
constexpr F SinPhi(const Vector3<F>& w)
{
    auto sinTheta = SinTheta(w);
    return (sinTheta == 0) ? To<F>(0) : std::clamp(w.y() / sinTheta, To<F>(-1), To<F>(1));
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr auto CosDPhi(const Vector3<T>& wa, const Vector3<U>& wb) -> CommonFloatType<T, U>
{
    using R = CommonFloatType<T, U>;

    auto waxy = To<R>(wa.x() * wa.x() + wa.y() * wa.y());
    auto wbxy = To<R>(wb.x() * wb.x() + wb.y() * wb.y());

    if (waxy == 0 || wbxy == 0)
        return To<R>(1);

    return std::clamp(To<R>(wa.x() * wb.x() + wa.y() * wb.y()) / std::sqrt(waxy * wbxy), To<R>(-1), To<R>(1));
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr bool SameHemisphere(const Vector3<T>& w, const Vector3<U>& wp)
{
    return w.z() * wp.z() > 0;
}

template <ArithType T, ArithType U> requires (ArithType<T> && ArithType<U>)
constexpr bool SameHemisphere(const Vector3<T>& w, const Normal3<U>& wp)
{
    return w.z() * wp.z() > 0;
}

} // namespace bee
