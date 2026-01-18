/**
 * @File MatrixType.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Bee/Math/Common.hpp"
#include "Bee/Math/VectorMath/VectorType.hpp"
#include "Bee/Math/VectorMath/CoordinateSystem.hpp"
#include "Bee/Core/Portable.hpp"

namespace bee
{
// Forward declarations
// clang-format off
template <ArithType T> class Quaternion;
template <ArithType T> class Matrix2x2;
template <ArithType T> class Matrix3x3;
template <ArithType T> class Matrix4x4;
// clang-format on

// ==================================================================================
// Matrix2x2 Declaration
// ==================================================================================

template <ArithType T>
class Matrix2x2
{
public:
    using value_type               = T;
    static constexpr int RowCount  = 2;
    static constexpr int ColCount  = 2;
    static constexpr int Dimension = 4;

    // clang-format off
    #ifdef BEE_GPU_CODE
    T m[4];
    #else
    union
    {
        T m[4];
        struct { T m00, m10, m01, m11; };
        struct { Vector2<T> c0, c1; };
    };
    #endif

    // ========== Constructors ==========

    constexpr Matrix2x2() noexcept;
    constexpr explicit Matrix2x2(T diag) noexcept;
    
    constexpr Matrix2x2(T m00, T m01, 
                        T m10, T m11) noexcept;
    
    constexpr Matrix2x2(const Vector2<T>& c0, const Vector2<T>& c1) noexcept;

    template <ArithType U> constexpr explicit Matrix2x2(const Matrix2x2<U>& other) noexcept;

    // ========== Static Factories ==========

    constexpr static Matrix2x2 Identity() noexcept;
    constexpr static Matrix2x2 Zero() noexcept;

    // ========== Accessors ==========

    constexpr T& operator()(int col, int row)       noexcept;
    constexpr T  operator()(int col, int row) const noexcept;
    
    constexpr Vector2<T> col(int col) const noexcept;
    constexpr Vector2<T> row(int row) const noexcept;

    // ========== Operators ==========

    constexpr Matrix2x2  operator*(const Matrix2x2& other) const noexcept;
    constexpr Vector2<T> operator*(const Vector2<T>& v) const noexcept;
    constexpr Normal2<T> operator*(const Normal2<T>& n) const noexcept;
    constexpr Matrix2x2  operator*(T scalar) const noexcept;
    
    constexpr Matrix2x2& operator*=(const Matrix2x2& other) noexcept;
    constexpr Matrix2x2& operator*=(T scalar) noexcept;
    
    constexpr Matrix2x2 operator+(const Matrix2x2& other) const noexcept;
    constexpr Matrix2x2 operator-(const Matrix2x2& other) const noexcept;
    
    constexpr bool operator==(const Matrix2x2& other) const noexcept;
    constexpr bool operator!=(const Matrix2x2& other) const noexcept;

    // ========== Matrix Operations ==========

    constexpr T determinant() const noexcept;
    constexpr Matrix2x2 transpose() const noexcept;
    constexpr Matrix2x2 adjugate() const noexcept;
    constexpr Matrix2x2 inverse() const noexcept;
    constexpr T trace() const noexcept;

    // ========== Static Factories - Transforms ==========

    static constexpr Matrix2x2 Rotate(T angle) noexcept;
    static constexpr Matrix2x2 Scale(T sx, T sy) noexcept;
    static constexpr Matrix2x2 Scale(const Vector2<T>& s) noexcept;
    static constexpr Matrix2x2 ShearX(T sy) noexcept;
    static constexpr Matrix2x2 ShearY(T sx) noexcept;

    // ========== Utilities ==========

    constexpr bool hasNaN() const noexcept;
    constexpr bool isIdentity(T eps = T(1e-6)) const noexcept;
    // clang-format on
};

// ==================================================================================
// Matrix3x3 Declaration
// ==================================================================================

template <ArithType T>
class Matrix3x3
{
public:
    using value_type               = T;
    static constexpr int RowCount  = 3;
    static constexpr int ColCount  = 3;
    static constexpr int Dimension = 9;

    // clang-format off
    #ifdef BEE_GPU_CODE
    T m[9];
    #else
    union
    {
        T m[9];
        struct { Vector3<T> c0, c1, c2; };
    };
    #endif

    // ========== Constructors ==========

    constexpr Matrix3x3() noexcept;
    constexpr explicit Matrix3x3(T diag) noexcept;
    
    constexpr Matrix3x3(T m00, T m01, T m02, 
                        T m10, T m11, T m12,
                        T m20, T m21, T m22) noexcept;
    
    constexpr Matrix3x3(const Vector3<T>& c0, const Vector3<T>& c1, const Vector3<T>& c2) noexcept;

    template <ArithType U> constexpr explicit Matrix3x3(const Matrix3x3<U>& other) noexcept;

    // ========== Static Factories ==========

    constexpr static Matrix3x3 Identity() noexcept;
    constexpr static Matrix3x3 Zero() noexcept;

    // ========== Accessors ==========

    constexpr T& operator()(int col, int row)       noexcept;
    constexpr T  operator()(int col, int row) const noexcept;
    
    constexpr Vector3<T> col(int col) const noexcept;
    constexpr Vector3<T> row(int row) const noexcept;

    // ========== Operators ==========

    constexpr Matrix3x3  operator*(const Matrix3x3& other) const noexcept;
    constexpr Vector3<T> operator*(const Vector3<T>& v) const noexcept;
    constexpr Normal3<T> operator*(const Normal3<T>& n) const noexcept;
    constexpr Matrix3x3  operator*(T scalar) const noexcept;
    
    constexpr Matrix3x3& operator*=(const Matrix3x3& other) noexcept;
    constexpr Matrix3x3& operator*=(T scalar) noexcept;
    
    constexpr Matrix3x3 operator+(const Matrix3x3& other) const noexcept;
    constexpr Matrix3x3 operator-(const Matrix3x3& other) const noexcept;
    
    constexpr bool operator==(const Matrix3x3& other) const noexcept;
    constexpr bool operator!=(const Matrix3x3& other) const noexcept;

    // ========== Matrix Operations ==========

    constexpr T determinant() const noexcept;
    constexpr Matrix3x3 transpose() const noexcept;
    constexpr Matrix3x3 adjugate() const noexcept;
    constexpr Matrix3x3 cofactor() const noexcept;
    constexpr Matrix3x3 inverse() const noexcept;
    constexpr T trace() const noexcept;

    // ========== Static Factories - Transforms ==========

    static constexpr Matrix3x3 RotateX(T angle) noexcept;
    static constexpr Matrix3x3 RotateY(T angle) noexcept;
    static constexpr Matrix3x3 RotateZ(T angle) noexcept;
    static constexpr Matrix3x3 Rotate(T angle, const Vector3<T>& axis) noexcept;
    static constexpr Matrix3x3 Scale(T sx, T sy, T sz) noexcept;
    static constexpr Matrix3x3 Scale(const Vector3<T>& s) noexcept;

    // ========== Utilities ==========

    constexpr bool hasNaN() const noexcept;
    constexpr bool isIdentity(T eps = T(1e-6)) const noexcept;
    constexpr bool isOrthogonal(T eps = T(1e-4)) const noexcept;
    // clang-format on
};

// ==================================================================================
// Matrix4x4 Declaration
// ==================================================================================

template <ArithType T>
class Matrix4x4
{
public:
    using value_type               = T;
    static constexpr int RowCount  = 4;
    static constexpr int ColCount  = 4;
    static constexpr int Dimension = 16;

    // clang-format off
    #ifdef BEE_GPU_CODE
    T m[16];
    #else
    union
    {
        T m[16];
        struct { Vector4<T> c0, c1, c2, c3; };
    };
    #endif

    // ========== Constructors ==========

    constexpr Matrix4x4() noexcept;
    constexpr explicit Matrix4x4(T diag) noexcept;
    
    constexpr Matrix4x4(T m00, T m01, T m02, T m03, 
                        T m10, T m11, T m12, T m13,
                        T m20, T m21, T m22, T m23, 
                        T m30, T m31, T m32, T m33) noexcept;
    
    constexpr Matrix4x4(const Vector4<T>& c0, const Vector4<T>& c1, const Vector4<T>& c2, const Vector4<T>& c3) noexcept;
    constexpr explicit Matrix4x4(const Matrix3x3<T>& m) noexcept;

    template <ArithType U> constexpr explicit Matrix4x4(const Matrix4x4<U>& other) noexcept;

    // ========== Static Factories ==========

    constexpr static Matrix4x4 Identity() noexcept;
    constexpr static Matrix4x4 Zero() noexcept;

    // ========== Accessors ==========

    constexpr T& operator()(int col, int row)       noexcept;
    constexpr T  operator()(int col, int row) const noexcept;
    
    constexpr Vector4<T> col(int col) const noexcept;
    constexpr Vector4<T> row(int row) const noexcept;

    // ========== Operators ==========

    constexpr Matrix4x4  operator*(const Matrix4x4& other) const noexcept;
    constexpr Vector4<T> operator*(const Vector4<T>& v) const noexcept;
    constexpr Vector3<T> operator*(const Vector3<T>& v) const noexcept;
    constexpr Point3<T>  operator*(const Point3<T>& p) const noexcept;
    constexpr Matrix4x4  operator*(T scalar) const noexcept;
    
    constexpr Matrix4x4& operator*=(const Matrix4x4& other) noexcept;
    constexpr Matrix4x4& operator*=(T scalar) noexcept;
    
    constexpr Matrix4x4 operator+(const Matrix4x4& other) const noexcept;
    constexpr Matrix4x4 operator-(const Matrix4x4& other) const noexcept;
    
    constexpr bool operator==(const Matrix4x4& other) const noexcept;
    constexpr bool operator!=(const Matrix4x4& other) const noexcept;

    // ========== Matrix Operations ==========

    constexpr T determinant() const noexcept;
    constexpr Matrix4x4 transpose() const noexcept;
    constexpr Matrix4x4 adjugate() const noexcept;
    constexpr Matrix4x4 inverse() const noexcept;
    constexpr Matrix3x3<T> submatrix(int removeCol, int removeRow) const noexcept;
    constexpr Matrix3x3<T> submatrix3x3() const noexcept;
    constexpr T trace() const noexcept;

    // ========== Static Factories - Transforms ==========

    static constexpr Matrix4x4 Translate(const Vector3<T>& v) noexcept;
    static constexpr Matrix4x4 RotateX(T angle) noexcept;
    static constexpr Matrix4x4 RotateY(T angle) noexcept;
    static constexpr Matrix4x4 RotateZ(T angle) noexcept;
    static constexpr Matrix4x4 Rotate(T angle, const Vector3<T>& axis) noexcept;
    static constexpr Matrix4x4 Scale(T sx, T sy, T sz) noexcept;
    static constexpr Matrix4x4 Scale(const Vector3<T>& s) noexcept;
    static constexpr Matrix4x4 TRS(const Vector3<T>& translation, const Quaternion<T>& rotation, const Vector3<T>& scale) noexcept;
    static constexpr Matrix4x4 TRS(const Vector3<T>& translation, const Quaternion<T>& rotation, T uniformScale) noexcept;

    // ========== View and Projection ==========

    static constexpr Matrix4x4 LookAt(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up = Vector3<T>::UnitY()) noexcept;
    static Matrix4x4 Perspective(T fov, T aspect, T zNear, T zFar) noexcept;
    static Matrix4x4 Orthographic(T left, T right, T bottom, T top, T zNear, T zFar) noexcept;

    static Matrix4x4 PerspectiveLH_ZO(T fov, T aspect, T zNear, T zFar) noexcept;
    static Matrix4x4 PerspectiveLH_NO(T fov, T aspect, T zNear, T zFar) noexcept;
    static Matrix4x4 PerspectiveRH_ZO(T fov, T aspect, T zNear, T zFar) noexcept;
    static Matrix4x4 PerspectiveRH_NO(T fov, T aspect, T zNear, T zFar) noexcept;

    static constexpr Matrix4x4 OrthographicLH_ZO(T left, T right, T bottom, T top, T zNear, T zFar) noexcept;
    static constexpr Matrix4x4 OrthographicLH_NO(T left, T right, T bottom, T top, T zNear, T zFar) noexcept;
    static constexpr Matrix4x4 OrthographicRH_ZO(T left, T right, T bottom, T top, T zNear, T zFar) noexcept;
    static constexpr Matrix4x4 OrthographicRH_NO(T left, T right, T bottom, T top, T zNear, T zFar) noexcept;

    // ========== Utilities ==========

    constexpr bool hasNaN() const noexcept;
    constexpr bool isIdentity(T eps = T(1e-6)) const noexcept;
    // clang-format on
};

// ==================================================================================
// Standalone Functions Declarations
// ==================================================================================

template <ArithType T>
constexpr Matrix4x4<T> LookAtLH(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up = Vector3<T>::UnitY()) noexcept;

template <ArithType T>
constexpr Matrix4x4<T> LookAtRH(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up = Vector3<T>::UnitY()) noexcept;

template <ArithType T>
Matrix4x4<T> PerspectiveLH(T fov, T aspect, T zNear, T zFar) noexcept;

template <ArithType T>
Matrix4x4<T> PerspectiveRH(T fov, T aspect, T zNear, T zFar) noexcept;

template <ArithType T>
constexpr Matrix4x4<T> OrthographicLH(T left, T right, T bottom, T top, T zNear, T zFar) noexcept;

template <ArithType T>
constexpr Matrix4x4<T> OrthographicRH(T left, T right, T bottom, T top, T zNear, T zFar) noexcept;

// ==================================================================================
// Matrix2x2 Implementation
// ==================================================================================

#ifdef BEE_GPU_CODE
template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2() noexcept
    : m{T(1), T{}, T{}, T(1)}
{
}

template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2(T diag) noexcept
    : m{diag, T{}, T{}, diag}
{
}

template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2(T m00, T m01, T m10, T m11) noexcept
    : m{m00, m10, m01, m11}
{
}

template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2(const Vector2<T>& c0, const Vector2<T>& c1) noexcept
    : m{c0.x, c0.y, c1.x, c1.y}
{
}
#else
template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2() noexcept
    : m00(T(1)), m10(T{}), m01(T{}), m11(T(1))
{
}

template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2(T diag) noexcept
    : m00(diag), m10{}, m01{}, m11(diag)
{
}

template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2(T m00, T m01, T m10, T m11) noexcept
    : m00(m00), m10(m10), m01(m01), m11(m11)
{
}

template <ArithType T>
constexpr Matrix2x2<T>::Matrix2x2(const Vector2<T>& c0, const Vector2<T>& c1) noexcept
    : c0(c0), c1(c1)
{
}
#endif

// clang-format off
template <ArithType T>
template <ArithType U>
constexpr Matrix2x2<T>::Matrix2x2(const Matrix2x2<U>& other) noexcept
#ifdef BEE_GPU_CODE
    : m{ To<T>(other.m[0]), To<T>(other.m[1]), To<T>(other.m[2]), To<T>(other.m[3]) }
#else
    : m00(To<T>(other.m00)), m10(To<T>(other.m10)), m01(To<T>(other.m01)), m11(To<T>(other.m11))
#endif
{
}
// clang-format on

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::Identity() noexcept
{
    return Matrix2x2(1, 0, 0, 1);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::Zero() noexcept
{
    return Matrix2x2(0, 0, 0, 0);
}

template <ArithType T>
constexpr T& Matrix2x2<T>::operator()(int col, int row) noexcept
{
    BEE_DCHECK(col >= 0 && col < 2);
    BEE_DCHECK(row >= 0 && row < 2);
    return m[col * 2 + row];
}

template <ArithType T>
constexpr T Matrix2x2<T>::operator()(int col, int row) const noexcept
{
    BEE_DCHECK(col >= 0 && col < 2);
    BEE_DCHECK(row >= 0 && row < 2);
    return m[col * 2 + row];
}

template <ArithType T>
constexpr Vector2<T> Matrix2x2<T>::col(int col) const noexcept
{
    BEE_DCHECK(col >= 0 && col < 2);
    return Vector2<T>(m[col * 2], m[col * 2 + 1]);
}

template <ArithType T>
constexpr Vector2<T> Matrix2x2<T>::row(int row) const noexcept
{
    BEE_DCHECK(row >= 0 && row < 2);
    return Vector2<T>(m[row], m[row + 2]);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::operator*(const Matrix2x2& other) const noexcept
{
    Matrix2x2 result;

    T m00 = m[0], m10 = m[1], m01 = m[2], m11 = m[3];

    T o00 = other.m[0], o10 = other.m[1], o01 = other.m[2], o11 = other.m[3];

    return Matrix2x2(
            m00 * o00 + m01 * o10, // r00
            m00 * o01 + m01 * o11, // r01
            m10 * o00 + m11 * o10, // r10
            m10 * o01 + m11 * o11  // r11
            );
}

template <ArithType T>
constexpr Vector2<T> Matrix2x2<T>::operator*(const Vector2<T>& v) const noexcept
{
    return Vector2<T>(
            m[0] * v.x + m[2] * v.y,
            m[1] * v.x + m[3] * v.y
            );
}

template <ArithType T>
constexpr Normal2<T> Matrix2x2<T>::operator*(const Normal2<T>& n) const noexcept
{
    Matrix2x2 invT = inverse().transpose();
    Vector2<T> res = Normalize(invT * Vector2<T>(n.x, n.y));
    return Normal2<T>(res.x, res.y);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::operator*(T scalar) const noexcept
{
    return Matrix2x2(m[0] * scalar, m[2] * scalar, m[1] * scalar, m[3] * scalar);
}

template <ArithType T>
constexpr Matrix2x2<T>& Matrix2x2<T>::operator*=(const Matrix2x2& other) noexcept
{
    *this = *this * other;
    return *this;
}

template <ArithType T>
constexpr Matrix2x2<T>& Matrix2x2<T>::operator*=(T scalar) noexcept
{
    m[0] *= scalar;
    m[1] *= scalar;
    m[2] *= scalar;
    m[3] *= scalar;
    return *this;
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::operator+(const Matrix2x2& other) const noexcept
{
    return Matrix2x2(m[0] + other.m[0], m[2] + other.m[2], m[1] + other.m[1], m[3] + other.m[3]);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::operator-(const Matrix2x2& other) const noexcept
{
    return Matrix2x2(m[0] - other.m[0], m[2] - other.m[2], m[1] - other.m[1], m[3] - other.m[3]);
}

template <ArithType T>
constexpr bool Matrix2x2<T>::operator==(const Matrix2x2& other) const noexcept
{
    return m[0] == other.m[0] && m[1] == other.m[1] && m[2] == other.m[2] && m[3] == other.m[3];
}

template <ArithType T>
constexpr bool Matrix2x2<T>::operator!=(const Matrix2x2& other) const noexcept
{
    return !(*this == other);
}

template <ArithType T>
constexpr T Matrix2x2<T>::determinant() const noexcept
{
    return m[0] * m[3] - m[2] * m[1];
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::transpose() const noexcept
{
    return Matrix2x2(m[0], m[1], m[2], m[3]);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::adjugate() const noexcept
{
    return Matrix2x2(m[3], -m[2], -m[1], m[0]);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::inverse() const noexcept
{
    T det = m[0] * m[3] - m[2] * m[1];
    BEE_DCHECK(det != 0);
    T invDet = T(1) / det;
    return Matrix2x2(m[3] * invDet, -m[2] * invDet, -m[1] * invDet, m[0] * invDet);
}

template <ArithType T>
constexpr T Matrix2x2<T>::trace() const noexcept
{
    return m[0] + m[3];
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::Rotate(T angle) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    return Matrix2x2(c, -s, s, c);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::Scale(T sx, T sy) noexcept
{
    return Matrix2x2(sx, 0, 0, sy);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::Scale(const Vector2<T>& s) noexcept
{
    return Scale(s.x, s.y);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::ShearX(T sy) noexcept
{
    return Matrix2x2(1, sy, 0, 1);
}

template <ArithType T>
constexpr Matrix2x2<T> Matrix2x2<T>::ShearY(T sx) noexcept
{
    return Matrix2x2(1, 0, sx, 1);
}

template <ArithType T>
constexpr bool Matrix2x2<T>::hasNaN() const noexcept
{
    return IsNaN(m[0]) || IsNaN(m[1]) || IsNaN(m[2]) || IsNaN(m[3]);
}

template <ArithType T>
constexpr bool Matrix2x2<T>::isIdentity(T eps) const noexcept
{
    return Abs(m[0] - T(1)) < eps && Abs(m[3] - T(1)) < eps && Abs(m[2]) < eps && Abs(m[1]) < eps;
}

// ==================================================================================
// Matrix3x3 Implementation
// ==================================================================================

template <ArithType T>
constexpr Matrix3x3<T>::Matrix3x3() noexcept
    : m{T(1), T{}, T{}, T{}, T(1), T{}, T{}, T{}, T(1)}
{
}

template <ArithType T>
constexpr Matrix3x3<T>::Matrix3x3(T diag) noexcept
    : m{diag, 0, 0, 0, diag, 0, 0, 0, diag}
{
}

template <ArithType T>
constexpr Matrix3x3<T>::Matrix3x3(T m00, T m01, T m02,
                                  T m10, T m11, T m12,
                                  T m20, T m21, T m22) noexcept
    : m{m00, m10, m20, m01, m11, m21, m02, m12, m22}
{
}

// clang-format off
#ifdef BEE_GPU_CODE
template <ArithType T>
constexpr Matrix3x3<T>::Matrix3x3(const Vector3<T>& c0, const Vector3<T>& c1, const Vector3<T>& c2) noexcept
    : m{c0.x, c0.y, c0.z,
        c1.x, c1.y, c1.z,
        c2.x, c2.y, c2.z}
{
}
#else
template <ArithType T>
constexpr Matrix3x3<T>::Matrix3x3(const Vector3<T>& c0, const Vector3<T>& c1, const Vector3<T>& c2) noexcept
    : c0(c0), c1(c1), c2(c2)
{
}
#endif
// clang-format on

template <ArithType T>
template <ArithType U>
constexpr Matrix3x3<T>::Matrix3x3(const Matrix3x3<U>& other) noexcept
{
    m[0] = To<T>(other.m[0]);
    m[1] = To<T>(other.m[1]);
    m[2] = To<T>(other.m[2]);

    m[3] = To<T>(other.m[3]);
    m[4] = To<T>(other.m[4]);
    m[5] = To<T>(other.m[5]);

    m[6] = To<T>(other.m[6]);
    m[7] = To<T>(other.m[7]);
    m[8] = To<T>(other.m[8]);
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::Identity() noexcept
{
    return Matrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::Zero() noexcept
{
    return Matrix3x3(0, 0, 0, 0, 0, 0, 0, 0, 0);
}

template <ArithType T>
constexpr T& Matrix3x3<T>::operator()(int col, int row) noexcept
{
    BEE_DCHECK(col >= 0 && col < 3);
    BEE_DCHECK(row >= 0 && row < 3);
    return m[col * 3 + row];
}

template <ArithType T>
constexpr T Matrix3x3<T>::operator()(int col, int row) const noexcept
{
    BEE_DCHECK(col >= 0 && col < 3);
    BEE_DCHECK(row >= 0 && row < 3);
    return m[col * 3 + row];
}

template <ArithType T>
constexpr Vector3<T> Matrix3x3<T>::col(int col) const noexcept
{
    BEE_DCHECK(col >= 0 && col < 3);
    int offset = col * 3;
    return Vector3<T>(m[offset], m[offset + 1], m[offset + 2]);
}

template <ArithType T>
constexpr Vector3<T> Matrix3x3<T>::row(int row) const noexcept
{
    BEE_DCHECK(row >= 0 && row < 3);
    return Vector3<T>(m[row], m[row + 3], m[row + 6]);
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::operator*(const Matrix3x3& other) const noexcept
{
    return Matrix3x3(
            m[0] * other.m[0] + m[3] * other.m[1] + m[6] * other.m[2],
            m[1] * other.m[0] + m[4] * other.m[1] + m[7] * other.m[2],
            m[2] * other.m[0] + m[5] * other.m[1] + m[8] * other.m[2],

            m[0] * other.m[3] + m[3] * other.m[4] + m[6] * other.m[5],
            m[1] * other.m[3] + m[4] * other.m[4] + m[7] * other.m[5],
            m[2] * other.m[3] + m[5] * other.m[4] + m[8] * other.m[5],

            m[0] * other.m[6] + m[3] * other.m[7] + m[6] * other.m[8],
            m[1] * other.m[6] + m[4] * other.m[7] + m[7] * other.m[8],
            m[2] * other.m[6] + m[5] * other.m[7] + m[8] * other.m[8]
            );
}

template <ArithType T>
constexpr Vector3<T> Matrix3x3<T>::operator*(const Vector3<T>& v) const noexcept
{
    return Vector3<T>(
            m[0] * v.x + m[3] * v.y + m[6] * v.z,
            m[1] * v.x + m[4] * v.y + m[7] * v.z,
            m[2] * v.x + m[5] * v.y + m[8] * v.z
            );
}

template <ArithType T>
constexpr Normal3<T> Matrix3x3<T>::operator*(const Normal3<T>& n) const noexcept
{
    Matrix3x3 invT = inverse().transpose();
    Vector3<T> res = Normalize(invT * Vector3<T>(n.x, n.y, n.z));
    return Normal3<T>(res.x, res.y, res.z);
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::operator*(T scalar) const noexcept
{
    return Matrix3x3(
            m[0] * scalar, m[3] * scalar, m[6] * scalar,
            m[1] * scalar, m[4] * scalar, m[7] * scalar,
            m[2] * scalar, m[5] * scalar, m[8] * scalar
            );
}

template <ArithType T>
constexpr Matrix3x3<T>& Matrix3x3<T>::operator*=(const Matrix3x3& other) noexcept
{
    *this = *this * other;
    return *this;
}

template <ArithType T>
constexpr Matrix3x3<T>& Matrix3x3<T>::operator*=(T scalar) noexcept
{
    m[0] *= scalar;
    m[1] *= scalar;
    m[2] *= scalar;

    m[3] *= scalar;
    m[4] *= scalar;
    m[5] *= scalar;

    m[6] *= scalar;
    m[7] *= scalar;
    m[8] *= scalar;

    return *this;
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::operator+(const Matrix3x3& other) const noexcept
{
    return Matrix3x3(
            m[0] + other.m[0], m[3] + other.m[3], m[6] + other.m[6],
            m[1] + other.m[1], m[4] + other.m[4], m[7] + other.m[7],
            m[2] + other.m[2], m[5] + other.m[5], m[8] + other.m[8]
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::operator-(const Matrix3x3& other) const noexcept
{
    return Matrix3x3(
            m[0] - other.m[0], m[3] - other.m[3], m[6] - other.m[6],
            m[1] - other.m[1], m[4] - other.m[4], m[7] - other.m[7],
            m[2] - other.m[2], m[5] - other.m[5], m[8] - other.m[8]
            );
}

template <ArithType T>
constexpr bool Matrix3x3<T>::operator==(const Matrix3x3& other) const noexcept
{
    for (int i = 0; i < 9; ++i)
        if (m[i] != other.m[i])
            return false;
    return true;
}

template <ArithType T>
constexpr bool Matrix3x3<T>::operator!=(const Matrix3x3& other) const noexcept
{
    return !(*this == other);
}

template <ArithType T>
constexpr T Matrix3x3<T>::determinant() const noexcept
{
    return m[0] * (m[4] * m[8] - m[5] * m[7]) - m[3] * (m[1] * m[8] - m[2] * m[7]) + m[6] * (m[1] * m[5] - m[2] * m[4]);
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::transpose() const noexcept
{
    return Matrix3x3(
            m[0], m[1], m[2],
            m[3], m[4], m[5],
            m[6], m[7], m[8]
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::adjugate() const noexcept
{
    return Matrix3x3(
            (m[4] * m[8] - m[5] * m[7]), -(m[3] * m[8] - m[5] * m[6]), (m[3] * m[7] - m[4] * m[6]),
            -(m[1] * m[8] - m[2] * m[7]), (m[0] * m[8] - m[2] * m[6]), -(m[0] * m[7] - m[1] * m[6]),
            (m[1] * m[5] - m[2] * m[4]), -(m[0] * m[5] - m[2] * m[3]), (m[0] * m[4] - m[1] * m[3])
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::cofactor() const noexcept
{
    return Matrix3x3(
            (m[4] * m[8] - m[5] * m[7]), -(m[1] * m[8] - m[2] * m[7]), (m[1] * m[5] - m[2] * m[4]),
            -(m[3] * m[8] - m[5] * m[6]), (m[0] * m[8] - m[2] * m[6]), -(m[0] * m[5] - m[2] * m[3]),
            (m[3] * m[7] - m[4] * m[6]), -(m[0] * m[7] - m[1] * m[6]), (m[0] * m[4] - m[1] * m[3])
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::inverse() const noexcept
{
    T t00 = m[4] * m[8] - m[5] * m[7];
    T t10 = m[1] * m[8] - m[2] * m[7];
    T t20 = m[1] * m[5] - m[2] * m[4];

    T det = m[0] * t00 - m[3] * t10 + m[6] * t20;
    BEE_DCHECK(det != 0);
    T invDet = T(1) / det;

    return Matrix3x3(
            t00 * invDet, -(m[3] * m[8] - m[5] * m[6]) * invDet, (m[3] * m[7] - m[4] * m[6]) * invDet,
            -t10 * invDet, (m[0] * m[8] - m[2] * m[6]) * invDet, -(m[0] * m[7] - m[1] * m[6]) * invDet,
            t20 * invDet, -(m[0] * m[5] - m[2] * m[3]) * invDet, (m[0] * m[4] - m[1] * m[3]) * invDet
            );
}

template <ArithType T>
constexpr T Matrix3x3<T>::trace() const noexcept
{
    return m[0] + m[4] + m[8];
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::RotateX(T angle) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    return Matrix3x3(
            1, 0, 0,
            0, c, s,
            0, -s, c
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::RotateY(T angle) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    return Matrix3x3(
            c, 0, -s,
            0, 1, 0,
            s, 0, c
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::RotateZ(T angle) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    return Matrix3x3(
            c, s, 0,
            -s, c, 0,
            0, 0, 1
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::Rotate(T angle, const Vector3<T>& axis) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    T t = T(1) - c;

    T x = axis.x;
    T y = axis.y;
    T z = axis.z;

    return Matrix3x3(
            t * x * x + c, t * x * y + s * z, t * x * z - s * y,
            t * x * y - s * z, t * y * y + c, t * y * z + s * x,
            t * x * z + s * y, t * y * z - s * x, t * z * z + c
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::Scale(T sx, T sy, T sz) noexcept
{
    return Matrix3x3(
            sx, 0, 0,
            0, sy, 0,
            0, 0, sz
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix3x3<T>::Scale(const Vector3<T>& s) noexcept
{
    return Scale(s.x, s.y, s.z);
}

template <ArithType T>
constexpr bool Matrix3x3<T>::hasNaN() const noexcept
{
    for (int i = 0; i < 9; ++i)
        if (IsNaN(m[i]))
            return true;
    return false;
}

template <ArithType T>
constexpr bool Matrix3x3<T>::isIdentity(T eps) const noexcept
{
    return Abs(m[0] - T(1)) < eps && Abs(m[4] - T(1)) < eps && Abs(m[8] - T(1)) < eps &&
           Abs(m[1]) < eps && Abs(m[2]) < eps && Abs(m[3]) < eps && Abs(m[5]) < eps && Abs(m[6]) < eps && Abs(m[7]) < eps;
}

template <ArithType T>
constexpr bool Matrix3x3<T>::isOrthogonal(T eps) const noexcept
{
    auto shouldBeIdentity = (*this) * this->transpose();
    return shouldBeIdentity.isIdentity(eps);
}

// ==================================================================================
// Matrix4x4 Implementation
// ==================================================================================

template <ArithType T>
constexpr Matrix4x4<T>::Matrix4x4() noexcept
    : m{T(1), T{}, T{}, T{}, T{}, T(1), T{}, T{}, T{}, T{}, T(1), T{}, T{}, T{}, T{}, T(1)}
{
}

// clang-format off
template <ArithType T>
constexpr Matrix4x4<T>::Matrix4x4(T diag) noexcept
    : m{diag, 0, 0, 0,
        0, diag, 0, 0,
        0, 0, diag, 0,
        0, 0, 0, diag}
{
}

template <ArithType T>
constexpr Matrix4x4<T>::Matrix4x4(
        T m00, T m01, T m02, T m03,
        T m10, T m11, T m12, T m13,
        T m20, T m21, T m22, T m23,
        T m30, T m31, T m32, T m33
        ) noexcept
    : m{m00, m10, m20, m30,
        m01, m11, m21, m31,
        m02, m12, m22, m32,
        m03, m13, m23, m33}
{
}

#ifdef BEE_GPU_CODE
template <ArithType T>
constexpr Matrix4x4<T>::Matrix4x4(const Vector4<T>& c0, const Vector4<T>& c1,
                                  const Vector4<T>& c2, const Vector4<T>& c3) noexcept
    : m{c0.x, c0.y, c0.z, c0.w,
        c1.x, c1.y, c1.z, c1.w,
        c2.x, c2.y, c2.z, c2.w,
        c3.x, c3.y, c3.z, c3.w}
{
}
#else
template <ArithType T>
constexpr Matrix4x4<T>::Matrix4x4(const Vector4<T>& c0, const Vector4<T>& c1,
                                  const Vector4<T>& c2, const Vector4<T>& c3) noexcept
    : c0(c0), c1(c1), c2(c2), c3(c3)
{
}
#endif

template <ArithType T>
constexpr Matrix4x4<T>::Matrix4x4(const Matrix3x3<T>& m) noexcept
    : m{m.m[0], m.m[1], m.m[2], 0,
        m.m[3], m.m[4], m.m[5], 0,
        m.m[6], m.m[7], m.m[8], 0,
        0, 0, 0, 1}
{
}
// clang-format on

template <ArithType T>
template <ArithType U>
constexpr Matrix4x4<T>::Matrix4x4(const Matrix4x4<U>& other) noexcept
{
    m[0] = To<T>(other.m[0]);
    m[1] = To<T>(other.m[1]);
    m[2] = To<T>(other.m[2]);
    m[3] = To<T>(other.m[3]);

    m[4] = To<T>(other.m[4]);
    m[5] = To<T>(other.m[5]);
    m[6] = To<T>(other.m[6]);
    m[7] = To<T>(other.m[7]);

    m[8]  = To<T>(other.m[8]);
    m[9]  = To<T>(other.m[9]);
    m[10] = To<T>(other.m[10]);
    m[11] = To<T>(other.m[11]);

    m[12] = To<T>(other.m[12]);
    m[13] = To<T>(other.m[13]);
    m[14] = To<T>(other.m[14]);
    m[15] = To<T>(other.m[15]);
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::Identity() noexcept
{
    return Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::Zero() noexcept
{
    return Matrix4x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

template <ArithType T>
constexpr T& Matrix4x4<T>::operator()(int col, int row) noexcept
{
    BEE_DCHECK(col >= 0 && col < 4);
    BEE_DCHECK(row >= 0 && row < 4);
    return m[col * 4 + row];
}

template <ArithType T>
constexpr T Matrix4x4<T>::operator()(int col, int row) const noexcept
{
    BEE_DCHECK(col >= 0 && col < 4);
    BEE_DCHECK(row >= 0 && row < 4);
    return m[col * 4 + row];
}

template <ArithType T>
constexpr Vector4<T> Matrix4x4<T>::col(int col) const noexcept
{
    BEE_DCHECK(col >= 0 && col < 4);
    int offset = col * 4;
    return Vector4<T>(m[offset], m[offset + 1], m[offset + 2], m[offset + 3]);
}

template <ArithType T>
constexpr Vector4<T> Matrix4x4<T>::row(int row) const noexcept
{
    BEE_DCHECK(row >= 0 && row < 4);
    return Vector4<T>(m[row + 0], m[row + 4], m[row + 8], m[row + 12]);
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::operator*(const Matrix4x4& other) const noexcept
{
    return Matrix4x4(
            m[0] * other.m[0] + m[4] * other.m[1] + m[8] * other.m[2] + m[12] * other.m[3],
            m[0] * other.m[4] + m[4] * other.m[5] + m[8] * other.m[6] + m[12] * other.m[7],
            m[0] * other.m[8] + m[4] * other.m[9] + m[8] * other.m[10] + m[12] * other.m[11],
            m[0] * other.m[12] + m[4] * other.m[13] + m[8] * other.m[14] + m[12] * other.m[15],

            m[1] * other.m[0] + m[5] * other.m[1] + m[9] * other.m[2] + m[13] * other.m[3],
            m[1] * other.m[4] + m[5] * other.m[5] + m[9] * other.m[6] + m[13] * other.m[7],
            m[1] * other.m[8] + m[5] * other.m[9] + m[9] * other.m[10] + m[13] * other.m[11],
            m[1] * other.m[12] + m[5] * other.m[13] + m[9] * other.m[14] + m[13] * other.m[15],

            m[2] * other.m[0] + m[6] * other.m[1] + m[10] * other.m[2] + m[14] * other.m[3],
            m[2] * other.m[4] + m[6] * other.m[5] + m[10] * other.m[6] + m[14] * other.m[7],
            m[2] * other.m[8] + m[6] * other.m[9] + m[10] * other.m[10] + m[14] * other.m[11],
            m[2] * other.m[12] + m[6] * other.m[13] + m[10] * other.m[14] + m[14] * other.m[15],

            m[3] * other.m[0] + m[7] * other.m[1] + m[11] * other.m[2] + m[15] * other.m[3],
            m[3] * other.m[4] + m[7] * other.m[5] + m[11] * other.m[6] + m[15] * other.m[7],
            m[3] * other.m[8] + m[7] * other.m[9] + m[11] * other.m[10] + m[15] * other.m[11],
            m[3] * other.m[12] + m[7] * other.m[13] + m[11] * other.m[14] + m[15] * other.m[15]
            );
}

template <ArithType T>
constexpr Vector4<T> Matrix4x4<T>::operator*(const Vector4<T>& v) const noexcept
{
    return Vector4<T>(
            m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
            m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
            m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
            m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w
            );
}

template <ArithType T>
constexpr Vector3<T> Matrix4x4<T>::operator*(const Vector3<T>& v) const noexcept
{
    return Vector3<T>(
            m[0] * v.x + m[4] * v.y + m[8] * v.z,
            m[1] * v.x + m[5] * v.y + m[9] * v.z,
            m[2] * v.x + m[6] * v.y + m[10] * v.z
            );
}

template <ArithType T>
constexpr Point3<T> Matrix4x4<T>::operator*(const Point3<T>& p) const noexcept
{
    T w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
    BEE_DCHECK(Abs(w) > T(1e-8));
    if (Abs(w) < T(1e-8))
    {
        return Point3<T>{};
    }
    T invW = T(1) / w;
    return Point3<T>(
            (m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12]) * invW,
            (m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13]) * invW,
            (m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]) * invW
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::operator*(T scalar) const noexcept
{
    return Matrix4x4(
            m[0] * scalar, m[4] * scalar, m[8] * scalar, m[12] * scalar,
            m[1] * scalar, m[5] * scalar, m[9] * scalar, m[13] * scalar,
            m[2] * scalar, m[6] * scalar, m[10] * scalar, m[14] * scalar,
            m[3] * scalar, m[7] * scalar, m[11] * scalar, m[15] * scalar
            );
}

template <ArithType T>
constexpr Matrix4x4<T>& Matrix4x4<T>::operator*=(const Matrix4x4& other) noexcept
{
    *this = *this * other;
    return *this;
}

template <ArithType T>
constexpr Matrix4x4<T>& Matrix4x4<T>::operator*=(T scalar) noexcept
{
    for (int i = 0; i < 16; ++i)
        m[i] *= scalar;
    return *this;
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::operator+(const Matrix4x4& other) const noexcept
{
    return Matrix4x4(
            m[0] + other.m[0], m[4] + other.m[4], m[8] + other.m[8], m[12] + other.m[12],
            m[1] + other.m[1], m[5] + other.m[5], m[9] + other.m[9], m[13] + other.m[13],
            m[2] + other.m[2], m[6] + other.m[6], m[10] + other.m[10], m[14] + other.m[14],
            m[3] + other.m[3], m[7] + other.m[7], m[11] + other.m[11], m[15] + other.m[15]
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::operator-(const Matrix4x4& other) const noexcept
{
    return Matrix4x4(
            m[0] - other.m[0], m[4] - other.m[4], m[8] - other.m[8], m[12] - other.m[12],
            m[1] - other.m[1], m[5] - other.m[5], m[9] - other.m[9], m[13] - other.m[13],
            m[2] - other.m[2], m[6] - other.m[6], m[10] - other.m[10], m[14] - other.m[14],
            m[3] - other.m[3], m[7] - other.m[7], m[11] - other.m[11], m[15] - other.m[15]
            );
}

template <ArithType T>
constexpr bool Matrix4x4<T>::operator==(const Matrix4x4& other) const noexcept
{
    for (int i = 0; i < 16; ++i)
        if (m[i] != other.m[i])
            return false;
    return true;
}

template <ArithType T>
constexpr bool Matrix4x4<T>::operator!=(const Matrix4x4& other) const noexcept
{
    return !(*this == other);
}

template <ArithType T>
constexpr T Matrix4x4<T>::determinant() const noexcept
{
    T s0 = m[0] * m[5] - m[1] * m[4];
    T s1 = m[0] * m[6] - m[2] * m[4];
    T s2 = m[0] * m[7] - m[3] * m[4];
    T s3 = m[1] * m[6] - m[2] * m[5];
    T s4 = m[1] * m[7] - m[3] * m[5];
    T s5 = m[2] * m[7] - m[3] * m[6];

    T c5 = m[10] * m[15] - m[11] * m[14];
    T c4 = m[9] * m[15] - m[11] * m[13];
    T c3 = m[9] * m[14] - m[10] * m[13];
    T c2 = m[8] * m[15] - m[11] * m[12];
    T c1 = m[8] * m[14] - m[10] * m[12];
    T c0 = m[8] * m[13] - m[9] * m[12];

    return (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::transpose() const noexcept
{
    return Matrix4x4(
            m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11],
            m[12], m[13], m[14], m[15]
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::adjugate() const noexcept
{
    const T m00 = m[0], m01 = m[4], m02 = m[8], m03  = m[12];
    const T m10 = m[1], m11 = m[5], m12 = m[9], m13  = m[13];
    const T m20 = m[2], m21 = m[6], m22 = m[10], m23 = m[14];
    const T m30 = m[3], m31 = m[7], m32 = m[11], m33 = m[15];

    const T v0 = m20 * m31 - m21 * m30;
    const T v1 = m20 * m32 - m22 * m30;
    const T v2 = m20 * m33 - m23 * m30;
    const T v3 = m21 * m32 - m22 * m31;
    const T v4 = m21 * m33 - m23 * m31;
    const T v5 = m22 * m33 - m23 * m32;

    const T t00 = +(v5 * m11 - v4 * m12 + v3 * m13);
    const T t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
    const T t20 = +(v4 * m10 - v2 * m11 + v0 * m13);
    const T t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

    const T t01 = -(v5 * m01 - v4 * m02 + v3 * m03);
    const T t11 = +(v5 * m00 - v2 * m02 + v1 * m03);
    const T t21 = -(v4 * m00 - v2 * m01 + v0 * m03);
    const T t31 = +(v3 * m00 - v1 * m01 + v0 * m02);

    const T u0 = m00 * m11 - m01 * m10;
    const T u1 = m00 * m12 - m02 * m10;
    const T u2 = m00 * m13 - m03 * m10;
    const T u3 = m01 * m12 - m02 * m11;
    const T u4 = m01 * m13 - m03 * m11;
    const T u5 = m02 * m13 - m03 * m12;

    const T t02 = +(u5 * m31 - u4 * m32 + u3 * m33);
    const T t12 = -(u5 * m30 - u2 * m32 + u1 * m33);
    const T t22 = +(u4 * m30 - u2 * m31 + u0 * m33);
    const T t32 = -(u3 * m30 - u1 * m31 + u0 * m32);

    const T t03 = -(u5 * m21 - u4 * m22 + u3 * m23);
    const T t13 = +(u5 * m20 - u2 * m22 + u1 * m23);
    const T t23 = -(u4 * m20 - u2 * m21 + u0 * m23);
    const T t33 = +(u3 * m20 - u1 * m21 + u0 * m22);

    return Matrix4x4(
            t00, t01, t02, t03,
            t10, t11, t12, t13,
            t20, t21, t22, t23,
            t30, t31, t32, t33);
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::inverse() const noexcept
{
    const T m00 = m[0], m01 = m[4], m02 = m[8], m03  = m[12];
    const T m10 = m[1], m11 = m[5], m12 = m[9], m13  = m[13];
    const T m20 = m[2], m21 = m[6], m22 = m[10], m23 = m[14];
    const T m30 = m[3], m31 = m[7], m32 = m[11], m33 = m[15];

    const T v0 = m20 * m31 - m21 * m30;
    const T v1 = m20 * m32 - m22 * m30;
    const T v2 = m20 * m33 - m23 * m30;
    const T v3 = m21 * m32 - m22 * m31;
    const T v4 = m21 * m33 - m23 * m31;
    const T v5 = m22 * m33 - m23 * m32;

    const T t00 = +(v5 * m11 - v4 * m12 + v3 * m13);
    const T t10 = -(v5 * m10 - v2 * m12 + v1 * m13);
    const T t20 = +(v4 * m10 - v2 * m11 + v0 * m13);
    const T t30 = -(v3 * m10 - v1 * m11 + v0 * m12);

    const T det = (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);
    BEE_DCHECK(det != T(0));
    const T invDet = T(1) / det;

    const T d00 = t00 * invDet;
    const T d10 = t10 * invDet;
    const T d20 = t20 * invDet;
    const T d30 = t30 * invDet;

    const T d01 = -(v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    const T d11 = +(v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    const T d21 = -(v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    const T d31 = +(v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    const T u0 = m00 * m11 - m01 * m10;
    const T u1 = m00 * m12 - m02 * m10;
    const T u2 = m00 * m13 - m03 * m10;
    const T u3 = m01 * m12 - m02 * m11;
    const T u4 = m01 * m13 - m03 * m11;
    const T u5 = m02 * m13 - m03 * m12;

    const T d02 = +(u5 * m31 - u4 * m32 + u3 * m33) * invDet;
    const T d12 = -(u5 * m30 - u2 * m32 + u1 * m33) * invDet;
    const T d22 = +(u4 * m30 - u2 * m31 + u0 * m33) * invDet;
    const T d32 = -(u3 * m30 - u1 * m31 + u0 * m32) * invDet;

    const T d03 = -(u5 * m21 - u4 * m22 + u3 * m23) * invDet;
    const T d13 = +(u5 * m20 - u2 * m22 + u1 * m23) * invDet;
    const T d23 = -(u4 * m20 - u2 * m21 + u0 * m23) * invDet;
    const T d33 = +(u3 * m20 - u1 * m21 + u0 * m22) * invDet;

    return Matrix4x4(
            d00, d01, d02, d03,
            d10, d11, d12, d13,
            d20, d21, d22, d23,
            d30, d31, d32, d33);
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix4x4<T>::submatrix(int removeCol, int removeRow) const noexcept
{
    BEE_DCHECK(removeCol >= 0 && removeCol < 4);
    BEE_DCHECK(removeRow >= 0 && removeRow < 4);

    T sub[9];
    int subIdx = 0;

    for (int col = 0; col < 4; ++col)
    {
        if (col == removeCol)
            continue;
        for (int row = 0; row < 4; ++row)
        {
            if (row == removeRow)
                continue;
            sub[subIdx++] = m[col * 4 + row];
        }
    }

    return Matrix3x3<T>(
            sub[0], sub[3], sub[6],
            sub[1], sub[4], sub[7],
            sub[2], sub[5], sub[8]
            );
}

template <ArithType T>
constexpr Matrix3x3<T> Matrix4x4<T>::submatrix3x3() const noexcept
{
    return Matrix3x3<T>(
            m[0], m[4], m[8],
            m[1], m[5], m[9],
            m[2], m[6], m[10]
            );
}

template <ArithType T>
constexpr T Matrix4x4<T>::trace() const noexcept
{
    return m[0] + m[5] + m[10] + m[15];
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::Translate(const Vector3<T>& v) noexcept
{
    return Matrix4x4(
            1, 0, 0, v.x,
            0, 1, 0, v.y,
            0, 0, 1, v.z,
            0, 0, 0, 1
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::RotateX(T angle) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    return Matrix4x4(
            1, 0, 0, 0,
            0, c, s, 0,
            0, -s, c, 0,
            0, 0, 0, 1
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::RotateY(T angle) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    return Matrix4x4(
            c, 0, -s, 0,
            0, 1, 0, 0,
            s, 0, c, 0,
            0, 0, 0, 1
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::RotateZ(T angle) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    return Matrix4x4(
            c, s, 0, 0,
            -s, c, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::Rotate(T angle, const Vector3<T>& axis) noexcept
{
    T c = Cos(angle);
    T s = Sin(angle);
    T t = T(1) - c;

    T x = axis.x;
    T y = axis.y;
    T z = axis.z;

    return Matrix4x4(
            t * x * x + c, t * x * y + s * z, t * x * z - s * y, 0,
            t * x * y - s * z, t * y * y + c, t * y * z + s * x, 0,
            t * x * z + s * y, t * y * z - s * x, t * z * z + c, 0,
            0, 0, 0, 1
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::Scale(T sx, T sy, T sz) noexcept
{
    return Matrix4x4(
            sx, 0, 0, 0,
            0, sy, 0, 0,
            0, 0, sz, 0,
            0, 0, 0, 1
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::Scale(const Vector3<T>& s) noexcept
{
    return Scale(s.x, s.y, s.z);
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::TRS(const Vector3<T>& translation, const Quaternion<T>& rotation, const Vector3<T>& scale) noexcept
{
    Matrix4x4 m = rotation.toMatrix4x4();
    m.c0        *= scale.x;
    m.c1        *= scale.y;
    m.c2        *= scale.z;
    m.c3        = Vector4<T>{translation, T(1)};
    return m;
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::TRS(const Vector3<T>& translation, const Quaternion<T>& rotation, T uniformScale) noexcept
{
    return TRS(translation, rotation, Vector3<T>(uniformScale, uniformScale, uniformScale));
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::LookAt(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up) noexcept
{
    #if BEE_COORD_SYSTEM_LH
    return LookAtLH(eye, target, up);
    #else
    return LookAtRH(eye, target, up);
    #endif
}

template <ArithType T>
Matrix4x4<T> Matrix4x4<T>::Perspective(T fov, T aspect, T zNear, T zFar) noexcept
{
    #if BEE_COORD_SYSTEM_LH
    return PerspectiveLH(fov, aspect, zNear, zFar);
    #else
    return PerspectiveRH(fov, aspect, zNear, zFar);
    #endif
}

template <ArithType T>
Matrix4x4<T> Matrix4x4<T>::Orthographic(T left, T right, T bottom, T top, T zNear, T zFar) noexcept
{
    #if BEE_COORD_SYSTEM_LH
    return OrthographicLH(left, right, bottom, top, zNear, zFar);
    #else
    return OrthographicRH(left, right, bottom, top, zNear, zFar);
    #endif
}

template <ArithType T>
Matrix4x4<T> Matrix4x4<T>::PerspectiveLH_ZO(T fov, T aspect, T zNear, T zFar) noexcept
{
    const T one          = To<T>(1);
    const T two          = To<T>(2);
    const T tanHalfFov   = Tan(fov / two);
    const T invAspectTan = one / (aspect * tanHalfFov);
    const T invTan       = one / tanHalfFov;
    const T farDiff      = zFar - zNear;

    const T depthParam  = zFar / farDiff;
    const T depthParam2 = -zNear * zFar / farDiff;

    return Matrix4x4<T>(
            invAspectTan, T{}, T{}, T{},
            T{}, invTan, T{}, T{},
            T{}, T{}, depthParam, one,
            T{}, T{}, depthParam2, T{}
            );
}

template <ArithType T>
Matrix4x4<T> Matrix4x4<T>::PerspectiveLH_NO(T fov, T aspect, T zNear, T zFar) noexcept
{
    const T one          = To<T>(1);
    const T two          = To<T>(2);
    const T tanHalfFov   = Tan(fov / two);
    const T invAspectTan = one / (aspect * tanHalfFov);
    const T invTan       = one / tanHalfFov;
    const T farDiff      = zFar - zNear;

    const T depthParam  = (zFar + zNear) / farDiff;
    const T depthParam2 = -two * zNear * zFar / farDiff;

    return Matrix4x4<T>(
            invAspectTan, T{}, T{}, T{},
            T{}, invTan, T{}, T{},
            T{}, T{}, depthParam, one,
            T{}, T{}, depthParam2, T{}
            );
}

template <ArithType T>
Matrix4x4<T> Matrix4x4<T>::PerspectiveRH_ZO(T fov, T aspect, T zNear, T zFar) noexcept
{
    const T one          = To<T>(1);
    const T two          = To<T>(2);
    const T tanHalfFov   = Tan(fov / two);
    const T invAspectTan = one / (aspect * tanHalfFov);
    const T invTan       = one / tanHalfFov;
    const T farDiff      = zFar - zNear;
    BEE_DCHECK(farDiff > T{});

    const T depthParam  = zFar / farDiff;
    const T depthParam2 = -zNear * zFar / farDiff;

    return Matrix4x4<T>(
            invAspectTan, T{}, T{}, T{},
            T{}, invTan, T{}, T{},
            T{}, T{}, depthParam, -one,
            T{}, T{}, depthParam2, T{}
            );
}

template <ArithType T>
Matrix4x4<T> Matrix4x4<T>::PerspectiveRH_NO(T fov, T aspect, T zNear, T zFar) noexcept
{
    const T one          = To<T>(1);
    const T two          = To<T>(2);
    const T tanHalfFov   = Tan(fov / two);
    const T invAspectTan = one / (aspect * tanHalfFov);
    const T invTan       = one / tanHalfFov;
    const T farDiff      = zFar - zNear;
    BEE_DCHECK(farDiff > T{});

    const T depthParam  = -(zFar + zNear) / farDiff;
    const T depthParam2 = -(two * zFar * zNear) / farDiff;

    return Matrix4x4<T>(
            invAspectTan, T{}, T{}, T{},
            T{}, invTan, T{}, T{},
            T{}, T{}, depthParam, -one,
            T{}, T{}, depthParam2, T{}
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::OrthographicLH_ZO(
        T left, T right, T bottom, T top, T zNear, T zFar) noexcept
{
    const T one   = To<T>(1);
    const T two   = To<T>(2);
    const T invRL = one / (right - left);
    const T invTB = one / (top - bottom);
    const T invFN = one / (zFar - zNear);

    const T twoOverRL = two * invRL;
    const T twoOverTB = two * invTB;
    const T tx        = -(right + left) * invRL;
    const T ty        = -(top + bottom) * invTB;
    const T tz        = -zNear * invFN;

    return Matrix4x4<T>(
            twoOverRL, T{}, T{}, T{},
            T{}, twoOverTB, T{}, T{},
            T{}, T{}, invFN, tz,
            tx, ty, T{}, one
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::OrthographicLH_NO(
        T left, T right, T bottom, T top, T zNear, T zFar) noexcept
{
    const T one   = To<T>(1);
    const T two   = To<T>(2);
    const T invRL = one / (right - left);
    const T invTB = one / (top - bottom);
    const T invFN = one / (zFar - zNear);

    const T twoOverRL = two * invRL;
    const T twoOverTB = two * invTB;
    const T twoOverFN = two * invFN;
    const T tx        = -(right + left) * invRL;
    const T ty        = -(top + bottom) * invTB;
    const T tz        = -(zFar + zNear) * invFN;

    return Matrix4x4<T>(
            twoOverRL, T{}, T{}, T{},
            T{}, twoOverTB, T{}, T{},
            T{}, T{}, twoOverFN, tz,
            tx, ty, T{}, one
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::OrthographicRH_ZO(
        T left, T right, T bottom, T top, T zNear, T zFar) noexcept
{
    const T one   = To<T>(1);
    const T two   = To<T>(2);
    const T invRL = one / (right - left);
    const T invTB = one / (top - bottom);
    const T invFN = one / (zFar - zNear);

    const T twoOverRL = two * invRL;
    const T twoOverTB = two * invTB;
    const T tx        = -(right + left) * invRL;
    const T ty        = -(top + bottom) * invTB;
    const T tz        = -zNear * invFN;

    return Matrix4x4<T>(
            twoOverRL, T{}, T{}, T{},
            T{}, twoOverTB, T{}, T{},
            T{}, T{}, invFN, tz,
            tx, ty, T{}, one
            );
}

template <ArithType T>
constexpr Matrix4x4<T> Matrix4x4<T>::OrthographicRH_NO(
        T left, T right, T bottom, T top, T zNear, T zFar) noexcept
{
    const T one   = To<T>(1);
    const T two   = To<T>(2);
    const T invRL = one / (right - left);
    const T invTB = one / (top - bottom);
    const T invFN = one / (zFar - zNear);

    const T twoOverRL = two * invRL;
    const T twoOverTB = two * invTB;
    const T twoOverFN = two * invFN;
    const T tx        = -(right + left) * invRL;
    const T ty        = -(top + bottom) * invTB;
    const T tz        = -(zFar + zNear) * invFN;

    return Matrix4x4<T>(
            twoOverRL, T{}, T{}, T{},
            T{}, twoOverTB, T{}, T{},
            T{}, T{}, twoOverFN, tz,
            tx, ty, T{}, one
            );
}

template <ArithType T>
constexpr bool Matrix4x4<T>::hasNaN() const noexcept
{
    for (int i = 0; i < 16; ++i)
        if (IsNaN(m[i]))
            return true;
    return false;
}

template <ArithType T>
constexpr bool Matrix4x4<T>::isIdentity(T eps) const noexcept
{
    return Abs(m[0] - T(1)) < eps && Abs(m[5] - T(1)) < eps &&
           Abs(m[10] - T(1)) < eps && Abs(m[15] - T(1)) < eps &&
           Abs(m[1]) < eps && Abs(m[2]) < eps && Abs(m[3]) < eps &&
           Abs(m[4]) < eps && Abs(m[6]) < eps && Abs(m[7]) < eps &&
           Abs(m[8]) < eps && Abs(m[9]) < eps && Abs(m[11]) < eps &&
           Abs(m[12]) < eps && Abs(m[13]) < eps && Abs(m[14]) < eps;
}

// ==================================================================================
// Standalone Functions Implementation
// ==================================================================================

template <ArithType T>
constexpr Matrix4x4<T> LookAtLH(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up) noexcept
{
    Vector3<T> forward    = Normalize(target - eye);
    Vector3<T> right      = Normalize(Cross(up, forward));
    Vector3<T> computedUp = Cross(forward, right);

    return Matrix4x4<T>(
            right.x, right.y, right.z, T{},
            computedUp.x, computedUp.y, computedUp.z, T{},
            forward.x, forward.y, forward.z, T{},
            -Dot(right, eye), -Dot(computedUp, eye), -Dot(forward, eye), T(1)
            );
}

template <ArithType T>
constexpr Matrix4x4<T> LookAtRH(const Vector3<T>& eye, const Vector3<T>& target, const Vector3<T>& up) noexcept
{
    Vector3<T> forward    = Normalize(target - eye);
    Vector3<T> right      = Normalize(Cross(forward, up));
    Vector3<T> computedUp = Cross(right, forward);

    return Matrix4x4<T>(
            right.x, computedUp.x, forward.x, -Dot(right, eye),
            right.y, computedUp.y, forward.y, -Dot(computedUp, eye),
            right.z, computedUp.z, forward.z, -Dot(forward, eye),
            T{}, T{}, T{}, T(1)
            );
}

template <ArithType T>
Matrix4x4<T> PerspectiveLH(T fov, T aspect, T zNear, T zFar) noexcept
{
    #if BEE_NDC_RANGE_ZO
    return Matrix4x4<T>::PerspectiveLH_ZO(fov, aspect, zNear, zFar);
    #else
    return Matrix4x4<T>::PerspectiveLH_NO(fov, aspect, zNear, zFar);
    #endif
}

template <ArithType T>
Matrix4x4<T> PerspectiveRH(T fov, T aspect, T zNear, T zFar) noexcept
{
    #if BEE_NDC_RANGE_ZO
    return Matrix4x4<T>::PerspectiveRH_ZO(fov, aspect, zNear, zFar);
    #else
    return Matrix4x4<T>::PerspectiveRH_NO(fov, aspect, zNear, zFar);
    #endif
}

template <ArithType T>
constexpr Matrix4x4<T> OrthographicLH(T left, T right, T bottom, T top, T zNear, T zFar) noexcept
{
    #if BEE_NDC_RANGE_ZO
    return Matrix4x4<T>::OrthographicLH_ZO(left, right, bottom, top, zNear, zFar);
    #else
    return Matrix4x4<T>::OrthographicLH_NO(left, right, bottom, top, zNear, zFar);
    #endif
}

template <ArithType T>
constexpr Matrix4x4<T> OrthographicRH(T left, T right, T bottom, T top, T zNear, T zFar) noexcept
{
    #if BEE_NDC_RANGE_ZO
    return Matrix4x4<T>::OrthographicRH_ZO(left, right, bottom, top, zNear, zFar);
    #else
    return Matrix4x4<T>::OrthographicRH_NO(left, right, bottom, top, zNear, zFar);
    #endif
}

// ==================================================================================
// Type Aliases
// ==================================================================================

using Matrix2x2f = Matrix2x2<f32>;
using Matrix2x2d = Matrix2x2<f64>;
using Matrix2x2i = Matrix2x2<i32>;
using Mat2f      = Matrix2x2<f32>;
using Mat2d      = Matrix2x2<f64>;
using Mat2i      = Matrix2x2<i32>;

using Matrix3x3f = Matrix3x3<f32>;
using Matrix3x3d = Matrix3x3<f64>;
using Mat3f      = Matrix3x3<f32>;
using Mat3d      = Matrix3x3<f64>;

using Matrix4x4f = Matrix4x4<f32>;
using Matrix4x4d = Matrix4x4<f64>;
using Mat4f      = Matrix4x4<f32>;
using Mat4d      = Matrix4x4<f64>;

} // namespace bee
