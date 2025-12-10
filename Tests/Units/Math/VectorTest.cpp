/**
 * @File VectorTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/10
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Math/Vector.hpp>

using namespace bee;

TEST(VectorType1_Vector2_Basic, ConstructAndOps)
{
    Vec2f a{1.0f, -2.0f};
    Vec2f b{3.0f, 4.0f};
    auto c = a + b;
    EXPECT_FLOAT_EQ(c.x(), 4.0f);
    EXPECT_FLOAT_EQ(c.y(), 2.0f);
    auto d = b - a;
    EXPECT_FLOAT_EQ(d.x(), 2.0f);
    EXPECT_FLOAT_EQ(d.y(), 6.0f);
    auto m = a * 2.0f;
    EXPECT_FLOAT_EQ(m.x(), 2.0f);
    EXPECT_FLOAT_EQ(m.y(), -4.0f);
    auto ml = 2.0f * a;
    EXPECT_FLOAT_EQ(ml.x(), 2.0f);
    EXPECT_FLOAT_EQ(ml.y(), -4.0f);
    auto div = b / 2.0f;
    EXPECT_FLOAT_EQ(div.x(), 1.5f);
    EXPECT_FLOAT_EQ(div.y(), 2.0f);
    auto z = Vec2f::Zero();
    EXPECT_FLOAT_EQ(z.x(), 0.0f);
    EXPECT_FLOAT_EQ(z.y(), 0.0f);
    auto o = Vec2f::One();
    EXPECT_FLOAT_EQ(o.x(), 1.0f);
    EXPECT_FLOAT_EQ(o.y(), 1.0f);
    auto ux = Vec2f::UnitX();
    EXPECT_FLOAT_EQ(ux.x(), 1.0f);
    EXPECT_FLOAT_EQ(ux.y(), 0.0f);
    auto uy = Vec2f::UnitY();
    EXPECT_FLOAT_EQ(uy.x(), 0.0f);
    EXPECT_FLOAT_EQ(uy.y(), 1.0f);
}

TEST(VectorType1_Vector2_Swizzle, Swizzle2D3D4D)
{
    Vec2i a{2, -1};
    auto xx = a.xx();
    EXPECT_EQ(xx.x(), 2);
    EXPECT_EQ(xx.y(), 2);
    auto xy = a.xy();
    EXPECT_EQ(xy.x(), 2);
    EXPECT_EQ(xy.y(), -1);
    auto yx = a.yx();
    EXPECT_EQ(yx.x(), -1);
    EXPECT_EQ(yx.y(), 2);
    auto yy = a.yy();
    EXPECT_EQ(yy.x(), -1);
    EXPECT_EQ(yy.y(), -1);
    auto xxx = a.xxx();
    EXPECT_EQ(xxx.x(), 2);
    EXPECT_EQ(xxx.y(), 2);
    EXPECT_EQ(xxx.z(), 2);
    auto xyy = a.xyy();
    EXPECT_EQ(xyy.x(), 2);
    EXPECT_EQ(xyy.y(), -1);
    EXPECT_EQ(xyy.z(), -1);
    auto xyxy = a.xyxy();
    EXPECT_EQ(xyxy.x(), 2);
    EXPECT_EQ(xyxy.y(), -1);
    EXPECT_EQ(xyxy.z(), 2);
    EXPECT_EQ(xyxy.w(), -1);
}

TEST(VectorType1_Vector3_Constructors, FromMixed)
{
    Vec2f v2{1.0f, 2.0f};
    Vec3f v3a{v2, 3.0f};
    EXPECT_FLOAT_EQ(v3a.x(), 1.0f);
    EXPECT_FLOAT_EQ(v3a.y(), 2.0f);
    EXPECT_FLOAT_EQ(v3a.z(), 3.0f);
    Vec3f v3b{4.0f, Vec2f{5.0f, 6.0f}};
    EXPECT_FLOAT_EQ(v3b.x(), 4.0f);
    EXPECT_FLOAT_EQ(v3b.y(), 5.0f);
    EXPECT_FLOAT_EQ(v3b.z(), 6.0f);
    Vec3f v3c{Vec2f{7.0f, 8.0f}};
    EXPECT_FLOAT_EQ(v3c.x(), 7.0f);
    EXPECT_FLOAT_EQ(v3c.y(), 8.0f);
    EXPECT_FLOAT_EQ(v3c.z(), 0.0f);
    Vec3d v3d{Vec4f{1.0f, 2.0f, 3.0f, 4.0f}};
    EXPECT_DOUBLE_EQ(v3d.x(), 1.0);
    EXPECT_DOUBLE_EQ(v3d.y(), 2.0);
    EXPECT_DOUBLE_EQ(v3d.z(), 3.0);
}

TEST(VectorType1_Vector3_Swizzle, Swizzle)
{
    Vec3i v{1, 2, 3};
    auto zz = v.zz();
    EXPECT_EQ(zz.x(), 3);
    EXPECT_EQ(zz.y(), 3);
    auto xyz = v.xyz();
    EXPECT_EQ(xyz.x(), 1);
    EXPECT_EQ(xyz.y(), 2);
    EXPECT_EQ(xyz.z(), 3);
    auto zxy = v.zxy();
    EXPECT_EQ(zxy.x(), 3);
    EXPECT_EQ(zxy.y(), 1);
    EXPECT_EQ(zxy.z(), 2);
}

TEST(VectorType1_Vector4_Constructors, FromMixed)
{
    Vec2f v2{1.0f, 2.0f};
    Vec3f v3{3.0f, 4.0f, 5.0f};
    Vec4f v4a{v2, 6.0f, 7.0f};
    EXPECT_FLOAT_EQ(v4a.x(), 1.0f);
    EXPECT_FLOAT_EQ(v4a.y(), 2.0f);
    EXPECT_FLOAT_EQ(v4a.z(), 6.0f);
    EXPECT_FLOAT_EQ(v4a.w(), 7.0f);
    Vec4f v4b{8.0f, v2, 9.0f};
    EXPECT_FLOAT_EQ(v4b.x(), 8.0f);
    EXPECT_FLOAT_EQ(v4b.y(), 1.0f);
    EXPECT_FLOAT_EQ(v4b.z(), 2.0f);
    EXPECT_FLOAT_EQ(v4b.w(), 9.0f);
    Vec4f v4c{10.0f, 11.0f, v2};
    EXPECT_FLOAT_EQ(v4c.x(), 10.0f);
    EXPECT_FLOAT_EQ(v4c.y(), 11.0f);
    EXPECT_FLOAT_EQ(v4c.z(), 1.0f);
    EXPECT_FLOAT_EQ(v4c.w(), 2.0f);
    Vec4f v4d{v3, 12.0f};
    EXPECT_FLOAT_EQ(v4d.x(), 3.0f);
    EXPECT_FLOAT_EQ(v4d.y(), 4.0f);
    EXPECT_FLOAT_EQ(v4d.z(), 5.0f);
    EXPECT_FLOAT_EQ(v4d.w(), 12.0f);
    Vec4f v4e{13.0f, v3};
    EXPECT_FLOAT_EQ(v4e.x(), 13.0f);
    EXPECT_FLOAT_EQ(v4e.y(), 3.0f);
    EXPECT_FLOAT_EQ(v4e.z(), 4.0f);
    EXPECT_FLOAT_EQ(v4e.w(), 5.0f);
    Vec4f v4f{v2, v2};
    EXPECT_FLOAT_EQ(v4f.x(), 1.0f);
    EXPECT_FLOAT_EQ(v4f.y(), 2.0f);
    EXPECT_FLOAT_EQ(v4f.z(), 1.0f);
    EXPECT_FLOAT_EQ(v4f.w(), 2.0f);
    Vec4f v4g{Vec2f{1.0f, 2.0f}};
    EXPECT_FLOAT_EQ(v4g.x(), 1.0f);
    EXPECT_FLOAT_EQ(v4g.y(), 2.0f);
    EXPECT_FLOAT_EQ(v4g.z(), 0.0f);
    EXPECT_FLOAT_EQ(v4g.w(), 0.0f);
    Vec4f v4h{v3};
    EXPECT_FLOAT_EQ(v4h.x(), 3.0f);
    EXPECT_FLOAT_EQ(v4h.y(), 4.0f);
    EXPECT_FLOAT_EQ(v4h.z(), 5.0f);
    EXPECT_FLOAT_EQ(v4h.w(), 0.0f);
    Vec4f v4i{Vec4f{1.0f, 2.0f, 3.0f, 4.0f}};
    EXPECT_FLOAT_EQ(v4i.x(), 1.0f);
    EXPECT_FLOAT_EQ(v4i.y(), 2.0f);
    EXPECT_FLOAT_EQ(v4i.z(), 3.0f);
    EXPECT_FLOAT_EQ(v4i.w(), 4.0f);
}

TEST(VectorType1_Vector4_Swizzle, Swizzle)
{
    Vec4i v{1, 2, 3, 4};
    auto xy = v.xy();
    EXPECT_EQ(xy.x(), 1);
    EXPECT_EQ(xy.y(), 2);
    auto wz = v.wz();
    EXPECT_EQ(wz.x(), 4);
    EXPECT_EQ(wz.y(), 3);
    auto yzw = v.yzw();
    EXPECT_EQ(yzw.x(), 2);
    EXPECT_EQ(yzw.y(), 3);
    EXPECT_EQ(yzw.z(), 4);
    auto yyyy = v.yyyy();
    EXPECT_EQ(yyyy.x(), 2);
    EXPECT_EQ(yyyy.y(), 2);
    EXPECT_EQ(yyyy.z(), 2);
    EXPECT_EQ(yyyy.w(), 2);
}

TEST(VectorType1_Point2_Ops, PointVectorOps)
{
    Point2f p{1.0f, 2.0f};
    auto p0 = Point2f::Origin();
    EXPECT_FLOAT_EQ(p0.x(), 0.0f);
    EXPECT_FLOAT_EQ(p0.y(), 0.0f);
    Vec2f v{3.0f, -1.0f};
    auto p_add = p + v;
    EXPECT_FLOAT_EQ(p_add.x(), 4.0f);
    EXPECT_FLOAT_EQ(p_add.y(), 1.0f);
    auto p_sub = p - v;
    EXPECT_FLOAT_EQ(p_sub.x(), -2.0f);
    EXPECT_FLOAT_EQ(p_sub.y(), 3.0f);
    Point2f q{4.0f, -1.0f};
    auto d = p - q;
    EXPECT_FLOAT_EQ(d.x(), -3.0f);
    EXPECT_FLOAT_EQ(d.y(), 3.0f);
}

TEST(VectorType1_Point3_Ops, PointVectorOps)
{
    Point3f p{1.0f, 2.0f, 3.0f};
    auto p0 = Point3f::Origin();
    EXPECT_FLOAT_EQ(p0.x(), 0.0f);
    EXPECT_FLOAT_EQ(p0.y(), 0.0f);
    EXPECT_FLOAT_EQ(p0.z(), 0.0f);
    Vec3f v{3.0f, -1.0f, 2.0f};
    auto p_add = p + v;
    EXPECT_FLOAT_EQ(p_add.x(), 4.0f);
    EXPECT_FLOAT_EQ(p_add.y(), 1.0f);
    EXPECT_FLOAT_EQ(p_add.z(), 5.0f);
    auto p_sub = p - v;
    EXPECT_FLOAT_EQ(p_sub.x(), -2.0f);
    EXPECT_FLOAT_EQ(p_sub.y(), 3.0f);
    EXPECT_FLOAT_EQ(p_sub.z(), 1.0f);
    Point3f q{4.0f, -1.0f, 0.0f};
    auto d = p - q;
    EXPECT_FLOAT_EQ(d.x(), -3.0f);
    EXPECT_FLOAT_EQ(d.y(), 3.0f);
    EXPECT_FLOAT_EQ(d.z(), 3.0f);
}

TEST(VectorType1_Normal2_Ops, NormalizeDotAbsDotFaceForward)
{
    Normal2f n{0.0f, 2.0f};
    auto nn = n.normalize();
    EXPECT_NEAR(Length(nn), 1.0f, 1e-6f);
    EXPECT_TRUE(nn.isNormalized());
    Vec2f v{3.0f, 4.0f};
    EXPECT_FLOAT_EQ(Dot(n, v), 8.0f);
    EXPECT_FLOAT_EQ(Dot(v, n), 8.0f);
    EXPECT_FLOAT_EQ(AbsDot(n, v), 8.0f);
    EXPECT_FLOAT_EQ(AbsDot(v, n), 8.0f);
    Normal2f n2{0.0f, -2.0f};
    auto ff1 = FaceForward(n, v);
    EXPECT_FLOAT_EQ(ff1.x(), (Dot(n, v) < 0.f) ? -n.x() : n.x());
    EXPECT_FLOAT_EQ(ff1.y(), (Dot(n, v) < 0.f) ? -n.y() : n.y());
    auto ff2 = FaceForward(n, n2);
    EXPECT_FLOAT_EQ(ff2.x(), (Dot(n, n2) < 0.f) ? -n.x() : n.x());
    EXPECT_FLOAT_EQ(ff2.y(), (Dot(n, n2) < 0.f) ? -n.y() : n.y());
}

TEST(VectorType1_Normal3_Ops, NormalizeDotAbsDotCrossFaceForward)
{
    Normal3f n{0.0f, 0.0f, 2.0f};
    auto nn = n.normalize();
    EXPECT_NEAR(Length(nn), 1.0f, 1e-6f);
    EXPECT_TRUE(nn.isNormalized());
    Vec3f v{1.0f, 2.0f, 3.0f};
    EXPECT_FLOAT_EQ(Dot(n, v), 6.0f);
    EXPECT_FLOAT_EQ(Dot(v, n), 6.0f);
    EXPECT_FLOAT_EQ(AbsDot(n, v), 6.0f);
    Vec3f cvn = Cross(v, n);
    EXPECT_FLOAT_EQ(cvn.x(), v.y() * n.z() - v.z() * n.y());
    EXPECT_FLOAT_EQ(cvn.y(), v.z() * n.x() - v.x() * n.z());
    EXPECT_FLOAT_EQ(cvn.z(), v.x() * n.y() - v.y() * n.x());
    Vec3f cnv = Cross(n, v);
    EXPECT_FLOAT_EQ(cnv.x(), n.y() * v.z() - n.z() * v.y());
    EXPECT_FLOAT_EQ(cnv.y(), n.z() * v.x() - n.x() * v.z());
    EXPECT_FLOAT_EQ(cnv.z(), n.x() * v.y() - n.y() * v.x());
    auto ff1 = FaceForward(n, v);
    EXPECT_FLOAT_EQ(ff1.z(), (Dot(n, v) < 0.f) ? -n.z() : n.z());
}

TEST(VectorType1_Vector_Normalize, Normalize2D3D4D)
{
    Vec2f v2{3.0f, 4.0f};
    auto n2 = Normalize(v2);
    EXPECT_NEAR(Length(n2), 1.0f, 1e-6f);
    EXPECT_NEAR(n2.x(), 0.6f, 1e-6f);
    EXPECT_NEAR(n2.y(), 0.8f, 1e-6f);

    Vec3f v3{1.0f, 2.0f, 2.0f};
    auto n3 = Normalize(v3);
    EXPECT_NEAR(Length(n3), 1.0f, 1e-6f);
    EXPECT_NEAR(n3.x(), 1.0f / 3.0f, 1e-6f);
    EXPECT_NEAR(n3.y(), 2.0f / 3.0f, 1e-6f);
    EXPECT_NEAR(n3.z(), 2.0f / 3.0f, 1e-6f);

    Vec4f v4{1.0f, 2.0f, 2.0f, 1.0f};
    auto n4 = Normalize(v4);
    EXPECT_NEAR(Length(n4), 1.0f, 1e-6f);
}

TEST(VectorType1_Point_Distance, Distance2D3D)
{
    Point2f p2{1.0f, 2.0f};
    Point2f q2{4.0f, -1.0f};
    EXPECT_NEAR(Distance(p2, q2), std::sqrt(18.0f), 1e-6f);
    EXPECT_NEAR(DistanceSquared(p2, q2), 18.0f, 1e-6f);

    Point3f p3{1.0f, 2.0f, 3.0f};
    Point3f q3{4.0f, -1.0f, 0.0f};
    EXPECT_NEAR(Distance(p3, q3), std::sqrt(27.0f), 1e-6f);
    EXPECT_NEAR(DistanceSquared(p3, q3), 27.0f, 1e-6f);
}

TEST(VectorType1_Vector_Angle, AngleBetween)
{
    Vec3f a{1.0f, 0.0f, 0.0f};
    Vec3f b{0.0f, 1.0f, 0.0f};
    auto ang = AngleBetween(a, b);
    EXPECT_NEAR(ang, PiOver2<f32>(), 1e-6f);

    Normal3f n1{1.0f, 0.0f, 0.0f};
    Normal3f n2{0.0f, 1.0f, 0.0f};
    auto angn = AngleBetween(n1, n2);
    EXPECT_NEAR(angn, PiOver2<f32>(), 1e-6f);
}

TEST(VectorType1_Vector_GramSchmidt, Orthogonalize)
{
    Vec3f v{1.0f, 1.0f, 0.0f};
    Vec3f w{0.0f, 1.0f, 0.0f};
    auto g = GramSchmidt(v, w);
    EXPECT_NEAR(g.x(), 1.0f, 1e-6f);
    EXPECT_NEAR(g.y(), 0.0f, 1e-6f);
    EXPECT_NEAR(g.z(), 0.0f, 1e-6f);
}

TEST(VectorType1_Vector_CoordinateSystem, BuildBasis)
{
    Vec3f v1{0.0f, 0.0f, 1.0f};
    Vec3f v2{}, v3{};
    CoordinateSystem(v1, &v2, &v3);
    EXPECT_NEAR(Dot(v1, v2), 0.0f, 1e-6f);
    EXPECT_NEAR(Dot(v1, v3), 0.0f, 1e-6f);
}

TEST(VectorType1_Vector_FaceForward, VectorWithVectorNormal)
{
    Vec3f v{1.0f, 0.0f, 0.0f};
    Vec3f v2{-1.0f, 0.0f, 0.0f};
    auto ffv = FaceForward(v, v2);
    EXPECT_NEAR(ffv.x(), -1.0f, 1e-6f);
    EXPECT_NEAR(ffv.y(), 0.0f, 1e-6f);
    EXPECT_NEAR(ffv.z(), 0.0f, 1e-6f);

    Normal3f n2{0.0f, -1.0f, 0.0f};
    auto ffvn = FaceForward(v, n2);
    EXPECT_NEAR(ffvn.x(), 1.0f, 1e-6f);
    EXPECT_NEAR(ffvn.y(), 0.0f, 1e-6f);
    EXPECT_NEAR(ffvn.z(), 0.0f, 1e-6f);
}

TEST(VectorMath_SphericalAngles, ThetaPhiBasic)
{
    Vector3<f32> zpos{0.f, 0.f, 1.f};
    Vector3<f32> zneg{0.f, 0.f, -1.f};
    Vector3<f32> xpos{1.f, 0.f, 0.f};
    Vector3<f32> ypos{0.f, 1.f, 0.f};

    EXPECT_NEAR(SphericalTheta(zpos), 0.f, 1e-6f);
    EXPECT_NEAR(SphericalTheta(zneg), Pi<f32>(), 1e-6f);
    EXPECT_NEAR(SphericalTheta(xpos), PiOver2<f32>(), 1e-6f);

    EXPECT_NEAR(SphericalPhi(xpos), 0.f, 1e-6f);
    EXPECT_NEAR(SphericalPhi(ypos), PiOver2<f32>(), 1e-6f);
    EXPECT_NEAR(SphericalPhi(Vector3<f32>{-1.f, 0.f, 0.f}), Pi<f32>(), 1e-6f);
    EXPECT_NEAR(SphericalPhi(Vector3<f32>{0.f, -1.f, 0.f}), TwoPi<f32>() - PiOver2<f32>(), 1e-6f);
}

TEST(VectorMath_SphericalComponents, CosSinTan)
{
    Vector3<f32> v{To<f32>(1), To<f32>(0), To<f32>(0)}; // theta=pi/2, phi=0
    EXPECT_NEAR(CosTheta(v), 0.f, 1e-6f);
    EXPECT_NEAR(Cos2Theta(v), 0.f, 1e-6f);
    EXPECT_NEAR(AbsCosTheta(v), 0.f, 1e-6f);
    EXPECT_NEAR(Sin2Theta(v), 1.f, 1e-6f);
    EXPECT_NEAR(SinTheta(v), 1.f, 1e-6f);
    EXPECT_TRUE(std::isinf(TanTheta(v)));
    EXPECT_TRUE(std::isinf(Tan2Theta(v)));

    Vector3<f32> v2{0.70710678f, 0.f, 0.70710678f}; // 45 degrees elevation
    EXPECT_NEAR(CosTheta(v2), 0.70710678f, 1e-6f);
    EXPECT_NEAR(Cos2Theta(v2), 0.5f, 1e-6f);
    EXPECT_NEAR(Sin2Theta(v2), 0.5f, 1e-6f);
    EXPECT_NEAR(SinTheta(v2), 0.70710678f, 1e-6f);
    EXPECT_NEAR(TanTheta(v2), 1.f, 1e-6f);
    EXPECT_NEAR(Tan2Theta(v2), 1.f, 1e-6f);
}

TEST(VectorMath_SphericalPhiComponents, CosPhiSinPhi)
{
    Vector3<f32> zpos{0.f, 0.f, 1.f};
    EXPECT_NEAR(CosPhi(zpos), 1.f, 1e-6f);
    EXPECT_NEAR(SinPhi(zpos), 0.f, 1e-6f);

    Vector3<f32> v{0.5f, 0.5f, 0.70710678f};
    auto st = SinTheta(v);
    EXPECT_NEAR(CosPhi(v), std::clamp(v.x() / st, -1.f, 1.f), 1e-6f);
    EXPECT_NEAR(SinPhi(v), std::clamp(v.y() / st, -1.f, 1.f), 1e-6f);
}

TEST(VectorMath_CosDPhiAndHemisphere, CosDPhiAndSameHemisphere)
{
    Vector3<f32> x{1.f, 0.f, 0.f};
    Vector3<f32> y{0.f, 1.f, 0.f};
    Vector3<f32> xm{-1.f, 0.f, 0.f};

    EXPECT_NEAR(CosDPhi(x, x), 1.f, 1e-6f);
    EXPECT_NEAR(CosDPhi(x, y), 0.f, 1e-6f);
    EXPECT_NEAR(CosDPhi(x, xm), -1.f, 1e-6f);

    EXPECT_TRUE(SameHemisphere(Vector3<f32>{0.f, 0.f, 1.f}, Vector3<f32>{0.f, 0.f, 0.5f}));
    EXPECT_FALSE(SameHemisphere(Vector3<f32>{0.f, 0.f, 1.f}, Vector3<f32>{0.f, 0.f, -0.5f}));
    EXPECT_TRUE(SameHemisphere(Vector3<f32>{0.f, 0.f, 1.f}, Normal3<f32>{0.f, 0.f, 0.5f}));
}