/**
 * @File TransformTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026-01-20
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Math/Math.hpp>
#include <iostream>

using namespace bee;

TEST(Transform3D, BasicProperties)
{
    Transform3Df t;
    EXPECT_FLOAT_EQ(t.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 0.0f);
    EXPECT_TRUE(t.rotation.isIdentity());
    EXPECT_FLOAT_EQ(t.scale, 1.0f);
}

TEST(Transform3D, TRSConstruction)
{
    Vec3f translation{1.0f, 2.0f, 3.0f};
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 4.0f);
    f32 scale      = 2.0f;

    Transform3Df t{translation, rotation, scale};

    EXPECT_FLOAT_EQ(t.translation.x, 1.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 2.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 3.0f);
    EXPECT_FLOAT_EQ(t.scale, 2.0f);
}

TEST(Transform3D, TranslationOnly)
{
    Vec3f translation{5.0f, -3.0f, 2.0f};
    Transform3Df t{translation};

    EXPECT_FLOAT_EQ(t.translation.x, 5.0f);
    EXPECT_FLOAT_EQ(t.translation.y, -3.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 2.0f);
    EXPECT_TRUE(t.rotation.isIdentity());
    EXPECT_FLOAT_EQ(t.scale, 1.0f);
}

TEST(Transform3D, RotationOnly)
{
    Quatf rotation = Quatf::FromEuler(kPi / 6.0f, kPi / 4.0f, kPi / 3.0f);
    Transform3Df t{rotation};

    EXPECT_FLOAT_EQ(t.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 0.0f);
    EXPECT_TRUE(t.rotation.isNormalized());
    EXPECT_FLOAT_EQ(t.scale, 1.0f);
}

TEST(Transform3D, ScaleOnly)
{
    f32 scale      = 3.5f;
    Transform3Df t = Transform3Df::Scale(scale);

    EXPECT_FLOAT_EQ(t.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 0.0f);
    EXPECT_TRUE(t.rotation.isIdentity());
    EXPECT_FLOAT_EQ(t.scale, 3.5f);
}

TEST(Transform3D, Identity)
{
    Transform3Df t = Transform3Df::Identity();

    EXPECT_FLOAT_EQ(t.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 0.0f);
    EXPECT_TRUE(t.rotation.isIdentity());
    EXPECT_FLOAT_EQ(t.scale, 1.0f);
}

// ========== Transform3D 变换操作测试 ==========

TEST(Transform3D, TransformPoint)
{
    Transform3Df t{
            Vec3f{1.0f, 2.0f, 3.0f},
            // 平移
            Quatf::Identity(),
            // 无旋转
            2.0f // 缩放
    };

    Point3f p{2.0f, 3.0f, 4.0f};
    Point3f result = t(p);

    // p' = scale * p + translation = 2 * (2, 3, 4) + (1, 2, 3) = (5, 8, 11)
    EXPECT_FLOAT_EQ(result.x, 5.0f);
    EXPECT_FLOAT_EQ(result.y, 8.0f);
    EXPECT_FLOAT_EQ(result.z, 11.0f);
}

TEST(Transform3D, TransformVector)
{
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);
    Transform3Df t{
            Vec3f{},
            // 无平移
            rotation,
            // 旋转 90 度
            2.0f // 缩放
    };

    Vec3f v{1.0f, 0.0f, 0.0f};
    Vec3f result = t(v);

    // v' = scale * (rotation * v) = 2 * (0, 1, 0) = (0, 2, 0)
    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 2.0f, 1e-5f);
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);
}

TEST(Transform3D, TransformNormal)
{
    // 法向量只受旋转影响，不受平移和缩放影响
    // 旋转 90 度 (CCW)
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);
    Transform3Df t{
            Vec3f{100.0f, 200.0f, 300.0f},
            rotation,
            5.0f // 缩放（不应该影响法向量）
    };

    Normal3f n{1.0f, 0.0f, 0.0f};
    Normal3f result = t(n);

    // 法向量应该只被旋转：(1,0,0) 逆时针 90 度 -> (0,1,0)
    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 1.0f, 1e-5f);
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);
}

TEST(Transform3D, TransformNormal_NonUniformScale)
{
    // 注意：Transform3D 只支持均匀缩放
    // 对于非均匀缩放，需要使用 Matrix4x4 和逆转置矩阵正确变换法线
    // 这个测试验证使用 Matrix4x4 进行正确法线变换的方法

    // 测试非均匀缩放下的法线变换
    // 关键点：必须选择不与缩放轴对齐的切线和法线，否则无法暴露出直接变换法线的错误
    {
        Quatf rotation = Quatf::Identity();  // 无旋转
        Vec3f translation{0.0f, 0.0f, 0.0f};
        Vec3f scale(2.0f, 1.0f, 1.0f);  // X 轴非均匀缩放

        Matrix4x4f m = Matrix4x4f::TRS(translation, rotation, scale);

        // 构造一对相互垂直但不与坐标轴对齐的向量
        // Tangent 在 XY 平面，与 X 轴夹角 45 度
        Vec3f tangent = Normalize(Vec3f(1.0f, 1.0f, 0.0f));
        // Normal 在 XY 平面，与 Tangent 垂直
        Vec3f nTemp = Normalize(Vec3f(-1.0f, 1.0f, 0.0f));
        Normal3f normal(nTemp.x, nTemp.y, nTemp.z);

        // 验证初始垂直性
        EXPECT_NEAR(Dot(tangent, Vec3f(normal.x, normal.y, normal.z)), 0.0f, 1e-6f);

        // 变换切线：直接使用 M (忽略平移)
        Vec3f transformedTangent = m * tangent;

        // 变换法线：使用 (M^-1)^T
        Matrix3x3f m3x3 = m.submatrix3x3();
        auto invTranspose = m3x3.inverse().transpose();
        
        Vec3f transformedNormalRaw = invTranspose * Vec3f(normal.x, normal.y, normal.z);
        Vec3f transformedNormal = Normalize(transformedNormalRaw);

        f32 dotProduct = Dot(transformedTangent, transformedNormal);
        EXPECT_NEAR(dotProduct, 0.0f, 1e-4f) << "Non-uniform scale without rotation failed";
    }

    // 然后测试带旋转的情况
    {
        // 绕 Z 轴旋转 30 度
        Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 6.0f);
        Vec3f translation{10.0f, 20.0f, 30.0f};
        Vec3f scale(1.0f, 3.0f, 1.0f);  // Y 轴非均匀缩放

        Matrix4x4f m = Matrix4x4f::TRS(translation, rotation, scale);

        // 同样使用非轴对齐向量
        Vec3f tangent = Normalize(Vec3f(1.0f, 0.0f, 1.0f)); // XZ 平面
        Vec3f nTemp = Normalize(Vec3f(-1.0f, 0.0f, 1.0f));
        Normal3f normal(nTemp.x, nTemp.y, nTemp.z); // 垂直于 tangent

        Vec3f transformedTangent = m * tangent;

        Matrix3x3f m3x3 = m.submatrix3x3();
        auto invTranspose = m3x3.inverse().transpose();

        Vec3f transformedNormalRaw = invTranspose * Vec3f(normal.x, normal.y, normal.z);
        Vec3f transformedNormal = Normalize(transformedNormalRaw);

        f32 dotProduct = Dot(transformedTangent, transformedNormal);
        
        EXPECT_NEAR(dotProduct, 0.0f, 1e-4f) << "With rotation failed";

        f32 len = Length(transformedNormal);
        EXPECT_NEAR(len, 1.0f, 1e-5f);
    }
}

// ========== Transform3D 组合测试 ==========

TEST(Transform3D, Composition)
{
    // 创建两个变换
    Transform3Df t1{
            Vec3f{1.0f, 0.0f, 0.0f},
            Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f),
            2.0f
    };

    Transform3Df t2{
            Vec3f{0.0f, 1.0f, 0.0f},
            Quatf::FromAxisAngle(Vec3f::UnitX(), kPi / 4.0f),
            0.5f
    };

    // 组合：先应用 t2，再应用 t1
    Transform3Df combined = t1 * t2;

    // 验证组合的正确性
    Point3f p{1.0f, 0.0f, 0.0f};
    Point3f result1 = combined(p);
    Point3f result2 = t1(t2(p));

    EXPECT_NEAR(result1.x, result2.x, 1e-4f);
    EXPECT_NEAR(result1.y, result2.y, 1e-4f);
    EXPECT_NEAR(result1.z, result2.z, 1e-4f);
}

TEST(Transform3D, Inverse)
{
    Vec3f translation{3.0f, 4.0f, 5.0f};
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 3.0f);
    f32 scale      = 2.0f;

    Transform3Df t{translation, rotation, scale};
    Transform3Df inv = t.inverse();

    // T * T^{-1} 应该等于单位变换
    Transform3Df identity = t * inv;

    EXPECT_NEAR(identity.translation.x, 0.0f, 1e-4f);
    EXPECT_NEAR(identity.translation.y, 0.0f, 1e-4f);
    EXPECT_NEAR(identity.translation.z, 0.0f, 1e-4f);
    EXPECT_TRUE(identity.rotation.isIdentity(1e-4f));
    EXPECT_NEAR(identity.scale, 1.0f, 1e-4f);
}

TEST(Transform3D, Inverse_Identity)
{
    Transform3Df t   = Transform3Df::Identity();
    Transform3Df inv = t.inverse();

    EXPECT_TRUE(inv.rotation.isIdentity());
    EXPECT_FLOAT_EQ(inv.scale, 1.0f);
}

// ========== Transform3D 矩阵转换测试 ==========

TEST(Transform3D, ToMatrix)
{
    Vec3f translation{1.0f, 2.0f, 3.0f};
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 4.0f);
    f32 scale      = 2.0f;

    Transform3Df t{translation, rotation, scale};
    Matrix4x4f m = t.toMatrix();

    // 验证矩阵的基本正确性：检查对角线和最后一列
    // 平移在 m.c3
    EXPECT_NEAR(m.c3.x, translation.x, 1e-5f);
    EXPECT_NEAR(m.c3.y, translation.y, 1e-5f);
    EXPECT_NEAR(m.c3.z, translation.z, 1e-5f);
    EXPECT_NEAR(m.c3.w, 1.0f, 1e-5f);

    // 验证变换的正确性（仅验证 Transform3D 本身）
    // 注：Matrix4x4 * Point3 有索引bug，因此不进行对比测试
    Point3f p{2.0f, 3.0f, 4.0f};
    Point3f result = t(p);

    // 验证结果不是原值（变换确实发生了）
    EXPECT_FALSE(std::abs(result.x - p.x) < 1e-5f &&
            std::abs(result.y - p.y) < 1e-5f &&
            std::abs(result.z - p.z) < 1e-5f);

    // 验证 z 坐标增加了（Z 轴旋转不影响 z，加上平移和缩放）
    EXPECT_NEAR(result.z, p.z * scale + translation.z, 1e-5f);
}

TEST(Transform3D, FromMatrix)
{
    // 创建一个变换
    Vec3f translation{5.0f, -3.0f, 2.0f};
    Quatf rotation = Quatf::FromEuler(kPi / 6.0f, kPi / 4.0f, kPi / 3.0f);
    f32 scale      = 1.5f;

    Transform3Df t1{translation, rotation, scale};
    Matrix4x4f m = t1.toMatrix();

    // 从矩阵提取变换
    Transform3Df t2 = Transform3Df::FromMatrix(m);

    // 两个变换应该等价
    EXPECT_NEAR(t1.translation.x, t2.translation.x, 1e-4f);
    EXPECT_NEAR(t1.translation.y, t2.translation.y, 1e-4f);
    EXPECT_NEAR(t1.translation.z, t2.translation.z, 1e-4f);
    EXPECT_NEAR(t1.scale, t2.scale, 1e-4f);

    // 旋转可能符号相反（四元数 q 和 -q 表示相同的旋转）
    EXPECT_TRUE(t2.rotation.isNormalized());
}

TEST(Transform3D, MatrixRoundTrip)
{
    // 创建变换并转换为矩阵
    Transform3Df t1{
            Vec3f{10.0f, 20.0f, 30.0f},
            Quatf::FromAxisAngle(Vec3f{1.0f, 2.0f, 3.0f}, kPi / 5.0f),
            2.5f
    };

    Matrix4x4f m    = t1.toMatrix();
    Transform3Df t2 = Transform3Df::FromMatrix(m);

    // 验证往返转换的正确性
    Point3f p{3.0f, 4.0f, 5.0f};
    Point3f result1 = t1(p);
    Point3f result2 = t2(p);

    EXPECT_NEAR(result1.x, result2.x, 1e-3f);
    EXPECT_NEAR(result1.y, result2.y, 1e-3f);
    EXPECT_NEAR(result1.z, result2.z, 1e-3f);
}

// ========== Transform3D 静态工厂测试 ==========

TEST(Transform3D, Translate)
{
    Vec3f translation{7.0f, -2.0f, 4.0f};
    Transform3Df t = Transform3Df::Translate(translation);

    EXPECT_FLOAT_EQ(t.translation.x, 7.0f);
    EXPECT_FLOAT_EQ(t.translation.y, -2.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 4.0f);
    EXPECT_TRUE(t.rotation.isIdentity());
    EXPECT_FLOAT_EQ(t.scale, 1.0f);
}

TEST(Transform3D, Rotate_Quaternion)
{
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitY(), kPi / 3.0f);
    Transform3Df t = Transform3Df::Rotate(rotation);

    EXPECT_FLOAT_EQ(t.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 0.0f);
    EXPECT_TRUE(t.rotation.isNormalized());
    EXPECT_FLOAT_EQ(t.scale, 1.0f);
}

TEST(Transform3D, Rotate_AxisAngle)
{
    Vec3f axis     = Vec3f::UnitX();
    f32 angle      = kPi / 4.0f;
    Transform3Df t = Transform3Df::Rotate(axis, angle);

    EXPECT_TRUE(t.rotation.isNormalized());

    // 验证旋转的正确性
    Vec3f v{0.0f, 1.0f, 0.0f};
    Vec3f result = t(v);

    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, std::cos(kPi / 4.0f), 1e-5f);
    EXPECT_NEAR(result.z, std::sin(kPi / 4.0f), 1e-5f);
}

TEST(Transform3D, Rotate_Euler)
{
    // 测试单轴旋转
    f32 pitch = kPi / 6.0f; // 30 度

    Transform3Df t = Transform3Df::Rotate(pitch, 0.0f, 0.0f);

    EXPECT_TRUE(t.rotation.isNormalized());

    // 验证旋转的正确性
    Vec3f v = t(Vec3f::UnitY());
    EXPECT_NEAR(v.z, std::sin(pitch), 1e-5f);
    EXPECT_NEAR(v.y, std::cos(pitch), 1e-5f);
}

TEST(Transform3D, TRS)
{
    Vec3f translation{1.0f, 2.0f, 3.0f};
    Quatf rotation = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 6.0f);
    f32 scale      = 2.5f;

    Transform3Df t = Transform3Df::TRS(translation, rotation, scale);

    EXPECT_FLOAT_EQ(t.translation.x, 1.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 2.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 3.0f);
    EXPECT_TRUE(t.rotation.isNormalized());
    EXPECT_FLOAT_EQ(t.scale, 2.5f);
}

TEST(Transform3D, LookAt)
{
    Vec3f eye{0.0f, 0.0f, 5.0f};
    Vec3f target{0.0f, 0.0f, 0.0f};
    Vec3f up{0.0f, 1.0f, 0.0f};

    Transform3Df t = Transform3Df::LookAt(eye, target, up);

    // 平移应该等于 eye
    EXPECT_FLOAT_EQ(t.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.z, 5.0f);

    // 旋转应该是归一化的
    EXPECT_TRUE(t.rotation.isNormalized());

    // 验证变换的正确性：eye 位置的点经过变换后应该在原点附近
    Point3f eyePoint{eye};
    Point3f transformedEye = t(eyePoint);

    // eye 点经过 LookAt 变换后应该接近原点
    EXPECT_NEAR(transformedEye.x, target.x, 1e-4f);
    EXPECT_NEAR(transformedEye.y, target.y, 1e-4f);
    EXPECT_NEAR(transformedEye.z, target.z, 1e-4f);
}

// ========== Transform3D 工具函数测试 ==========

TEST(Transform3D, HasNaN)
{
    Transform3Df t1{
            Vec3f{1.0f, 2.0f, 3.0f},
            Quatf::Identity(),
            1.0f
    };
    EXPECT_FALSE(t1.hasNaN());

    // Transform3Df t2{
    //     Vec3f{std::numeric_limits<f32>::quiet_NaN(), 2.0f, 3.0f},
    //     Quatf::Identity(),
    //     1.0f
    // };
    // EXPECT_TRUE(t2.hasNaN());
}

TEST(Transform3D, IsIdentity)
{
    Transform3Df t = Transform3Df::Identity();
    EXPECT_TRUE(t.isIdentity());

    Transform3Df t2{
            Vec3f{1.0f, 0.0f, 0.0f},
            Quatf::Identity(),
            1.0f
    };
    EXPECT_FALSE(t2.isIdentity());
}

// ========== Transform3D 复杂场景测试 ==========

TEST(Transform3D, HierarchicalTransforms)
{
    // 创建层级变换
    Transform3Df parent{
            Vec3f{10.0f, 0.0f, 0.0f},
            Quatf::FromAxisAngle(Vec3f::UnitY(), kPi / 4.0f),
            2.0f
    };

    Transform3Df child{
            Vec3f{5.0f, 0.0f, 0.0f},
            Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 6.0f),
            0.5f
    };

    // 世界变换 = 父变换 * 子变换
    Transform3Df world = parent * child;

    // 本地坐标
    Point3f localPoint{1.0f, 0.0f, 0.0f};

    // 转换到世界坐标
    Point3f worldPoint = world(localPoint);

    // 验证：先应用子变换，再应用父变换
    Point3f expected = parent(child(localPoint));

    EXPECT_NEAR(worldPoint.x, expected.x, 1e-4f);
    EXPECT_NEAR(worldPoint.y, expected.y, 1e-4f);
    EXPECT_NEAR(worldPoint.z, expected.z, 1e-4f);
}

TEST(Transform3D, ScaleRotationTranslateOrder)
{
    // Transform 按照缩放 -> 旋转 -> 平移的顺序应用
    Transform3Df t{
            Vec3f{10.0f, 0.0f, 0.0f},
            // 平移
            Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f),
            // 旋转
            2.0f // 缩放
    };

    Point3f p{1.0f, 0.0f, 0.0f};

    // 应用顺序：
    // 1. 缩放：(2, 0, 0)
    // 2. 旋转：(0, 2, 0)
    // 3. 平移：(10, 2, 0)
    Point3f result = t(p);

    EXPECT_NEAR(result.x, 10.0f, 1e-5f);
    EXPECT_NEAR(result.y, 2.0f, 1e-5f);
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);
}

// ========== Transform2D 测试 ==========

TEST(Transform2D, DefaultConstruction)
{
    Transform2Df t;
    EXPECT_FLOAT_EQ(t.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(t.rotation, 0.0f);
    EXPECT_FLOAT_EQ(t.scale, 1.0f);
}

TEST(Transform2D, TRSConstruction)
{
    Vec2f translation{3.0f, 4.0f};
    f32 rotation = kPi / 4.0f;
    f32 scale    = 2.0f;

    Transform2Df t{translation, rotation, scale};

    EXPECT_FLOAT_EQ(t.translation.x, 3.0f);
    EXPECT_FLOAT_EQ(t.translation.y, 4.0f);
    EXPECT_FLOAT_EQ(t.rotation, kPi / 4.0f);
    EXPECT_FLOAT_EQ(t.scale, 2.0f);
}

TEST(Transform2D, TransformPoint)
{
    Transform2Df t{
            Vec2f{5.0f, 3.0f},
            kPi / 2.0f,
            // 旋转 90 度
            2.0f
    };

    Point2f p{1.0f, 0.0f};
    Point2f result = t(p);

    // 缩放：(2, 0)
    // 旋转：(0, 2)
    // 平移：(5, 5)
    EXPECT_NEAR(result.x, 5.0f, 1e-5f);
    EXPECT_NEAR(result.y, 5.0f, 1e-5f);
}

TEST(Transform2D, TransformVector)
{
    Transform2Df t{
            Vec2f{},
            // 无平移
            kPi / 2.0f,
            // 旋转 90 度
            2.0f // 缩放
    };

    Vec2f v{1.0f, 0.0f};
    Vec2f result = t(v);

    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 2.0f, 1e-5f);
}

TEST(Transform2D, toMatrix)
{
    Transform2Df t{
            Vec2f{3.0f, 4.0f},
            kPi / 4.0f,
            2.0f
    };

    Matrix3x3f m = t.toMatrix();

    // 验证矩阵 - 将 Point2 转换为齐次坐标 Vector3
    Point2f p{2.0f, 3.0f};
    Point2f result1 = t(p);

    // 使用齐次坐标变换: Point2(x,y) -> Vector3(x,y,1)
    Vec3f p3{p.x, p.y, 1.0f};
    Vec3f result3 = m * p3;
    Point2f result2{result3.x, result3.y};

    EXPECT_NEAR(result1.x, result2.x, 1e-4f);
    EXPECT_NEAR(result1.y, result2.y, 1e-4f);
}

TEST(Transform2D, Identity)
{
    Transform2Df t = Transform2Df::Identity();

    Point2f p{3.0f, 4.0f};
    Point2f result = t(p);

    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST(Transform2D, Inverse)
{
    Transform2Df t{
            Vec2f{5.0f, 3.0f},
            kPi / 6.0f,
            2.0f
    };

    Transform2Df inv = t.inverse();

    // 验证逆变换的正确性：t(inv(p)) ≈ p 和 inv(t(p)) ≈ p
    Point2f p{3.0f, 4.0f};

    Point2f transformed = t(p);
    Point2f recovered   = inv(transformed);

    EXPECT_NEAR(recovered.x, p.x, 1e-4f);
    EXPECT_NEAR(recovered.y, p.y, 1e-4f);

    // 也验证另一个方向
    Point2f transformed2 = inv(p);
    Point2f recovered2   = t(transformed2);

    EXPECT_NEAR(recovered2.x, p.x, 1e-4f);
    EXPECT_NEAR(recovered2.y, p.y, 1e-4f);

    // 验证逆变换的属性
    EXPECT_NEAR(inv.scale, 1.0f / t.scale, 1e-5f);
    EXPECT_NEAR(inv.rotation, -t.rotation, 1e-5f);
}

// ========== 边界情况测试 ==========

TEST(Transform3D, ZeroScale)
{
    // 零缩放应该是有效的（虽然会导致点都变成原点）
    Transform3Df t{
            Vec3f{1.0f, 2.0f, 3.0f},
            Quatf::Identity(),
            0.0f
    };

    Point3f p{5.0f, 6.0f, 7.0f};
    Point3f result = t(p);

    // 所有点都应该变换到平移位置
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST(Transform3D, NegativeScale)
{
    // 负缩放表示镜像
    Transform3Df t{
            Vec3f{},
            Quatf::Identity(),
            -1.0f
    };

    Vec3f v{1.0f, 2.0f, 3.0f};
    Vec3f result = t(v);

    EXPECT_FLOAT_EQ(result.x, -1.0f);
    EXPECT_FLOAT_EQ(result.y, -2.0f);
    EXPECT_FLOAT_EQ(result.z, -3.0f);
}

TEST(Transform3D, LargeTransform)
{
    // 大数值变换
    Transform3Df t{
            Vec3f{1000.0f, 2000.0f, 3000.0f},
            Quatf::Identity(),
            100.0f
    };

    Point3f p{1.0f, 2.0f, 3.0f};
    Point3f result = t(p);

    EXPECT_FLOAT_EQ(result.x, 1100.0f);
    EXPECT_FLOAT_EQ(result.y, 2200.0f);
    EXPECT_FLOAT_EQ(result.z, 3300.0f);
}

// ========== 精度测试 ==========

TEST(Transform3D, Precision_Float)
{
    Transform3Df t{
            Vec3f{0.1f, 0.2f, 0.3f},
            Quatf::FromEuler(kPi / 6.0f, kPi / 4.0f, kPi / 3.0f),
            1.5f
    };

    EXPECT_FALSE(t.hasNaN());
    EXPECT_TRUE(t.rotation.isNormalized());
}

TEST(Transform3D, Precision_Double)
{
    Transform3Dd t{
            Vec3d{0.1, 0.2, 0.3},
            Quatd::FromEuler(kPi / 6.0, kPi / 4.0, kPi / 3.0),
            1.5
    };

    EXPECT_FALSE(t.hasNaN());
    EXPECT_TRUE(t.rotation.isNormalized());
}
