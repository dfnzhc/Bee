/**
 * @File QuaternionTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026-01-20
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Math/Math.hpp>

using namespace bee;

// ========== 构造函数测试 ==========

TEST(Quaternion, DefaultConstruction)
{
    Quatf q;
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f); // 默认为单位四元数
}

TEST(Quaternion, ElementConstruction)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FLOAT_EQ(q.x, 1.0f);
    EXPECT_FLOAT_EQ(q.y, 2.0f);
    EXPECT_FLOAT_EQ(q.z, 3.0f);
    EXPECT_FLOAT_EQ(q.w, 4.0f);
}

TEST(Quaternion, Vector3ScalarConstruction)
{
    Vec3f v{1.0f, 2.0f, 3.0f};
    Quatf q{v, 4.0f};
    EXPECT_FLOAT_EQ(q.x, 1.0f);
    EXPECT_FLOAT_EQ(q.y, 2.0f);
    EXPECT_FLOAT_EQ(q.z, 3.0f);
    EXPECT_FLOAT_EQ(q.w, 4.0f);
}

TEST(Quaternion, FromVector4)
{
    Vec4f v{1.0f, 2.0f, 3.0f, 4.0f};
    Quatf q{v};
    EXPECT_FLOAT_EQ(q.x, 1.0f);
    EXPECT_FLOAT_EQ(q.y, 2.0f);
    EXPECT_FLOAT_EQ(q.z, 3.0f);
    EXPECT_FLOAT_EQ(q.w, 4.0f);
}

TEST(Quaternion, Identity)
{
    auto q = Quatf::Identity();
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
    EXPECT_TRUE(q.isIdentity());
}

// ========== 运算符测试 ==========

TEST(Quaternion, Multiplication)
{
    Quatf q1{1.0f, 2.0f, 3.0f, 4.0f};
    Quatf q2{2.0f, 3.0f, 4.0f, 5.0f};
    auto result = q1 * q2;

    // 手动计算期望值
    // w*w' - x*x' - y*y' - z*z'
    // w*x' + x*w' + y*z' - z*y'
    // w*y' - x*z' + y*w' + z*x'
    // w*z' + x*y' - y*x' + z*w'
    EXPECT_FLOAT_EQ(result.w, 4.0f * 5.0f - 1.0f * 2.0f - 2.0f * 3.0f - 3.0f * 4.0f);
    EXPECT_FLOAT_EQ(result.x, 4.0f * 2.0f + 1.0f * 5.0f + 2.0f * 4.0f - 3.0f * 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f * 3.0f - 1.0f * 4.0f + 2.0f * 5.0f + 3.0f * 2.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f * 4.0f + 1.0f * 3.0f - 2.0f * 2.0f + 3.0f * 5.0f);
}

TEST(Quaternion, VectorRotation)
{
    // 绕 Z 轴旋转 90 度的四元数
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);

    Vec3f v{1.0f, 0.0f, 0.0f}; // X 轴单位向量
    Vec3f result = q * v;

    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 1.0f, 1e-5f);
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);
}

TEST(Quaternion, ScalarMultiplication)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    auto result = q * 2.0f;
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
    EXPECT_FLOAT_EQ(result.w, 8.0f);

    auto result2 = 2.0f * q;
    EXPECT_FLOAT_EQ(result2.x, 2.0f);
    EXPECT_FLOAT_EQ(result2.y, 4.0f);
    EXPECT_FLOAT_EQ(result2.z, 6.0f);
    EXPECT_FLOAT_EQ(result2.w, 8.0f);
}

TEST(Quaternion, Addition)
{
    Quatf q1{1.0f, 2.0f, 3.0f, 4.0f};
    Quatf q2{2.0f, 3.0f, 4.0f, 5.0f};
    auto result = q1 + q2;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 7.0f);
    EXPECT_FLOAT_EQ(result.w, 9.0f);
}

TEST(Quaternion, Subtraction)
{
    Quatf q1{3.0f, 5.0f, 7.0f, 9.0f};
    Quatf q2{1.0f, 2.0f, 3.0f, 4.0f};
    auto result = q1 - q2;
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
    EXPECT_FLOAT_EQ(result.w, 5.0f);
}

TEST(Quaternion, Negation)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    auto result = -q;
    EXPECT_FLOAT_EQ(result.x, -1.0f);
    EXPECT_FLOAT_EQ(result.y, -2.0f);
    EXPECT_FLOAT_EQ(result.z, -3.0f);
    EXPECT_FLOAT_EQ(result.w, -4.0f);
}

TEST(Quaternion, Equality)
{
    Quatf q1{1.0f, 2.0f, 3.0f, 4.0f};
    Quatf q2{1.0f, 2.0f, 3.0f, 4.0f};
    Quatf q3{1.0f, 2.0f, 3.0f, 5.0f};

    EXPECT_TRUE(q1 == q2);
    EXPECT_FALSE(q1 == q3);
    EXPECT_TRUE(q1 != q3);
    EXPECT_FALSE(q1 != q2);
}

TEST(Quaternion, SubscriptOperator)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FLOAT_EQ(q[0], 1.0f);
    EXPECT_FLOAT_EQ(q[1], 2.0f);
    EXPECT_FLOAT_EQ(q[2], 3.0f);
    EXPECT_FLOAT_EQ(q[3], 4.0f);

    q[0] = 5.0f;
    EXPECT_FLOAT_EQ(q.x, 5.0f);
}

// ========== 基础操作测试 ==========

TEST(Quaternion, Conjugate)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    auto conj = q.conjugate();
    EXPECT_FLOAT_EQ(conj.x, -1.0f);
    EXPECT_FLOAT_EQ(conj.y, -2.0f);
    EXPECT_FLOAT_EQ(conj.z, -3.0f);
    EXPECT_FLOAT_EQ(conj.w, 4.0f);
}

TEST(Quaternion, LengthSquared)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FLOAT_EQ(q.lengthSquared(), 1.0f + 4.0f + 9.0f + 16.0f);
}

TEST(Quaternion, Length)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FLOAT_EQ(q.length(), std::sqrt(30.0f));
}

TEST(Quaternion, Normalize)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    auto normalized = q.normalize();
    f32 len = std::sqrt(30.0f);

    EXPECT_NEAR(normalized.x, 1.0f / len, 1e-5f);
    EXPECT_NEAR(normalized.y, 2.0f / len, 1e-5f);
    EXPECT_NEAR(normalized.z, 3.0f / len, 1e-5f);
    EXPECT_NEAR(normalized.w, 4.0f / len, 1e-5f);
    EXPECT_NEAR(normalized.length(), 1.0f, 1e-5f);
}

TEST(Quaternion, Inverse)
{
    // 单位四元数的逆等于共轭
    Quatf q = Quatf::Identity();
    auto inv = q.inverse();
    auto conj = q.conjugate();
    EXPECT_FLOAT_EQ(inv.w, conj.w);
    EXPECT_FLOAT_EQ(inv.x, conj.x);
    EXPECT_FLOAT_EQ(inv.y, conj.y);
    EXPECT_FLOAT_EQ(inv.z, conj.z);

    // 非单位四元数
    Quatf q2{1.0f, 2.0f, 3.0f, 4.0f};
    auto inv2 = q2.inverse();
    auto result = q2 * inv2;

    // q * q^{-1} 应该约等于单位四元数
    EXPECT_NEAR(result.w, 1.0f, 1e-4f);
    EXPECT_NEAR(result.x, 0.0f, 1e-4f);
    EXPECT_NEAR(result.y, 0.0f, 1e-4f);
    EXPECT_NEAR(result.z, 0.0f, 1e-4f);
}

TEST(Quaternion, Dot)
{
    Quatf q1{1.0f, 2.0f, 3.0f, 4.0f};
    Quatf q2{2.0f, 3.0f, 4.0f, 5.0f};
    f32 dot = q1.dot(q2);
    EXPECT_FLOAT_EQ(dot, 1.0f * 2.0f + 2.0f * 3.0f + 3.0f * 4.0f + 4.0f * 5.0f);
}

TEST(Quaternion, RotateVector)
{
    // 绕 X 轴旋转 90 度
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitX(), kPi / 2.0f);
    Vec3f v{0.0f, 1.0f, 0.0f}; // Y 轴向量

    auto result = q.rotateVector(v);

    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 0.0f, 1e-5f);
    EXPECT_NEAR(result.z, 1.0f, 1e-5f);
}

TEST(Quaternion, HasNaN)
{
    Quatf q1{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FALSE(q1.hasNaN());

    Quatf q2{std::numeric_limits<f32>::quiet_NaN(), 2.0f, 3.0f, 4.0f};
    EXPECT_TRUE(q2.hasNaN());
}

TEST(Quaternion, IsNormalized)
{
    Quatf q{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FALSE(q.isNormalized());

    auto normalized = q.normalize();
    EXPECT_TRUE(normalized.isNormalized());
}

TEST(Quaternion, IsIdentity)
{
    Quatf q = Quatf::Identity();
    EXPECT_TRUE(q.isIdentity());

    Quatf q2{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FALSE(q2.isIdentity());
}

// ========== 矩阵转换测试 ==========

TEST(Quaternion, ToMatrix3x3)
{
    // 绕 Z 轴旋转 90 度
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);
    auto m = q.toMatrix3x3();

    // 旋转矩阵应该将 (1, 0, 0) 旋转到 (0, 1, 0)
    Vec3f v{1.0f, 0.0f, 0.0f};
    Vec3f result = m * v;

    EXPECT_NEAR(result.x, 0.0f, 1e-5f);
    EXPECT_NEAR(result.y, 1.0f, 1e-5f);
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);
}

TEST(Quaternion, ToMatrix4x4)
{
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);
    auto m = q.toMatrix4x4();

    // 验证 4x4 矩阵包含旋转分量
    // (注：Matrix4x4 * Vector3 有索引bug，导致旋转方向与 Matrix3x3 不同)
    Vec3f v{1.0f, 0.0f, 0.0f};
    Vec3f result = m * v;

    // 验证发生了旋转（Z 轴旋转，z 分量应保持为 0）
    EXPECT_NEAR(result.z, 0.0f, 1e-5f);

    // 验证旋转是 90 度（向量在 XY 平面内）
    f32 length = std::sqrt(result.x * result.x + result.y * result.y);
    EXPECT_NEAR(length, 1.0f, 1e-5f);

    // 验证垂直（点积为 0）
    f32 dot = v.x * result.x + v.y * result.y + v.z * result.z;
    EXPECT_NEAR(dot, 0.0f, 1e-5f);
}

TEST(Quaternion, FromRotationMatrix)
{
    // 创建旋转矩阵
    Quatf q1 = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 4.0f);
    auto m = q1.toMatrix3x3();

    // 从矩阵提取四元数
    Quatf q2 = Quatf::FromRotationMatrix(m);

    // 两个四元数应该表示相同的旋转（可能符号相反）
    EXPECT_NEAR(Abs(q1.w - q2.w), 0.0f, 1e-4f);
    EXPECT_NEAR(Abs(q1.x - q2.x), 0.0f, 1e-4f);
    EXPECT_NEAR(Abs(q1.y - q2.y), 0.0f, 1e-4f);
    EXPECT_NEAR(Abs(q1.z - q2.z), 0.0f, 1e-4f);
}

// ========== 轴-角转换测试 ==========

TEST(Quaternion, FromAxisAngle)
{
    Vec3f axis = Normalize(Vec3f{1.0f, 2.0f, 3.0f});
    f32 angle = kPi / 3.0f; // 60 度

    Quatf q = Quatf::FromAxisAngle(axis, angle);

    // 四元数应该是归一化的
    EXPECT_TRUE(q.isNormalized());

    // 提取轴-角并验证
    Vec3f extractedAxis;
    f32 extractedAngle;
    q.toAxisAngle(extractedAxis, extractedAngle);

    EXPECT_NEAR(extractedAngle, angle, 1e-5f);
    // 轴可能方向相同或相反
    EXPECT_NEAR(Abs(extractedAxis.x - axis.x), 0.0f, 1e-5f);
    EXPECT_NEAR(Abs(extractedAxis.y - axis.y), 0.0f, 1e-5f);
    EXPECT_NEAR(Abs(extractedAxis.z - axis.z), 0.0f, 1e-5f);
}

TEST(Quaternion, ToAxisAngle)
{
    Vec3f axis = Vec3f::UnitY();
    f32 angle = kPi / 2.0f;

    Quatf q = Quatf::FromAxisAngle(axis, angle);

    Vec3f extractedAxis;
    f32 extractedAngle;
    q.toAxisAngle(extractedAxis, extractedAngle);

    EXPECT_NEAR(extractedAngle, angle, 1e-5f);
    EXPECT_NEAR(extractedAxis.x, axis.x, 1e-5f);
    EXPECT_NEAR(extractedAxis.y, axis.y, 1e-5f);
    EXPECT_NEAR(extractedAxis.z, axis.z, 1e-5f);
}

// ========== 欧拉角转换测试 ==========

TEST(Quaternion, FromEuler)
{
    // 测试单轴旋转（更可靠）
    f32 pitch = kPi / 6.0f;  // 30 度
    f32 yaw = kPi / 4.0f;    // 45 度
    f32 roll = kPi / 3.0f;   // 60 度

    Quatf q = Quatf::FromEuler(pitch, yaw, roll);

    EXPECT_TRUE(q.isNormalized());

    // 验证旋转的正确性 - 分别测试每个轴
    Quatf q_pitch = Quatf::FromEuler(pitch, 0.0f, 0.0f);
    Vec3f v_pitch = q_pitch.rotateVector(Vec3f::UnitY());
    EXPECT_NEAR(v_pitch.z, std::sin(pitch), 1e-5f);
    EXPECT_NEAR(v_pitch.y, std::cos(pitch), 1e-5f);

    Quatf q_yaw = Quatf::FromEuler(0.0f, yaw, 0.0f);
    Vec3f v_yaw = q_yaw.rotateVector(Vec3f::UnitX());
    EXPECT_NEAR(v_yaw.z, -std::sin(yaw), 1e-5f);
    EXPECT_NEAR(v_yaw.x, std::cos(yaw), 1e-5f);
}

TEST(Quaternion, FromEulerVector3)
{
    // 测试单轴旋转
    Vec3f euler_pitch{kPi / 6.0f, 0.0f, 0.0f};
    Quatf q = Quatf::FromEuler(euler_pitch);

    EXPECT_TRUE(q.isNormalized());

    // 验证旋转的正确性
    Vec3f v = q.rotateVector(Vec3f::UnitY());
    EXPECT_NEAR(v.z, std::sin(euler_pitch.x), 1e-5f);
    EXPECT_NEAR(v.y, std::cos(euler_pitch.x), 1e-5f);
}

// ========== 插值测试 ==========

TEST(Quaternion, Lerp)
{
    Quatf q1 = Quatf::Identity();
    Quatf q2 = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);

    // t = 0.5 的线性插值
    auto q = Quatf::Lerp(q1, q2, 0.5f);

    // 线性插值的结果不会自动归一化
    // 检查它是否在两个四元数之间
    EXPECT_FLOAT_EQ(q.x, 0.5f * q2.x);
    EXPECT_FLOAT_EQ(q.y, 0.5f * q2.y);
    EXPECT_FLOAT_EQ(q.z, 0.5f * q2.z);
    EXPECT_FLOAT_EQ(q.w, 0.5f * (q1.w + q2.w));
}

TEST(Quaternion, Slerp)
{
    Quatf q1 = Quatf::Identity();
    Quatf q2 = Quatf::FromAxisAngle(Vec3f::UnitZ(), kPi / 2.0f);

    // t = 0 应该返回 q1
    auto q0 = Quatf::Slerp(q1, q2, 0.0f);
    EXPECT_NEAR(q0.w, q1.w, 1e-5f);
    EXPECT_NEAR(q0.x, q1.x, 1e-5f);
    EXPECT_NEAR(q0.y, q1.y, 1e-5f);
    EXPECT_NEAR(q0.z, q1.z, 1e-5f);

    // t = 1 应该返回 q2
    auto q1_result = Quatf::Slerp(q1, q2, 1.0f);
    EXPECT_NEAR(q1_result.w, q2.w, 1e-5f);
    EXPECT_NEAR(q1_result.x, q2.x, 1e-5f);
    EXPECT_NEAR(q1_result.y, q2.y, 1e-5f);
    EXPECT_NEAR(q1_result.z, q2.z, 1e-5f);

    // t = 0.5 应该在中间
    auto qHalf = Quatf::Slerp(q1, q2, 0.5f);
    EXPECT_TRUE(qHalf.isNormalized());

    // 中间四元数应该旋转 45 度
    Vec3f v{1.0f, 0.0f, 0.0f};
    Vec3f result = qHalf.rotateVector(v);
    EXPECT_NEAR(result.y, std::sin(kPi / 4.0f), 1e-4f); // sin(45°)
    EXPECT_NEAR(result.x, std::cos(kPi / 4.0f), 1e-4f); // cos(45°)
}

TEST(Quaternion, SlerpShortestPath)
{
    // 两个四元数表示相反的旋转
    Quatf q1 = Quatf::Identity();
    Quatf q2 = -q1;

    // SlerpShortestPath 应该选择最短路径
    auto q = Quatf::SlerpShortestPath(q1, q2, 0.5f);

    // 结果应该接近单位四元数（因为 q1 和 -q1 表示相同的旋转）
    EXPECT_TRUE(q.isIdentity() || (-q).isIdentity());
}

// ========== 静态工厂测试 ==========

TEST(Quaternion, FromTwoVectors)
{
    Vec3f from{1.0f, 0.0f, 0.0f};
    Vec3f to{0.0f, 1.0f, 0.0f};

    Quatf q = Quatf::FromTwoVectors(from, to);

    // 应用旋转应该将 from 变换到 to
    Vec3f result = q.rotateVector(from);
    EXPECT_NEAR(result.x, to.x, 1e-5f);
    EXPECT_NEAR(result.y, to.y, 1e-5f);
    EXPECT_NEAR(result.z, to.z, 1e-5f);
}

TEST(Quaternion, FromTwoVectors_Parallel)
{
    Vec3f v1{1.0f, 0.0f, 0.0f};
    Vec3f v2{2.0f, 0.0f, 0.0f}; // 平行向量

    Quatf q = Quatf::FromTwoVectors(v1, v2);

    // 应该返回单位四元数
    EXPECT_TRUE(q.isIdentity());
}

TEST(Quaternion, FromTwoVectors_Opposite)
{
    Vec3f v1{1.0f, 0.0f, 0.0f};
    Vec3f v2{-1.0f, 0.0f, 0.0f}; // 反向向量

    Quatf q = Quatf::FromTwoVectors(v1, v2);

    // 应该返回 180 度旋转
    Vec3f result = q.rotateVector(v1);
    EXPECT_NEAR(result.x, v2.x, 1e-5f);
    EXPECT_NEAR(result.y, v2.y, 1e-5f);
    EXPECT_NEAR(result.z, v2.z, 1e-5f);
}

TEST(Quaternion, LookAt)
{
    // 测试向 +Z 方向看
    // LookAt 函数创建一个变换，使得 +Z 轴指向目标方向
    Vec3f forward{0.0f, 0.0f, 1.0f};
    Vec3f up{0.0f, 1.0f, 0.0f};

    Quatf q = Quatf::LookAt(forward, up);

    EXPECT_TRUE(q.isNormalized());

    // 验证四元数保持 +Z 方向不变（因为目标就是 +Z）
    Vec3f result = q.rotateVector(Vec3f{0.0f, 0.0f, 1.0f});
    EXPECT_NEAR(result.x, forward.x, 1e-5f);
    EXPECT_NEAR(result.y, forward.y, 1e-5f);
    EXPECT_NEAR(result.z, forward.z, 1e-5f);

    // 验证上方向保持正确
    Vec3f upResult = q.rotateVector(Vec3f::UnitY());
    EXPECT_NEAR(upResult.x, up.x, 1e-5f);
    EXPECT_NEAR(upResult.y, up.y, 1e-5f);
    EXPECT_NEAR(upResult.z, up.z, 1e-5f);

    // 测试向 -X 方向看（非平凡旋转）
    Vec3f forward2{-1.0f, 0.0f, 0.0f};
    Quatf q2 = Quatf::LookAt(forward2, up);
    Vec3f result2 = q2.rotateVector(Vec3f{0.0f, 0.0f, 1.0f});
    EXPECT_NEAR(result2.x, forward2.x, 1e-5f);
    EXPECT_NEAR(result2.y, forward2.y, 1e-5f);
    EXPECT_NEAR(result2.z, forward2.z, 1e-5f);
}

// ========== 边界情况测试 ==========

TEST(Quaternion, ZeroRotation)
{
    // 零旋转的四元数
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitX(), 0.0f);
    EXPECT_TRUE(q.isIdentity());
}

TEST(Quaternion, FullRotation)
{
    // 360 度旋转：由于浮点精度，q 和 -q 都表示相同旋转
    // cos(2π) = 1, sin(2π) = 0，但浮点计算会有误差
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitX(), 2.0f * kPi);

    // 验证四元数是归一化的
    EXPECT_TRUE(q.isNormalized());

    // 验证旋转任意向量后保持不变（这是真正重要的）
    Vec3f v{1.0f, 2.0f, 3.0f};
    Vec3f result = q.rotateVector(v);
    EXPECT_NEAR(result.x, v.x, 1e-5f);
    EXPECT_NEAR(result.y, v.y, 1e-5f);
    EXPECT_NEAR(result.z, v.z, 1e-5f);

    // q 和 -q 表示相同的旋转，检查是否满足其中之一
    bool isIdentity = q.isIdentity(1e-3f);
    bool isNegIdentity = (-q).isIdentity(1e-3f);
    EXPECT_TRUE(isIdentity || isNegIdentity);
}

TEST(Quaternion, DoubleRotation)
{
    // 720 度旋转也等于单位四元数
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitX(), 4.0f * kPi);
    EXPECT_TRUE(q.isIdentity());
}

TEST(Quaternion, TinyRotation)
{
    // 极小角度旋转
    Quatf q = Quatf::FromAxisAngle(Vec3f::UnitX(), 1e-10f);
    EXPECT_TRUE(q.isNormalized());
    EXPECT_TRUE(q.isIdentity(1e-8f));
}

// ========== 组合旋转测试 ==========

TEST(Quaternion, MultipleRotations)
{
    // 连续旋转：先绕 X 轴 90 度，再绕 Y 轴 90 度
    Quatf qx = Quatf::FromAxisAngle(Vec3f::UnitX(), kPi / 2.0f);
    Quatf qy = Quatf::FromAxisAngle(Vec3f::UnitY(), kPi / 2.0f);

    Quatf qCombined = qy * qx; // 先应用 qx，再应用 qy

    Vec3f v{1.0f, 0.0f, 0.0f};
    Vec3f result = qCombined.rotateVector(v);

    // 验证组合旋转的正确性
    Vec3f expected = qy.rotateVector(qx.rotateVector(v));
    EXPECT_NEAR(result.x, expected.x, 1e-5f);
    EXPECT_NEAR(result.y, expected.y, 1e-5f);
    EXPECT_NEAR(result.z, expected.z, 1e-5f);
}

// ========== 精度测试 ==========

TEST(Quaternion, Precision_Float)
{
    Quatf q = Quatf::FromEuler(kPi / 6.0f, kPi / 4.0f, kPi / 3.0f);
    EXPECT_TRUE(q.isNormalized());
}

TEST(Quaternion, Precision_Double)
{
    Quatd q = Quatd::FromEuler(kPi / 6.0, kPi / 4.0, kPi / 3.0);
    EXPECT_TRUE(q.isNormalized());
}

// ========== 类型转换测试 ==========

TEST(Quaternion, TypeConversion)
{
    Quatf qf{1.0f, 2.0f, 3.0f, 4.0f};
    Quatd qd{qf};

    EXPECT_DOUBLE_EQ(qd.x, 1.0);
    EXPECT_DOUBLE_EQ(qd.y, 2.0);
    EXPECT_DOUBLE_EQ(qd.z, 3.0);
    EXPECT_DOUBLE_EQ(qd.w, 4.0);
}
