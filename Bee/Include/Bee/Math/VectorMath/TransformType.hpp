/**
 * @File TransformType.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026-01-18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Bee/Math/VectorMath/VectorType.hpp>
#include <Bee/Math/VectorMath/QuaternionType.hpp>
#include <Bee/Math/VectorMath/MatrixType.hpp>
#include <Bee/Math/Common.hpp>

namespace bee
{

template <ArithType T>
class Transform3D
{
public:
    Vector3<T> translation; ///< 平移分量
    Quaternion<T> rotation; ///< 旋转分量（四元数）
    T scale = T(1);         ///< 缩放分量（均匀缩放）

    // clang-format off
    // ========== 构造函数 ==========

    constexpr Transform3D() noexcept = default;
    constexpr Transform3D(const Vector3<T>& t, const Quaternion<T>& r, T s = T(1)) noexcept;
    constexpr Transform3D(const Vector3<T>& t, const Quaternion<T>& r) noexcept;
    
    constexpr explicit Transform3D(const Vector3<T>& t) noexcept;
    constexpr explicit Transform3D(const Quaternion<T>& r) noexcept;
    constexpr explicit Transform3D(const Matrix4x4<T>& m) noexcept;

    // ========== 变换操作 ==========

    template <ArithType U> constexpr auto operator()(const Point3<U>& p) const noexcept -> Point3<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator()(const Vector3<U>& v) const noexcept -> Vector3<decltype(T{} * U{})>;
    template <ArithType U> constexpr auto operator()(const Normal3<U>& n) const noexcept -> Normal3<decltype(T{})>;
    template <ArithType U> constexpr auto operator*(const Transform3D<U>& other) const noexcept -> Transform3D<decltype(T{} + U{})>;

    constexpr Transform3D inverse() const noexcept;

    // ========== 转换 ==========

    constexpr Matrix4x4<T> toMatrix() const noexcept;
    constexpr Matrix3x3<T> toMatrix3x3() const noexcept;

    // ========== 静态工厂 ==========

    constexpr static Transform3D Identity() noexcept;
    constexpr static Transform3D Translate(const Vector3<T>& t) noexcept;
    constexpr static Transform3D Rotate(const Quaternion<T>& r) noexcept;
    constexpr static Transform3D Scale(T s) noexcept;
    constexpr static Transform3D TRS(const Vector3<T>& t, const Quaternion<T>& r, T s) noexcept;
    constexpr static Transform3D Rotate(const Vector3<T>& axis, T angle) noexcept;
    constexpr static Transform3D Rotate(T pitch, T yaw, T roll) noexcept;
    constexpr static Transform3D LookAt(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up = Vector3<T>::UnitY()) noexcept;
    constexpr static Transform3D FromMatrix(const Matrix4x4<T>& m) noexcept;

    // ========== 工具函数 ==========

    constexpr bool hasNaN() const noexcept;
    constexpr bool isIdentity(T epsilon = T(1e-4)) const noexcept;

    constexpr Vector3<T> forward() const noexcept;
    constexpr Vector3<T> right() const noexcept;
    constexpr Vector3<T> up() const noexcept;
    // clang-format on
};

template <ArithType T>
class Transform2D
{
public:
    Vector2<T> translation; ///< 平移分量
    T rotation = T{};       ///< 旋转分量（弧度）
    T scale    = T(1);      ///< 缩放分量

    // clang-format off
    // ========== 构造函数 ==========

    constexpr Transform2D() noexcept = default;
    constexpr Transform2D(const Vector2<T>& t, T r, T s = T(1)) noexcept;
    constexpr explicit Transform2D(const Vector2<T>& t) noexcept;
    constexpr explicit Transform2D(T r) noexcept;

    // ========== 变换操作 ==========

    template <ArithType U> constexpr auto operator()(const Point2<U>& p) const noexcept -> Point2<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator()(const Vector2<U>& v) const noexcept -> Vector2<decltype(T{} * U{})>;
    template <ArithType U> constexpr auto operator*(const Transform2D<U>& other) const noexcept -> Transform2D<decltype(T{} + U{})>;

    constexpr Transform2D inverse() const noexcept;

    // ========== 转换 ==========

    constexpr Matrix3x3<T> toMatrix() const noexcept;

    // ========== 静态工厂 ==========

    constexpr static Transform2D Identity() noexcept;
    constexpr static Transform2D Translate(const Vector2<T>& t) noexcept;
    constexpr static Transform2D Rotate(T angle) noexcept;
    constexpr static Transform2D Scale(T s) noexcept;
    constexpr static Transform2D TRS(const Vector2<T>& t, T r, T s) noexcept;

    // ========== 工具函数 ==========

    constexpr bool hasNaN() const noexcept;
    constexpr bool isIdentity(T epsilon = T(1e-4)) const noexcept;
    // clang-format on
};

// ========== Transform3D Implementation ==========

template <ArithType T>
constexpr Transform3D<T>::Transform3D(const Vector3<T>& t, const Quaternion<T>& r, T s) noexcept
    : translation(t), rotation(r), scale(s)
{
}

template <ArithType T>
constexpr Transform3D<T>::Transform3D(const Vector3<T>& t, const Quaternion<T>& r) noexcept
    : translation(t), rotation(r), scale(T(1))
{
}

template <ArithType T>
constexpr Transform3D<T>::Transform3D(const Vector3<T>& t) noexcept
    : translation(t), rotation(Quaternion<T>::Identity()), scale(T(1))
{
}

template <ArithType T>
constexpr Transform3D<T>::Transform3D(const Quaternion<T>& r) noexcept
    : translation(Vector3<T>::Zero()), rotation(r), scale(T(1))
{
}

template <ArithType T>
constexpr Transform3D<T>::Transform3D(const Matrix4x4<T>& m) noexcept
{
    *this = FromMatrix(m);
}

template <ArithType T>
template <ArithType U>
constexpr auto Transform3D<T>::operator()(const Point3<U>& p) const noexcept -> Point3<decltype(T{} + U{})>
{
    // p' = scale * (rotation * p) + translation
    return Point3<decltype(T{} + U{})>{(rotation.rotateVector(Vector3<decltype(T{} + U{})>(p))) * scale + translation};
}

template <ArithType T>
template <ArithType U>
constexpr auto Transform3D<T>::operator()(const Vector3<U>& v) const noexcept -> Vector3<decltype(T{} * U{})>
{
    // v' = scale * (rotation * v)
    return rotation.rotateVector(v) * scale;
}

template <ArithType T>
template <ArithType U>
constexpr auto Transform3D<T>::operator()(const Normal3<U>& n) const noexcept -> Normal3<decltype(T{})>
{
    // 法线只受旋转影响，忽略平移和缩放
    using R          = decltype(T{});
    auto m3x3        = rotation.toMatrix3x3();
    auto transformed = m3x3 * n;
    return Normal3<R>(Normalize(transformed));
}

template <ArithType T>
template <ArithType U>
constexpr auto Transform3D<T>::operator*(const Transform3D<U>& other) const noexcept -> Transform3D<decltype(T{} + U{})>
{
    using R = decltype(T{} + U{});
    return Transform3D<R>{
            translation + rotation.rotateVector(other.translation * scale),
            rotation * other.rotation,
            scale * other.scale
    };
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::inverse() const noexcept
{
    // T^{-1} = (R^{-1}, -R^{-1} * t / s, 1/s)
    const T invScale = To<T>(1) / scale;

    const Quaternion<T> invRotation = rotation.inverse();
    return Transform3D{
            invRotation.rotateVector(-translation) * invScale,
            invRotation,
            invScale
    };
}

template <ArithType T>
constexpr Matrix4x4<T> Transform3D<T>::toMatrix() const noexcept
{
    // M = T * R * S

    // 先计算 R * S
    Matrix4x4<T> m = rotation.toMatrix4x4();
    m.c0           *= scale;
    m.c1           *= scale;
    m.c2           *= scale;
    // 再应用平移
    m.c3 = Vector4<T>{translation, T(1)};

    return m;
}

template <ArithType T>
constexpr Matrix3x3<T> Transform3D<T>::toMatrix3x3() const noexcept
{
    Matrix3x3<T> m = rotation.toMatrix3x3();
    m.c0           *= scale;
    m.c1           *= scale;
    m.c2           *= scale;
    return m;
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::Identity() noexcept
{
    return Transform3D{};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::Translate(const Vector3<T>& t) noexcept
{
    return Transform3D{t, Quaternion<T>::Identity(), T(1)};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::Rotate(const Quaternion<T>& r) noexcept
{
    return Transform3D{Vector3<T>{}, r, T(1)};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::Scale(T s) noexcept
{
    return Transform3D{Vector3<T>{}, Quaternion<T>::Identity(), s};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::TRS(const Vector3<T>& t, const Quaternion<T>& r, T s) noexcept
{
    return Transform3D{t, r, s};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::Rotate(const Vector3<T>& axis, T angle) noexcept
{
    return Transform3D{Vector3<T>{}, Quaternion<T>::FromAxisAngle(axis, angle), T(1)};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::Rotate(T pitch, T yaw, T roll) noexcept
{
    return Transform3D{Vector3<T>{}, Quaternion<T>::FromEuler(pitch, yaw, roll), T(1)};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::LookAt(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up) noexcept
{
    const Vector3<T> forward    = Normalize(target - eye);
    const Vector3<T> right      = Normalize(Cross(up, forward));
    const Vector3<T> computedUp = Cross(forward, right);

    Matrix3x3<T> rotation;
    rotation.c0 = right;
    rotation.c1 = computedUp;
    rotation.c2 = forward;

    return Transform3D{eye, Quaternion<T>::FromRotationMatrix(rotation), T(1)};
}

template <ArithType T>
constexpr Transform3D<T> Transform3D<T>::FromMatrix(const Matrix4x4<T>& m) noexcept
{
    Transform3D t;

    // 提取平移
    t.translation = Vector3<T>{m.c3.x, m.c3.y, m.c3.z};
    // 提取缩放
    t.scale = bee::Length(m.c0.xyz());
    // 提取旋转
    Matrix3x3<T> rotationMatrix;
    rotationMatrix.c0 = m.c0.xyz() / t.scale;
    rotationMatrix.c1 = m.c1.xyz() / t.scale;
    rotationMatrix.c2 = m.c2.xyz() / t.scale;

    t.rotation = Quaternion<T>::FromRotationMatrix(rotationMatrix);

    return t;
}

template <ArithType T>
constexpr bool Transform3D<T>::hasNaN() const noexcept
{
    return IsNaN(translation.x) || IsNaN(translation.y) || IsNaN(translation.z) || rotation.hasNaN() || IsNaN(scale);
}

template <ArithType T>
constexpr bool Transform3D<T>::isIdentity(T epsilon) const noexcept
{
    return translation.isZero(epsilon) && rotation.isIdentity(epsilon) && Abs(scale - T(1)) < epsilon;
}

template <ArithType T>
constexpr Vector3<T> Transform3D<T>::forward() const noexcept
{
    return rotation * Vector3<T>::UnitZ();
}

template <ArithType T>
constexpr Vector3<T> Transform3D<T>::right() const noexcept
{
    return rotation * Vector3<T>::UnitX();
}

template <ArithType T>
constexpr Vector3<T> Transform3D<T>::up() const noexcept
{
    return rotation * Vector3<T>::UnitY();
}

// ========== 类型别名 ==========

using Transform3Df = Transform3D<f32>;
using Transform3Dd = Transform3D<f64>;

// ========== 全局函数 ==========

template <ArithType T, ArithType U>
constexpr auto TransformPoint(const Transform3D<T>& t, const Point3<U>& p) noexcept
{
    return t(p);
}

template <ArithType T, ArithType U>
constexpr auto TransformVector(const Transform3D<T>& t, const Vector3<U>& v) noexcept
{
    return t(v);
}

template <ArithType T, ArithType U>
constexpr auto TransformNormal(const Transform3D<T>& t, const Normal3<U>& n) noexcept
{
    return t(n);
}

template <ArithType T>
constexpr Transform3D<T> Inverse(const Transform3D<T>& t) noexcept
{
    return t.inverse();
}

// ========== Transform2D Implementation ==========

template <ArithType T>
constexpr Transform2D<T>::Transform2D(const Vector2<T>& t, T r, T s) noexcept
    : translation(t), rotation(r), scale(s)
{
}

template <ArithType T>
constexpr Transform2D<T>::Transform2D(const Vector2<T>& t) noexcept
    : translation(t), rotation(T{}), scale(T(1))
{
}

template <ArithType T>
constexpr Transform2D<T>::Transform2D(T r) noexcept
    : translation(T{}), rotation(r), scale(T(1))
{
}

template <ArithType T>
template <ArithType U>
constexpr auto Transform2D<T>::operator()(const Point2<U>& p) const noexcept -> Point2<decltype(T{} + U{})>
{
    const T c = Cos(rotation);
    const T s = Sin(rotation);
    using R   = decltype(T{} + U{});

    // 先旋转
    R x = c * p.x - s * p.y;
    R y = s * p.x + c * p.y;

    // 再缩放和平移
    return Point2<R>{x * scale + translation.x, y * scale + translation.y};
}

template <ArithType T>
template <ArithType U>
constexpr auto Transform2D<T>::operator()(const Vector2<U>& v) const noexcept -> Vector2<decltype(T{} * U{})>
{
    const T c = Cos(rotation);
    const T s = Sin(rotation);
    using R   = decltype(T{} * U{});

    return Vector2<R>{
            (c * v.x - s * v.y) * scale,
            (s * v.x + c * v.y) * scale
    };
}

template <ArithType T>
template <ArithType U>
constexpr auto Transform2D<T>::operator*(const Transform2D<U>& other) const noexcept -> Transform2D<decltype(T{} + U{})>
{
    using R   = decltype(T{} + U{});
    const T c = Cos(rotation);
    const T s = Sin(rotation);

    // 先应用 other，再应用 this
    return Transform2D<R>{
            Vector2<R>{
                    c * other.translation.x - s * other.translation.y + translation.x,
                    s * other.translation.x + c * other.translation.y + translation.y
            },
            rotation + other.rotation,
            scale * other.scale
    };
}

template <ArithType T>
constexpr Transform2D<T> Transform2D<T>::inverse() const noexcept
{
    const T invScale = To<T>(1) / scale;
    const T negRot   = -rotation;
    const T c        = Cos(negRot);
    const T s        = Sin(negRot);

    // 平移需要先缩放，再旋转
    const T tx = -translation.x * invScale;
    const T ty = -translation.y * invScale;

    return Transform2D{
            Vector2<T>{c * tx - s * ty, s * tx + c * ty},
            negRot,
            invScale
    };
}

template <ArithType T>
constexpr Matrix3x3<T> Transform2D<T>::toMatrix() const noexcept
{
    const T c = Cos(rotation);
    const T s = Sin(rotation);

    Matrix3x3<T> m;
    m.m[0] = c * scale;
    m.m[3] = -s * scale;
    m.m[6] = translation.x;
    m.m[1] = s * scale;
    m.m[4] = c * scale;
    m.m[7] = translation.y;
    m.m[2] = T{};
    m.m[5] = T{};
    m.m[8] = T(1);

    return m;
}

template <ArithType T>
constexpr Transform2D<T> Transform2D<T>::Identity() noexcept
{
    return Transform2D{};
}

template <ArithType T>
constexpr Transform2D<T> Transform2D<T>::Translate(const Vector2<T>& t) noexcept
{
    return Transform2D{t, T{}, T(1)};
}

template <ArithType T>
constexpr Transform2D<T> Transform2D<T>::Rotate(T angle) noexcept
{
    return Transform2D{Vector2<T>{}, angle, T(1)};
}

template <ArithType T>
constexpr Transform2D<T> Transform2D<T>::Scale(T s) noexcept
{
    return Transform2D{Vector2<T>{}, T{}, s};
}

template <ArithType T>
constexpr Transform2D<T> Transform2D<T>::TRS(const Vector2<T>& t, T r, T s) noexcept
{
    return Transform2D{t, r, s};
}

template <ArithType T>
constexpr bool Transform2D<T>::hasNaN() const noexcept
{
    return translation.hasNaN() || IsNaN(rotation) || IsNaN(scale);
}

template <ArithType T>
constexpr bool Transform2D<T>::isIdentity(T epsilon) const noexcept
{
    return translation.isZero(epsilon) && Abs(rotation) < epsilon && Abs(scale - T(1)) < epsilon;
}

// ========== 类型别名 ==========

using Transform2Df = Transform2D<f32>;
using Transform2Dd = Transform2D<f64>;

} // namespace bee
