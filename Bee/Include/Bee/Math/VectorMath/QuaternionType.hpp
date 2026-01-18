/**
 * @File QuaternionType.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026-01-18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Bee/Math/Common.hpp>
#include <Bee/Math/VectorMath/VectorType.hpp>
#include <Bee/Math/VectorMath/MatrixType.hpp>
#include <Bee/Math/VectorMath/CoordinateSystem.hpp>

namespace bee
{

template <ArithType T>
class Quaternion
{
public:
    using value_type = T;

    // clang-format off
    union
    {
        struct { T x, y, z, w; };
        struct { Vector3<T> xyz; T _w; };
        Vector4<T> v;
    };

    // ========== Constructors ==========

    constexpr Quaternion() noexcept;
    constexpr Quaternion(T x_, T y_, T z_, T w_) noexcept;
    constexpr Quaternion(const Vector3<T>& xyz_, T w_) noexcept;
    constexpr explicit Quaternion(const Vector4<T>& v_) noexcept;
    constexpr explicit Quaternion(const Matrix3x3<T>& m) noexcept;

    template <ArithType U> constexpr explicit Quaternion(const Quaternion<U>& q) noexcept;

    // ========== Operators ==========

    template <ArithType U> constexpr auto operator*(const Quaternion<U>& q) const noexcept -> Quaternion<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator*(const Vector3<U>& v) const noexcept -> Vector3<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator*(U s) const noexcept -> Quaternion<decltype(T{} * U{})>;
    template <ArithType U> constexpr auto operator+(const Quaternion<U>& q) const noexcept -> Quaternion<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator-(const Quaternion<U>& q) const noexcept -> Quaternion<decltype(T{} - U{})>;

    constexpr Quaternion operator-() const noexcept;

    constexpr bool operator==(const Quaternion& q) const noexcept;
    constexpr bool operator!=(const Quaternion& q) const noexcept;

    constexpr T& operator[](int i)       noexcept;
    constexpr T  operator[](int i) const noexcept;

    // ========== Basic Operations ==========

    constexpr Quaternion conjugate() const noexcept;
    constexpr T lengthSquared() const noexcept;
    constexpr T length() const noexcept;
    constexpr Quaternion normalize() const noexcept;
    constexpr Quaternion inverse() const noexcept;

    template <ArithType U> constexpr auto dot(const Quaternion<U>& q) const noexcept -> decltype(T{} * U{});
    template <ArithType U> constexpr auto rotateVector(const Vector3<U>& v) const noexcept -> Vector3<decltype(T{} + U{})>;

    constexpr bool hasNaN() const noexcept;
    constexpr bool isNormalized(T epsilon = T(1e-4)) const noexcept;
    constexpr bool isIdentity(T epsilon = T(1e-4)) const noexcept;

    // ========== Conversions ==========

    constexpr Matrix3x3<T> toMatrix3x3() const noexcept;
    constexpr Matrix4x4<T> toMatrix4x4() const noexcept;

    // Returns axis-angle representation
    [[nodiscard]] constexpr auto toAxisAngle() const noexcept -> std::pair<Vector3<T>, T>;
    constexpr void toAxisAngle(Vector3<T>& axis, T& angle) const noexcept;

    // Convert to Euler angles
    constexpr Vector3<T> toEuler(ECoordinateSystem coordSys = GetCoordinateSystem()) const noexcept;

    // ========== Interpolation ==========

    template <ArithType U> static constexpr auto Lerp(const Quaternion& q1, const Quaternion<U>& q2, T t) noexcept -> Quaternion<decltype(T{} + U{})>;

    // Spherical linear interpolation
    template <ArithType U> static constexpr Quaternion Slerp(const Quaternion& q1, const Quaternion<U>& q2, T t) noexcept;
    // Shortest path spherical interpolation
    template <ArithType U> static constexpr Quaternion SlerpShortestPath(const Quaternion& q1, const Quaternion<U>& q2, T t) noexcept;

    // ========== Static Factories ==========

    constexpr static Quaternion Identity() noexcept;
    constexpr static Quaternion FromAxisAngle(const Vector3<T>& axis, T angle) noexcept;
    constexpr static Quaternion FromEuler(T pitch, T yaw, T roll) noexcept;
    constexpr static Quaternion FromEuler(const Vector3<T>& euler) noexcept;
    constexpr static Quaternion FromRotationMatrix(const Matrix3x3<T>& m) noexcept;
    constexpr static Quaternion FromTwoVectors(const Vector3<T>& from, const Vector3<T>& to) noexcept;
    constexpr static Quaternion LookAt(const Vector3<T>& forward, const Vector3<T>& up = Vector3<T>::UnitY()) noexcept;
    // clang-format on
};

// ========== Type Aliases ==========

using Quaternionf = Quaternion<f32>;
using Quaterniond = Quaternion<f64>;
using Quatf       = Quaternion<f32>;
using Quatd       = Quaternion<f64>;

// ========== Global Operators ==========

template <ArithType T, ArithType U>
constexpr auto operator*(U s, const Quaternion<T>& q) noexcept -> Quaternion<decltype(U{} * T{})>;

// ========== Global Functions ==========

template <ArithType T, ArithType U>
constexpr auto Dot(const Quaternion<T>& q1, const Quaternion<U>& q2) noexcept -> decltype(T{} * U{});

template <ArithType T>
constexpr T Length(const Quaternion<T>& q) noexcept;

template <ArithType T>
constexpr Quaternion<T> Normalize(const Quaternion<T>& q) noexcept;

template <ArithType T>
constexpr Quaternion<T> Conjugate(const Quaternion<T>& q) noexcept;

template <ArithType T>
constexpr Quaternion<T> Inverse(const Quaternion<T>& q) noexcept;

template <ArithType T, ArithType U>
constexpr auto Lerp(const Quaternion<T>& q1, const Quaternion<U>& q2, f32 t) noexcept;

template <ArithType T, ArithType U>
constexpr auto Slerp(const Quaternion<T>& q1, const Quaternion<U>& q2, f32 t) noexcept;

// ==================== Implementation ====================

// ========== Constructors ==========

template <ArithType T>
constexpr Quaternion<T>::Quaternion() noexcept
    : x(T{}), y(T{}), z(T{}), w(T(1))
{
}

template <ArithType T>
constexpr Quaternion<T>::Quaternion(T x_, T y_, T z_, T w_) noexcept
    : x(x_), y(y_), z(z_), w(w_)
{
}

template <ArithType T>
constexpr Quaternion<T>::Quaternion(const Vector3<T>& xyz_, T w_) noexcept
    : x(xyz_.x), y(xyz_.y), z(xyz_.z), w(w_)
{
}

template <ArithType T>
constexpr Quaternion<T>::Quaternion(const Vector4<T>& v_) noexcept
    : x(v_.x), y(v_.y), z(v_.z), w(v_.w)
{
}

template <ArithType T>
constexpr Quaternion<T>::Quaternion(const Matrix3x3<T>& m) noexcept
{
    *this = FromRotationMatrix(m);
}

template <ArithType T>
template <ArithType U>
constexpr Quaternion<T>::Quaternion(const Quaternion<U>& q) noexcept
    : x(static_cast<T>(q.x)), y(static_cast<T>(q.y)),
      z(static_cast<T>(q.z)), w(static_cast<T>(q.w))
{
}

// ========== Operators ==========

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::operator*(const Quaternion<U>& q) const noexcept -> Quaternion<decltype(T{} + U{})>
{
    using R = decltype(T{} + U{});
    return Quaternion<R>{
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
    };
}

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::operator*(const Vector3<U>& v) const noexcept -> Vector3<decltype(T{} + U{})>
{
    using R = decltype(T{} + U{});
    Vector3<R> qv(x, y, z);
    Vector3<R> uv  = Cross(qv, Vector3<R>(v));
    Vector3<R> uuv = Cross(qv, uv);
    uv             *= To<R>(2) * w;
    uuv            *= To<R>(2);
    return v + uv + uuv;
}

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::operator*(U s) const noexcept -> Quaternion<decltype(T{} * U{})>
{
    return Quaternion<decltype(T{} * U{})>{x * s, y * s, z * s, w * s};
}

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::operator+(const Quaternion<U>& q) const noexcept -> Quaternion<decltype(T{} + U{})>
{
    return Quaternion<decltype(T{} + U{})>{x + q.x, y + q.y, z + q.z, w + q.w};
}

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::operator-(const Quaternion<U>& q) const noexcept -> Quaternion<decltype(T{} - U{})>
{
    return Quaternion<decltype(T{} - U{})>{x - q.x, y - q.y, z - q.z, w - q.w};
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::operator-() const noexcept
{
    return Quaternion{-x, -y, -z, -w};
}

template <ArithType T>
constexpr bool Quaternion<T>::operator==(const Quaternion& q) const noexcept
{
    return x == q.x && y == q.y && z == q.z && w == q.w;
}

template <ArithType T>
constexpr bool Quaternion<T>::operator!=(const Quaternion& q) const noexcept
{
    return !(*this == q);
}

template <ArithType T>
constexpr T& Quaternion<T>::operator[](int i) noexcept
{
    return (&x)[i];
}

template <ArithType T>
constexpr T Quaternion<T>::operator[](int i) const noexcept
{
    return (&x)[i];
}

// ========== Basic Operations ==========

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::conjugate() const noexcept
{
    return Quaternion{-x, -y, -z, w};
}

template <ArithType T>
constexpr T Quaternion<T>::lengthSquared() const noexcept
{
    return x * x + y * y + z * z + w * w;
}

template <ArithType T>
constexpr T Quaternion<T>::length() const noexcept
{
    return Sqrt(lengthSquared());
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::normalize() const noexcept
{
    const T len = length();
    BEE_DCHECK(len > T{});
    const T invLen = To<T>(1) / len;
    return Quaternion{x * invLen, y * invLen, z * invLen, w * invLen};
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::inverse() const noexcept
{
    const T lenSq = lengthSquared();
    BEE_DCHECK(lenSq > T{});
    const T invLenSq = To<T>(1) / lenSq;
    return Quaternion{-x * invLenSq, -y * invLenSq, -z * invLenSq, w * invLenSq};
}

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::dot(const Quaternion<U>& q) const noexcept -> decltype(T{} * U{})
{
    return x * q.x + y * q.y + z * q.z + w * q.w;
}

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::rotateVector(const Vector3<U>& v) const noexcept -> Vector3<decltype(T{} + U{})>
{
    return (*this) * v;
}

template <ArithType T>
constexpr bool Quaternion<T>::hasNaN() const noexcept
{
    return IsNaN(x) || IsNaN(y) || IsNaN(z) || IsNaN(w);
}

template <ArithType T>
constexpr bool Quaternion<T>::isNormalized(T epsilon) const noexcept
{
    return Abs(length() - T(1)) < epsilon;
}

template <ArithType T>
constexpr bool Quaternion<T>::isIdentity(T epsilon) const noexcept
{
    return Abs(x) < epsilon && Abs(y) < epsilon && Abs(z) < epsilon && Abs(w - T(1)) < epsilon;
}

// ========== Conversions ==========

template <ArithType T>
constexpr Matrix3x3<T> Quaternion<T>::toMatrix3x3() const noexcept
{
    // Assume unit quaternion
    const T xx = x * x, yy = y * y, zz = z * z;
    const T xy = x * y, xz = x * z, yz = y * z;
    const T wx = w * x, wy = w * y, wz = w * z;

    Matrix3x3<T> m;
    m.m[0] = To<T>(1) - To<T>(2) * (yy + zz); // m[0][0]
    m.m[1] = To<T>(2) * (xy + wz);            // m[1][0]
    m.m[2] = To<T>(2) * (xz - wy);            // m[2][0]

    m.m[3] = To<T>(2) * (xy - wz);            // m[0][1]
    m.m[4] = To<T>(1) - To<T>(2) * (xx + zz); // m[1][1]
    m.m[5] = To<T>(2) * (yz + wx);            // m[2][1]

    m.m[6] = To<T>(2) * (xz + wy);            // m[0][2]
    m.m[7] = To<T>(2) * (yz - wx);            // m[1][2]
    m.m[8] = To<T>(1) - To<T>(2) * (xx + yy); // m[2][2]

    return m;
}

template <ArithType T>
constexpr Matrix4x4<T> Quaternion<T>::toMatrix4x4() const noexcept
{
    Matrix4x4<T> m      = Matrix4x4<T>::Identity();
    Matrix3x3<T> rot3x3 = toMatrix3x3(); // Call once
    m.c0                = Vector4<T>{rot3x3.c0, T{}};
    m.c1                = Vector4<T>{rot3x3.c1, T{}};
    m.c2                = Vector4<T>{rot3x3.c2, T{}};
    return m;
}

template <ArithType T>
constexpr auto Quaternion<T>::toAxisAngle() const noexcept -> std::pair<Vector3<T>, T>
{
    const T len = Sqrt(x * x + y * y + z * z);
    if (len > T{})
    {
        return {Vector3<T>{x, y, z} / len, To<T>(2) * Acos(Clamp(w, T(-1), T(1)))};
    }
    return {Vector3<T>::UnitX(), T{}};
}

template <ArithType T>
constexpr void Quaternion<T>::toAxisAngle(Vector3<T>& axis, T& angle) const noexcept
{
    auto [axisResult, angleResult] = toAxisAngle();
    axis                           = axisResult;
    angle                          = angleResult;
}

template <ArithType T>
constexpr Vector3<T> Quaternion<T>::toEuler(ECoordinateSystem coordSys) const noexcept
{
    // Adjust Euler calculation based on handedness
    // Yaw (around Y axis)
    const T sinY = To<T>(2) * (w * y + z * x);
    const T cosY = To<T>(1) - To<T>(2) * (x * x + y * y);
    const T yaw  = MathAtan2(sinY, cosY);

    // Pitch (around X axis)
    const T sinP = To<T>(2) * (w * x - y * z);
    T pitch;
    if (Abs(sinP) >= T(1))
    {
        pitch = CopySign(To<T>(kPi) / To<T>(2), sinP); // 90 degrees
    }
    else
    {
        pitch = MathAsin(sinP);
    }

    // Roll (around Z axis)
    const T sinR = To<T>(2) * (w * z + x * y);
    const T cosR = To<T>(1) - To<T>(2) * (y * y + z * z);
    const T roll = MathAtan2(sinR, cosR);

    if (coordSys == ECoordinateSystem::LeftHanded)
    {
        // Left-handed: yaw and roll around Z need negation?
        // Original code: return Vector3<T>{pitch, -yaw, -roll};
        return Vector3<T>{pitch, -yaw, -roll};
    }
    return Vector3<T>{pitch, yaw, roll};
}

// ========== Interpolation ==========

template <ArithType T>
template <ArithType U>
constexpr auto Quaternion<T>::Lerp(const Quaternion& q1, const Quaternion<U>& q2, T t) noexcept -> Quaternion<decltype(T{} + U{})>
{
    using R = decltype(T{} + U{});
    return Quaternion<R>{
            q1.x + (q2.x - q1.x) * t,
            q1.y + (q2.y - q1.y) * t,
            q1.z + (q2.z - q1.z) * t,
            q1.w + (q2.w - q1.w) * t
    };
}

template <ArithType T>
template <ArithType U>
constexpr Quaternion<T> Quaternion<T>::Slerp(const Quaternion& q1, const Quaternion<U>& q2, T t) noexcept
{
    // Calculate cosine of angle
    T cosOmega = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

    // If cosOmega < 0, negate one to take shortest path? 
    // Wait, Slerp usually doesn't force shortest path unless requested, but original code did:
    // "If cosOmega < 0, take negative to choose shortest path"
    Quaternion q2Temp(q2);
    if (cosOmega < T{})
    {
        cosOmega = -cosOmega;
        q2Temp   = -q2Temp;
    }

    // If close, use linear interpolation
    const T kDelta = T(1e-4);
    if (cosOmega > T(1) - kDelta)
    {
        return Lerp(q1, q2Temp, t).normalize();
    }

    // Calculate omega
    const T omega    = Acos(Clamp(cosOmega, T(-1), T(1)));
    const T sinOmega = Sin(omega);

    const T scale0 = Sin((To<T>(1) - t) * omega) / sinOmega;
    const T scale1 = Sin(t * omega) / sinOmega;

    return Quaternion{
            scale0 * q1.x + scale1 * q2Temp.x,
            scale0 * q1.y + scale1 * q2Temp.y,
            scale0 * q1.z + scale1 * q2Temp.z,
            scale0 * q1.w + scale1 * q2Temp.w
    };
}

template <ArithType T>
template <ArithType U>
constexpr Quaternion<T> Quaternion<T>::SlerpShortestPath(const Quaternion& q1, const Quaternion<U>& q2, T t) noexcept
{
    // Ensure shortest path
    T cosOmega = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    Quaternion q2Temp(q2);
    if (cosOmega < T{})
    {
        q2Temp   = -q2Temp;
        cosOmega = -cosOmega;
    }

    return Slerp(q1, q2Temp, t);
}

// ========== Static Factories ==========

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::Identity() noexcept
{
    return Quaternion{};
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::FromAxisAngle(const Vector3<T>& axis, T angle) noexcept
{
    const T halfAngle = angle / To<T>(2);
    const T s         = Sin(halfAngle);
    const T c         = Cos(halfAngle);

    // Normalize axis
    const T len = bee::Length(axis);
    BEE_DCHECK(len > T{});
    const T invLen = To<T>(1) / len;

    return Quaternion{
            axis.x * invLen * s,
            axis.y * invLen * s,
            axis.z * invLen * s,
            c
    };
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::FromEuler(T pitch, T yaw, T roll) noexcept
{
    const T cy = Cos(yaw * To<T>(0.5));
    const T sy = Sin(yaw * To<T>(0.5));
    const T cp = Cos(pitch * To<T>(0.5));
    const T sp = Sin(pitch * To<T>(0.5));
    const T cr = Cos(roll * To<T>(0.5));
    const T sr = Sin(roll * To<T>(0.5));

    // clang-format off
    return Quaternion{
            cr * sp * cy - sr * cp * sy, // x
            cr * cp * sy + sr * sp * cy, // y
            sr * cp * cy - cr * sp * sy, // z
            cr * cp * cy + sr * sp * sy // w
    };
    // clang-format on
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::FromEuler(const Vector3<T>& euler) noexcept
{
    return FromEuler(euler.x, euler.y, euler.z);
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::FromRotationMatrix(const Matrix3x3<T>& m) noexcept
{
    Quaternion q;

    const T trace = m.m[0] + m.m[4] + m.m[8];
    if (trace > T{})
    {
        const T s = Sqrt(trace + To<T>(1)) * To<T>(2); // s = 4 * qw
        q.w       = To<T>(0.25) * s;
        q.x       = (m.m[5] - m.m[7]) / s;
        q.y       = (m.m[6] - m.m[2]) / s;
        q.z       = (m.m[1] - m.m[3]) / s;
    }
    else if ((m.m[0] > m.m[4]) && (m.m[0] > m.m[8]))
    {
        const T s = Sqrt(To<T>(1) + m.m[0] - m.m[4] - m.m[8]) * To<T>(2); // s = 4 * qx
        q.w       = (m.m[5] - m.m[7]) / s;
        q.x       = To<T>(0.25) * s;
        q.y       = (m.m[3] + m.m[1]) / s;
        q.z       = (m.m[6] + m.m[2]) / s;
    }
    else if (m.m[4] > m.m[8])
    {
        const T s = Sqrt(To<T>(1) + m.m[4] - m.m[0] - m.m[8]) * To<T>(2); // s = 4 * qy
        q.w       = (m.m[6] - m.m[2]) / s;
        q.x       = (m.m[3] + m.m[1]) / s;
        q.y       = To<T>(0.25) * s;
        q.z       = (m.m[7] + m.m[5]) / s;
    }
    else
    {
        const T s = Sqrt(To<T>(1) + m.m[8] - m.m[0] - m.m[4]) * To<T>(2); // s = 4 * qz
        q.w       = (m.m[1] - m.m[3]) / s;
        q.x       = (m.m[6] + m.m[2]) / s;
        q.y       = (m.m[7] + m.m[5]) / s;
        q.z       = To<T>(0.25) * s;
    }

    return q.normalize();
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::FromTwoVectors(const Vector3<T>& from, const Vector3<T>& to) noexcept
{
    // Calculate rotation axis (cross product)
    Vector3<T> axis = bee::Cross(from, to);
    const T len     = bee::Length(axis);

    // If vectors are parallel
    if (len < T(1e-6))
    {
        // Check if same direction
        if (bee::Dot(from, to) > T{})
        {
            return Identity();
        }
        else
        {
            // Opposite direction, rotate 180 degrees around orthogonal axis
            axis = bee::Orthogonal(from);
            return FromAxisAngle(axis, To<T>(kPi));
        }
    }

    // Calculate angle (dot product)
    const T dot   = Clamp(bee::Dot(from, to), T(-1), T(1));
    const T angle = Acos(dot);

    return FromAxisAngle(axis, angle);
}

template <ArithType T>
constexpr Quaternion<T> Quaternion<T>::LookAt(const Vector3<T>& forward, const Vector3<T>& up) noexcept
{
    Vector3<T> f = bee::Normalize(forward);
    Vector3<T> r = bee::Normalize(bee::Cross(up, f));
    Vector3<T> u = bee::Cross(f, r);

    Matrix3x3<T> m;
    m.c0 = r;
    m.c1 = u;
    m.c2 = f;

    return FromRotationMatrix(m);
}

// ========== Global Operators ==========

template <ArithType T, ArithType U>
constexpr auto operator*(U s, const Quaternion<T>& q) noexcept -> Quaternion<decltype(U{} * T{})>
{
    return Quaternion<decltype(U{} * T{})>{q.x * s, q.y * s, q.z * s, q.w * s};
}

// ========== Global Functions ==========

template <ArithType T, ArithType U>
constexpr auto Dot(const Quaternion<T>& q1, const Quaternion<U>& q2) noexcept -> decltype(T{} * U{})
{
    return q1.dot(q2);
}

template <ArithType T>
constexpr T Length(const Quaternion<T>& q) noexcept
{
    return q.length();
}

template <ArithType T>
constexpr Quaternion<T> Normalize(const Quaternion<T>& q) noexcept
{
    return q.normalize();
}

template <ArithType T>
constexpr Quaternion<T> Conjugate(const Quaternion<T>& q) noexcept
{
    return q.conjugate();
}

template <ArithType T>
constexpr Quaternion<T> Inverse(const Quaternion<T>& q) noexcept
{
    return q.inverse();
}

template <ArithType T, ArithType U>
constexpr auto Lerp(const Quaternion<T>& q1, const Quaternion<U>& q2, f32 t) noexcept
{
    return Quaternion<T>::Lerp(q1, q2, t);
}

template <ArithType T, ArithType U>
constexpr auto Slerp(const Quaternion<T>& q1, const Quaternion<U>& q2, f32 t) noexcept
{
    return Quaternion<T>::Slerp(q1, q2, t);
}

} // namespace bee
