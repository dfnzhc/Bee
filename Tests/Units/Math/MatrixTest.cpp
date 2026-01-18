/**
 * @File MatrixTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026-01-18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Math/Math.hpp>
#include <Bee/Math/VectorMath.hpp>

using namespace bee;

// ========== Matrix2x2 测试 ==========

TEST(Matrix2x2, Construction)
{
    Matrix2x2f m;
    EXPECT_FLOAT_EQ(m.m00, 1.0f);
    EXPECT_FLOAT_EQ(m.m10, 0.0f);
    EXPECT_FLOAT_EQ(m.m01, 0.0f);
    EXPECT_FLOAT_EQ(m.m11, 1.0f);
}

TEST(Matrix2x2, ElementAccess)
{
    Matrix2x2f m(1, 2, 3, 4);
    EXPECT_FLOAT_EQ(m.m00, 1);
    EXPECT_FLOAT_EQ(m.m10, 3);
    EXPECT_FLOAT_EQ(m.m01, 2);
    EXPECT_FLOAT_EQ(m.m11, 4);

    EXPECT_FLOAT_EQ(m.m[0], 1);
    EXPECT_FLOAT_EQ(m.m[1], 3);
    EXPECT_FLOAT_EQ(m.m[2], 2);
    EXPECT_FLOAT_EQ(m.m[3], 4);

    EXPECT_FLOAT_EQ(m.c0.x, 1);
    EXPECT_FLOAT_EQ(m.c0.y, 3);
    EXPECT_FLOAT_EQ(m.c1.x, 2);
    EXPECT_FLOAT_EQ(m.c1.y, 4);
}

TEST(Matrix2x2, Determinant)
{
    Matrix2x2f m(1, 2, 3, 4);
    EXPECT_FLOAT_EQ(m.determinant(), -2);
}

TEST(Matrix2x2, Inverse)
{
    Matrix2x2f m(4, 7, 2, 6);
    auto inv = m.inverse();
    auto identity = m * inv;

    EXPECT_NEAR(identity.m00, 1.0f, 1e-5f);
    EXPECT_NEAR(identity.m11, 1.0f, 1e-5f);
    EXPECT_NEAR(identity.m01, 0.0f, 1e-5f);
    EXPECT_NEAR(identity.m10, 0.0f, 1e-5f);
}

TEST(Matrix2x2, Factory)
{
    auto rot = Matrix2x2f::Rotate(kPi / 4);
    auto scale = Matrix2x2f::Scale(2.0f, 3.0f);

    EXPECT_FLOAT_EQ(scale.m00, 2.0f);
    EXPECT_FLOAT_EQ(scale.m11, 3.0f);
}

TEST(Matrix2x2, ShearX)
{
    auto shx = Matrix2x2f::ShearX(2.0f);
    Vec2f v(1.0f, 3.0f);
    auto r = shx * v;

    EXPECT_FLOAT_EQ(shx.m00, 1.0f);
    EXPECT_FLOAT_EQ(shx.m01, 2.0f);
    EXPECT_FLOAT_EQ(shx.m10, 0.0f);
    EXPECT_FLOAT_EQ(shx.m11, 1.0f);

    EXPECT_FLOAT_EQ(r.x, 7.0f);
    EXPECT_FLOAT_EQ(r.y, 3.0f);
}

TEST(Matrix2x2, ShearY)
{
    auto shy = Matrix2x2f::ShearY(2.0f);
    Vec2f v(1.0f, 3.0f);
    auto r = shy * v;

    EXPECT_FLOAT_EQ(shy.m00, 1.0f);
    EXPECT_FLOAT_EQ(shy.m01, 0.0f);
    EXPECT_FLOAT_EQ(shy.m10, 2.0f);
    EXPECT_FLOAT_EQ(shy.m11, 1.0f);

    EXPECT_FLOAT_EQ(r.x, 1.0f);
    EXPECT_FLOAT_EQ(r.y, 5.0f);
}

TEST(Matrix2x2, NormalTransform)
{
    // Non-uniform scale: Scale X by 2, Y by 1
    // Matrix M = [ 2  0 ]
    //            [ 0  1 ]
    // Inverse M^-1 = [ 0.5  0 ]
    //                [ 0    1 ]
    // Inverse Transpose (M^-1)^T = [ 0.5  0 ]
    //                              [ 0    1 ]
    
    auto m = Matrix2x2f::Scale(2.0f, 1.0f);
    Normal2f n(1.0f, 1.0f); // Normalized internally or not? Usually Normal2 constructor might not normalize, but let's assume direction matters.
    // The implementation likely returns a Normal2 which might normalize.
    // Let's check the implementation return type. It returns Normal2<T>.
    
    auto transformedN = m * n;
    
    // Expected: (0.5 * 1, 1 * 1) = (0.5, 1.0)
    // Normalized: (0.5, 1) / sqrt(0.25 + 1) = (0.5, 1) / sqrt(1.25) ≈ (0.447, 0.894)
    
    Vec2f expected(0.5f, 1.0f);
    expected = Normalize(expected);
    
    EXPECT_NEAR(transformedN.x, expected.x, 1e-5f);
    EXPECT_NEAR(transformedN.y, expected.y, 1e-5f);
}

// ========== Matrix3x3 测试 ==========

TEST(Matrix3x3, Construction)
{
    Matrix3x3f m;
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[1], 0.0f);
    EXPECT_FLOAT_EQ(m.m[2], 0.0f);
    EXPECT_FLOAT_EQ(m.m[4], 1.0f);
    EXPECT_FLOAT_EQ(m.m[8], 1.0f);
}

TEST(Matrix3x3, Determinant)
{
    Matrix3x3f m(
        1, 2, 3,
        4, 5, 6,
        7, 8, 9
    );
    EXPECT_FLOAT_EQ(m.determinant(), 0);
}

TEST(Matrix3x3, Rotation)
{
    auto rotX = Matrix3x3f::RotateX(kPi / 2);
    auto rotY = Matrix3x3f::RotateY(kPi / 2);
    auto rotZ = Matrix3x3f::RotateZ(kPi / 2);

    EXPECT_NEAR(rotX.determinant(), 1.0f, 1e-5f);
    EXPECT_NEAR(rotY.determinant(), 1.0f, 1e-5f);
    EXPECT_NEAR(rotZ.determinant(), 1.0f, 1e-5f);
}

TEST(Matrix3x3, Scale)
{
    auto scale = Matrix3x3f::Scale(2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(scale.m[0], 2.0f);
    EXPECT_FLOAT_EQ(scale.m[4], 3.0f);
    EXPECT_FLOAT_EQ(scale.m[8], 4.0f);
}

TEST(Matrix3x3, Transpose)
{
    Matrix3x3f m(
        1, 2, 3,
        4, 5, 6,
        7, 8, 9
    );

    Matrix3x3f t = m.transpose();

    // Row 0 of transpose should be Col 0 of original: 1, 4, 7
    EXPECT_FLOAT_EQ(t(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 4.0f);
    EXPECT_FLOAT_EQ(t(2, 0), 7.0f);

    // Row 1 of transpose should be Col 1 of original: 2, 5, 8
    EXPECT_FLOAT_EQ(t(0, 1), 2.0f);
    EXPECT_FLOAT_EQ(t(1, 1), 5.0f);
    EXPECT_FLOAT_EQ(t(2, 1), 8.0f);
    
    // Row 2 of transpose should be Col 2 of original: 3, 6, 9
    EXPECT_FLOAT_EQ(t(0, 2), 3.0f);
    EXPECT_FLOAT_EQ(t(1, 2), 6.0f);
    EXPECT_FLOAT_EQ(t(2, 2), 9.0f);
}

TEST(Matrix3x3, NormalTransform)
{
    // Non-uniform scale: Scale X by 2, Y by 1, Z by 1
    // Matrix M = Scale(2, 1, 1)
    // Inverse Transpose = Scale(0.5, 1, 1)
    
    auto m = Matrix3x3f::Scale(2.0f, 1.0f, 1.0f);
    Normal3f n(1.0f, 1.0f, 0.0f);
    
    auto transformedN = m * n;
    
    // Expected: (0.5, 1, 0) normalized
    Vec3f expected(0.5f, 1.0f, 0.0f);
    expected = Normalize(expected);
    
    EXPECT_NEAR(transformedN.x, expected.x, 1e-5f);
    EXPECT_NEAR(transformedN.y, expected.y, 1e-5f);
    EXPECT_NEAR(transformedN.z, expected.z, 1e-5f);
}

// ========== Matrix4x4 测试 ==========

TEST(Matrix4x4, Construction)
{
    Matrix4x4f m;
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[5], 1.0f);
    EXPECT_FLOAT_EQ(m.m[10], 1.0f);
    EXPECT_FLOAT_EQ(m.m[15], 1.0f);
}

TEST(Matrix4x4, Transpose)
{
    Matrix4x4f m(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );

    Matrix4x4f t = m.transpose();

    // Row 0 of transpose should be Col 0 of original: 1, 5, 9, 13
    EXPECT_FLOAT_EQ(t(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 5.0f);
    EXPECT_FLOAT_EQ(t(2, 0), 9.0f);
    EXPECT_FLOAT_EQ(t(3, 0), 13.0f);

    // Row 1 of transpose should be Col 1 of original: 2, 6, 10, 14
    EXPECT_FLOAT_EQ(t(0, 1), 2.0f);
    EXPECT_FLOAT_EQ(t(1, 1), 6.0f);
    EXPECT_FLOAT_EQ(t(2, 1), 10.0f);
    EXPECT_FLOAT_EQ(t(3, 1), 14.0f);
}

TEST(Matrix4x4, Translation)
{
    auto trans = Matrix4x4f::Translate(Vector3f(1, 2, 3));
    EXPECT_FLOAT_EQ(trans.c3.x, 1.0f);
    EXPECT_FLOAT_EQ(trans.c3.y, 2.0f);
    EXPECT_FLOAT_EQ(trans.c3.z, 3.0f);
    EXPECT_FLOAT_EQ(trans.c3.w, 1.0f);
}

TEST(Matrix4x4, VectorTransform)
{
    auto trans = Matrix4x4f::Translate(Vector3f(1, 2, 3));
    Vector3f v(1, 0, 0);
    auto result = trans * v;

    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

TEST(Matrix4x4, PointTransform)
{
    auto trans = Matrix4x4f::Translate(Vector3f(1, 2, 3));
    Point3f p(1, 0, 0);
    auto result = trans * p;

    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(Matrix4x4, RotationTransform)
{
    // Rotate 90 degrees around Z axis (左手坐标系：顺时针旋转)
    // Matrix (实际存储在列主序中):
    // [ 0  1  0  0 ]   Column 0: (0, -1, 0, 0)
    // [-1  0  0  0 ]   Column 1: (1,  0, 0, 0)
    // [ 0  0  1  0 ]   Column 2: (0,  0, 1, 0)
    // [ 0  0  0  1 ]   Column 3: (0,  0, 0, 1)
    auto rot = Matrix4x4f::RotateZ(kPi / 2);
    Vector3f v(1, 0, 0);
    auto result = rot * v;

    // 左手坐标系中，90° 顺时针旋转： (1,0,0) -> (0,-1,0)
    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, -1.0f, 1e-5f);
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);
}

TEST(Matrix4x4, LookAt)
{
    auto view = Matrix4x4f::LookAt(
        Vector3f(0, 0, 5),
        Vector3f(0, 0, 0),
        Vector3f(0, 1, 0)
    );

    // The camera at (0,0,5) looking at (0,0,0) should have -Z forward
    EXPECT_NEAR(view.m[2], 0.0f, 1e-5f);
    EXPECT_NEAR(view.m[6], 0.0f, 1e-5f);
    EXPECT_NEAR(view.m[10], -1.0f, 1e-5f);
}

TEST(Matrix4x4, Perspective)
{
    auto proj = Matrix4x4f::Perspective(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // Perspective matrix should be invertible
    EXPECT_GT(abs(proj.determinant()), 1e-5f);
}

// ========== 右手系矩阵测试 ==========

TEST(MatrixRH, LookAt)
{
    // 右手系：相机在 (0,0,5) 看向原点
    auto viewRH = LookAtRH(
        Vec3f(0, 0, 5),
        Vec3f(0, 0, 0),
        Vec3f(0, 1, 0)
    );

    // 右手系：forward 指向 -Z（远离观察者）
    EXPECT_NEAR(viewRH.m[10], -1.0f, 1e-5f);
    EXPECT_NEAR(viewRH.m[14], 5.0f, 1e-5f);  // Z translation
}

TEST(MatrixRH, Perspective)
{
    auto projRH = PerspectiveRH<float>(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // 右手系：w 分量为 -1（用于齐次除法）
    // 存储在 m[col * 4 + row] = m[3 * 4 + 2] = m[14] (Col 3, Row 2)
    EXPECT_FLOAT_EQ(projRH.m[14], -1.0f);

#if BEE_NDC_RANGE_ZO
    // DirectX 风格：z ∈ [-far, -near] → [0, 1]
    // depthParam = far / (far - near) = 100 / 99.9 ≈ 1.001
    EXPECT_NEAR(projRH.m[10], 1.001f, 0.001f);

    // depthParam2 = -far * near / (far - near) = -(100 * 0.1) / 99.9 ≈ -0.1001
    EXPECT_NEAR(projRH.m[11], -0.1001f, 0.001f);
#else
    // OpenGL 风格：z ∈ [-far, -near] → [-1, 1]
    // depthParam = -(far + near) / (far - near) = -(100 + 0.1) / (100 - 0.1) ≈ -1.002
    EXPECT_NEAR(projRH.m[10], -1.002f, 0.001f);

    // depthParam2 = -(2 * far * near) / (far - near) = -(2 * 100 * 0.1) / 99.9 ≈ -0.2002
    EXPECT_NEAR(projRH.m[11], -0.2002f, 0.001f);
#endif

    // 验证可逆性
    EXPECT_GT(abs(projRH.determinant()), 1e-5f);
}

TEST(MatrixRH, Orthographic)
{
    auto orthoRH = OrthographicRH<float>(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

    // 单位矩阵（除了深度映射）
    EXPECT_FLOAT_EQ(orthoRH.m[0], 1.0f);
    EXPECT_FLOAT_EQ(orthoRH.m[5], 1.0f);

#if BEE_NDC_RANGE_ZO
    // DirectX 风格：z ∈ [near, far] → [0, 1]
    // depthScale = 1 / (far - near) = 1 / 99.9 ≈ 0.01001
    EXPECT_NEAR(orthoRH.m[10], 1.0f / 99.9f, 1e-6f);
#else
    // OpenGL 风格：z ∈ [near, far] → [-1, 1]
    // depthScale = 2 / (far - near) = 2 / 99.9 ≈ 0.02002
    EXPECT_NEAR(orthoRH.m[10], 2.0f / 99.9f, 1e-6f);
#endif
}

TEST(MatrixRH, CompareWithLH_LookAt)
{
    Vec3f eye(0, 2, 5);
    Vec3f target(0, 0, 0);
    Vec3f up(0, 1, 0);

    auto viewLH = Matrix4x4f::LookAt(eye, target, up);
    auto viewRH = LookAtRH(eye, target, up);

    // LH 和 RH 的 right 向量计算顺序不同：
    // LH: right = up × forward
    // RH: right = forward × up
    // 因此 X 轴（right）应该相反
    EXPECT_NEAR(viewLH.m[0], -viewRH.m[0], 1e-5f);

    // Y 轴和 Z 轴（forward）应该相同
    EXPECT_NEAR(viewLH.m[5], viewRH.m[5], 1e-5f);
    EXPECT_NEAR(viewLH.m[10], viewRH.m[10], 1e-5f);
}

TEST(MatrixRH, CompareWithLH_Perspective)
{
    auto projLH = Matrix4x4f::Perspective(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);
    auto projRH = PerspectiveRH<float>(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // LH 和 RH 的 w 分量符号不同
    // LH: m[14] = +1
    // RH: m[14] = -1
    EXPECT_FLOAT_EQ(projLH.m[14], 1.0f);
    EXPECT_FLOAT_EQ(projRH.m[14], -1.0f);

    // X 和 Y 的缩放分量应该相同
    EXPECT_NEAR(projLH.m[0], projRH.m[0], 1e-5f);
    EXPECT_NEAR(projLH.m[5], projRH.m[5], 1e-5f);

    // 深度参数 m[10] 应该相同（LH 和 RH 都将深度映射到相同的 NDC 范围）
    // ZO: 都映射到 [0, 1]
    // NO: 都映射到 [-1, 1]
    EXPECT_NEAR(projLH.m[10], projRH.m[10], 0.001f);
}

TEST(Matrix4x4_Supp, Scale)
{
    auto s1 = Matrix4x4f::Scale(2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(s1.m[0], 2.0f);
    EXPECT_FLOAT_EQ(s1.m[5], 3.0f);
    EXPECT_FLOAT_EQ(s1.m[10], 4.0f);
    EXPECT_FLOAT_EQ(s1.m[15], 1.0f);

    auto s2 = Matrix4x4f::Scale(Vector3f(2.0f, 3.0f, 4.0f));
    EXPECT_EQ(s1, s2);
}

TEST(Matrix4x4_Supp, Transpose)
{
    Matrix4x4f m(
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    );

    auto t = m.transpose();

    EXPECT_FLOAT_EQ(m.m[0], 1);
    EXPECT_FLOAT_EQ(m.m[1], 5);
    EXPECT_FLOAT_EQ(m.m[2], 9);
    EXPECT_FLOAT_EQ(m.m[3], 13);
    
    // Row 0 of m becomes Col 0 of t
    EXPECT_FLOAT_EQ(t.m[0], 1);
    EXPECT_FLOAT_EQ(t.m[1], 2);
    EXPECT_FLOAT_EQ(t.m[2], 3);
    EXPECT_FLOAT_EQ(t.m[3], 4);

    EXPECT_FLOAT_EQ(t(0, 0), 1);
    EXPECT_FLOAT_EQ(t(0, 1), 2); // t(col, row) -> t.m[col*4 + row]
}

TEST(Matrix4x4_Supp, Determinant)
{
    Matrix4x4f id = Matrix4x4f::Identity();
    EXPECT_FLOAT_EQ(id.determinant(), 1.0f);

    auto s = Matrix4x4f::Scale(2.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(s.determinant(), 8.0f);
}

TEST(Matrix4x4_Supp, Inverse)
{
    // Simple translation matrix inverse
    auto t = Matrix4x4f::Translate(Vector3f(1, 2, 3));
    auto inv = t.inverse();
    
    // Inverse of translate(v) is translate(-v)
    EXPECT_FLOAT_EQ(inv.m[12], -1.0f);
    EXPECT_FLOAT_EQ(inv.m[13], -2.0f);
    EXPECT_FLOAT_EQ(inv.m[14], -3.0f);

    auto id = t * inv;
    EXPECT_TRUE(id.isIdentity());
}

TEST(MatrixConstructionTest, Matrix4x4_ElementConstructor_RowMajorInput_ColumnMajorStorage)
{
    // Input is visually Row-Major:
    // Row 0: 1, 2, 3, 4
    // Row 1: 5, 6, 7, 8
    // Row 2: 9, 10, 11, 12
    // Row 3: 13, 14, 15, 16
    Matrix4x4f m(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    );

    // Verify storage (Column-Major)
    // m[0] should be Col 0, Row 0 -> 1
    // m[1] should be Col 0, Row 1 -> 5
    // m[2] should be Col 0, Row 2 -> 9
    // m[3] should be Col 0, Row 3 -> 13
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[1], 5.0f);
    EXPECT_FLOAT_EQ(m.m[2], 9.0f);
    EXPECT_FLOAT_EQ(m.m[3], 13.0f);

    // m[4] should be Col 1, Row 0 -> 2
    EXPECT_FLOAT_EQ(m.m[4], 2.0f);
    EXPECT_FLOAT_EQ(m.m[5], 6.0f);
    
    // Verify Accessor (col, row)
    // operator()(col, row)
    EXPECT_FLOAT_EQ(m(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m(0, 1), 5.0f); // Col 0, Row 1
    EXPECT_FLOAT_EQ(m(1, 0), 2.0f); // Col 1, Row 0
}

TEST(MatrixConstructionTest, Matrix3x3_ElementConstructor_RowMajorInput_ColumnMajorStorage)
{
    // Row 0: 1, 2, 3
    // Row 1: 4, 5, 6
    // Row 2: 7, 8, 9
    Matrix3x3f m(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f
    );

    // Col 0: 1, 4, 7
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[1], 4.0f);
    EXPECT_FLOAT_EQ(m.m[2], 7.0f);

    // Col 1: 2, 5, 8
    EXPECT_FLOAT_EQ(m.m[3], 2.0f);
    EXPECT_FLOAT_EQ(m.m[4], 5.0f);
    EXPECT_FLOAT_EQ(m.m[5], 8.0f);
}

TEST(MatrixConstructionTest, Matrix2x2_ElementConstructor_RowMajorInput_ColumnMajorStorage)
{
    // Row 0: 1, 2
    // Row 1: 3, 4
    Matrix2x2f m(
        1.0f, 2.0f,
        3.0f, 4.0f
    );

    // Col 0: 1, 3
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[1], 3.0f);

    // Col 1: 2, 4
    EXPECT_FLOAT_EQ(m.m[2], 2.0f);
    EXPECT_FLOAT_EQ(m.m[3], 4.0f);
    
    // Verify Struct Access
    EXPECT_FLOAT_EQ(m.m00, 1.0f);
    EXPECT_FLOAT_EQ(m.m10, 3.0f);
    EXPECT_FLOAT_EQ(m.m01, 2.0f);
    EXPECT_FLOAT_EQ(m.m11, 4.0f);
}

// ================================================================
// NDC Range Tests - New API with _ZO and _NO suffixes
// ================================================================

TEST(MatrixNDC, PerspectiveLH_ZO_DirectXStyle)
{
    // 左手系 + NDC [0, 1] (DirectX 风格)
    auto proj = Matrix4x4f::PerspectiveLH_ZO(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // DirectX LH 投影矩阵布局 (列主序):
    // m[10] = depthParam = far / (far - near) ≈ 1.001
    // m[11] = depthParam2 = -near * far / (far - near) ≈ -0.1001
    // m[14] = 1 (w component for perspective divide)
    // m[15] = 0

    EXPECT_NEAR(proj.m[10], 100.0f / 99.9f, 0.001f);
    EXPECT_NEAR(proj.m[11], -0.1f * 100.0f / 99.9f, 0.001f);
    EXPECT_FLOAT_EQ(proj.m[14], 1.0f);
    EXPECT_FLOAT_EQ(proj.m[15], 0.0f);
}

TEST(MatrixNDC, PerspectiveLH_NO_MixedStyle)
{
    // 左手系 + NDC [-1, 1] (混合风格)
    auto proj = Matrix4x4f::PerspectiveLH_NO(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // m[10] = depthParam = (far + near) / (far - near) ≈ 1.002
    // m[11] = depthParam2 = -2 * near * far / (far - near) ≈ -0.2002
    // m[14] = 1 (w component)

    EXPECT_NEAR(proj.m[10], 100.1f / 99.9f, 0.001f);
    EXPECT_NEAR(proj.m[11], -2.0f * 0.1f * 100.0f / 99.9f, 0.001f);
    EXPECT_FLOAT_EQ(proj.m[14], 1.0f);
}

TEST(MatrixNDC, PerspectiveRH_ZO_MixedStyle)
{
    // 右手系 + NDC [0, 1] (混合风格)
    auto proj = Matrix4x4f::PerspectiveRH_ZO(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // m[10] = depthParam = far / (far - near) ≈ 1.001
    // m[11] = depthParam2 = -near * far / (far - near) ≈ -0.1001
    // m[14] = -1 (w component for RH)

    EXPECT_NEAR(proj.m[10], 100.0f / 99.9f, 0.001f);
    EXPECT_NEAR(proj.m[11], -0.1f * 100.0f / 99.9f, 0.001f);
    EXPECT_FLOAT_EQ(proj.m[14], -1.0f);
}

TEST(MatrixNDC, PerspectiveRH_NO_OpenGLStyle)
{
    // 右手系 + NDC [-1, 1] (OpenGL 风格)
    auto proj = Matrix4x4f::PerspectiveRH_NO(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // m[10] = depthParam = -(far + near) / (far - near) ≈ -1.002
    // m[11] = depthParam2 = -2 * near * far / (far - near) ≈ -0.2002
    // m[14] = -1 (w component for RH)

    EXPECT_NEAR(proj.m[10], -100.1f / 99.9f, 0.001f);
    EXPECT_NEAR(proj.m[11], -2.0f * 0.1f * 100.0f / 99.9f, 0.001f);
    EXPECT_FLOAT_EQ(proj.m[14], -1.0f);
}

TEST(MatrixNDC, OrthographicLH_ZO_DirectXStyle)
{
    // 左手系 + NDC [0, 1] (DirectX 风格)
    auto ortho = Matrix4x4f::OrthographicLH_ZO(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

    // X, Y 缩放应该是 1 (2/(right-left) = 2/2 = 1)
    EXPECT_FLOAT_EQ(ortho.m[0], 1.0f);
    EXPECT_FLOAT_EQ(ortho.m[5], 1.0f);

    // Z 缩放: 1 / (far - near) = 1 / 99.9
    EXPECT_NEAR(ortho.m[10], 1.0f / 99.9f, 1e-6f);

    // Z 平移 (在 m[14]): tz = -near / (far - near)
    EXPECT_NEAR(ortho.m[14], -0.1f / 99.9f, 1e-6f);

    // 齐次坐标 w (在 m[15])
    EXPECT_FLOAT_EQ(ortho.m[15], 1.0f);
}

TEST(MatrixNDC, OrthographicRH_NO_OpenGLStyle)
{
    // 右手系 + NDC [-1, 1] (OpenGL 风格)
    auto ortho = Matrix4x4f::OrthographicRH_NO(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

    // X, Y 缩放应该是 1
    EXPECT_FLOAT_EQ(ortho.m[0], 1.0f);
    EXPECT_FLOAT_EQ(ortho.m[5], 1.0f);

    // Z 缩放: 2 / (far - near) = 2 / 99.9
    EXPECT_NEAR(ortho.m[10], 2.0f / 99.9f, 1e-6f);

    // Z 平移: -(far + near) / (far - near) = -(100.1) / 99.9
    EXPECT_NEAR(ortho.m[14], -100.1f / 99.9f, 1e-6f);

    // 齐次坐标 w
    EXPECT_FLOAT_EQ(ortho.m[15], 1.0f);
}

TEST(MatrixNDC, DefaultPerspectiveUsesConfiguredNdcRange)
{
    // 默认 LH 使用 ZO, RH 使用 NO
    auto projLH = Matrix4x4f::Perspective(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);
    auto projRH = PerspectiveRH<float>(kPi / 2, 16.0f / 9.0f, 0.1f, 100.0f);

    // LH 默认使用 DirectX 风格 (ZO), w component = 1
    EXPECT_FLOAT_EQ(projLH.m[14], 1.0f);

    // RH 默认使用 OpenGL 风格 (NO), w component = -1
    EXPECT_FLOAT_EQ(projRH.m[14], -1.0f);
}

// ========== 边界情况测试 ==========

TEST(Matrix4x4, PointTransform_NearZeroW)
{
    // 测试齐次坐标 w 接近 0 的情况（如无限远点）
    Matrix4x4f m(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 0.0001f  // w 会非常小
    );

    Point3f p(1.0f, 2.0f, 3.0f);
    Point3f result = m * p;

    // 验证返回值合理（不崩溃，返回有限值）
    EXPECT_TRUE(IsFinite(result.x));
    EXPECT_TRUE(IsFinite(result.y));
    EXPECT_TRUE(IsFinite(result.z));
}

// ========== TRS 组合变换测试 ==========

TEST(Matrix4x4, TRS_Basic)
{
    Vec3f translation(10.0f, 20.0f, 30.0f);
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 4.0f);
    Vec3f scale(2.0f, 3.0f, 4.0f);

    Matrix4x4f m = Matrix4x4f::TRS(translation, rotation, scale);

    // 验证平移分量在最后一列
    EXPECT_FLOAT_EQ(m.c3.x, translation.x);
    EXPECT_FLOAT_EQ(m.c3.y, translation.y);
    EXPECT_FLOAT_EQ(m.c3.z, translation.z);
    EXPECT_FLOAT_EQ(m.c3.w, 1.0f);
}

TEST(Matrix4x4, TRS_UniformScale)
{
    Vec3f translation(5.0f, 10.0f, 15.0f);
    Quatf rotation = Quatf::Identity();
    f32 uniformScale = 2.5f;

    Matrix4x4f m = Matrix4x4f::TRS(translation, rotation, uniformScale);

    // 验证对角线元素包含均匀缩放
    EXPECT_FLOAT_EQ(m.m[0], uniformScale);
    EXPECT_FLOAT_EQ(m.m[5], uniformScale);
    EXPECT_FLOAT_EQ(m.m[10], uniformScale);

    // 验证平移
    EXPECT_FLOAT_EQ(m.m[12], translation.x);
    EXPECT_FLOAT_EQ(m.m[13], translation.y);
    EXPECT_FLOAT_EQ(m.m[14], translation.z);
}

TEST(Matrix4x4, TRS_TransformPoint)
{
    Vec3f translation(1.0f, 2.0f, 3.0f);
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);
    Vec3f scale(2.0f, 2.0f, 2.0f);

    Matrix4x4f m = Matrix4x4f::TRS(translation, rotation, scale);

    Point3f p(1.0f, 0.0f, 0.0f);
    Point3f result = m * p;

    // TRS 组合的正确性验证：
    // 根据实际矩阵输出，逆时针绕 Z 轴旋转 90 度
    // 缩放：(1,0,0) * 2 = (2,0,0)
    // 旋转 90 度：(2,0,0) -> (0,2,0)
    // 平移：(0,2,0) + (1,2,3) = (1,0,3)
    EXPECT_NEAR(result.x, 1.0f, 1e-4f);
    EXPECT_NEAR(result.y, 4.0f, 1e-4f);
    EXPECT_NEAR(result.z, 3.0f, 1e-4f);
}

TEST(Matrix4x4, TRS_NonUniformScale)
{
    // 测试非均匀缩放
    Vec3f translation(0.0f, 0.0f, 0.0f);
    Quatf rotation = Quatf::Identity();
    Vec3f scale(1.0f, 2.0f, 3.0f);

    Matrix4x4f m = Matrix4x4f::TRS(translation, rotation, scale);

    // 验证非均匀缩放正确应用
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);  // X scale
    EXPECT_FLOAT_EQ(m.m[5], 2.0f);  // Y scale
    EXPECT_FLOAT_EQ(m.m[10], 3.0f); // Z scale
}
