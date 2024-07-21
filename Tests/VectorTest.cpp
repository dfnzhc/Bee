/**
 * @File VectorTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/30
 * @Brief 
 */

#include <gtest/gtest.h>

#include <Bee/Bee.hpp>
#include <format>

using namespace bee;

TEST(CwEqualWithEpsilonTest, MakeVec)
{
    EXPECT_EQ(make_vec(1.0f), float1(1.0));
    EXPECT_EQ(make_vec(1.0f, 2.0f), float2(1.0, 2.0));
    EXPECT_EQ(make_vec(1.0f, 2.0f, 3.0f), float3(1.0, 2.0, 3.0));
    EXPECT_EQ(make_vec(1.0f, 2.0f, 3.0f, 4.0f), float4(1.0, 2.0, 3.0, 4.0));
}

TEST(BoolVectorFunctionsTest, AnyTest)
{
    EXPECT_TRUE(any(bool1{true}));
    EXPECT_FALSE(any(bool1{false}));
    EXPECT_TRUE(any(bool2{false, true}));
    EXPECT_FALSE(any(bool2{false, false}));

    EXPECT_TRUE(any(bool3{true, false, false}));
    EXPECT_FALSE(any(bool3{false, false, false}));
    EXPECT_TRUE(any(bool3{false, true, false}));
    EXPECT_TRUE(any(bool3{false, false, true}));

    EXPECT_TRUE(any(bool4{true, false, false, false}));
    EXPECT_FALSE(any(bool4{false, false, false, false}));
    EXPECT_TRUE(any(bool4{false, true, false, false}));
    EXPECT_TRUE(any(bool4{false, false, true, false}));
    EXPECT_TRUE(any(bool4{false, false, false, true}));
}

TEST(BoolVectorFunctionsTest, AllTest)
{
    EXPECT_FALSE(all(bool1{false}));
    EXPECT_TRUE(all(bool1{true}));
    EXPECT_TRUE(all(bool2{true, true}));
    EXPECT_FALSE(all(bool2{true, false}));

    EXPECT_FALSE(all(bool3{true, false, false}));
    EXPECT_TRUE(all(bool3{true, true, true}));

    EXPECT_FALSE(all(bool4{true, false, true, true}));
    EXPECT_TRUE(all(bool4{true, true, true, true}));
}

TEST(BoolVectorFunctionsTest, NoneTest)
{
    EXPECT_TRUE(none(bool1{false}));
    EXPECT_FALSE(none(bool1{true}));
    EXPECT_TRUE(none(bool2{false, false}));
    EXPECT_FALSE(none(bool2{true, false}));

    EXPECT_TRUE(none(bool3{false, false, false}));
    EXPECT_FALSE(none(bool3{true, false, false}));

    EXPECT_TRUE(none(bool4{false, false, false, false}));
    EXPECT_FALSE(none(bool4{false, true, false, false}));
}

TEST(BoolVectorFunctionsTest, NegativeTest)
{
    EXPECT_EQ(negative(bool1{true}).x, false);
    EXPECT_EQ(negative(bool1{false}).x, true);
    EXPECT_EQ(negative(bool2{true, false}).x, false);
    EXPECT_EQ(negative(bool2{true, false}).y, true);

    auto neg3 = negative(bool3{true, false, true});
    EXPECT_FALSE(neg3.x);
    EXPECT_TRUE(neg3.y);
    EXPECT_FALSE(neg3.z);

    auto neg4 = negative(bool4{true, false, true, false});
    EXPECT_FALSE(neg4.x);
    EXPECT_TRUE(neg4.y);
    EXPECT_FALSE(neg4.z);
    EXPECT_TRUE(neg4.w);
}

TEST(CwEqualTest, CompareVec1)
{
    vec<1, int> a1(5), b1(5);
    auto result1 = cwEqual(a1, b1);
    EXPECT_TRUE(result1.x);

    vec<1, int> c1(5), d1(3);
    auto result2 = cwEqual(c1, d1);
    EXPECT_FALSE(result2.x);
}

TEST(CwEqualTest, CompareVec2)
{
    vec<2, int> a2(5, 10), b2(5, 10);
    auto result1 = cwEqual(a2, b2);
    EXPECT_TRUE(result1.x && result1.y);

    vec<2, int> c2(5, 10), d2(3, 10);
    auto result2 = cwEqual(c2, d2);
    EXPECT_FALSE(result2.x && result2.y);
}

TEST(CwEqualTest, CompareVec3)
{
    vec<3, int> a3(5, 10, 15), b3(5, 10, 15);
    auto result1 = cwEqual(a3, b3);
    EXPECT_TRUE(result1.x && result1.y && result1.z);

    vec<3, int> c3(5, 10, 15), d3(3, 10, 15);
    auto result2 = cwEqual(c3, d3);
    EXPECT_FALSE(result2.x && result2.y && result2.z);
}

TEST(CwEqualTest, CompareVec4)
{
    vec<4, int> a4(5, 10, 15, 20), b4(5, 10, 15, 20);
    auto result1 = cwEqual(a4, b4);
    EXPECT_TRUE(result1.x && result1.y && result1.z && result1.w);

    vec<4, int> c4(5, 10, 15, 20), d4(3, 10, 15, 20);
    auto result2 = cwEqual(c4, d4);
    EXPECT_FALSE(result2.x && result2.y && result2.z && result2.w);
}

TEST(CwEqualWithEpsilonTest, CompareVec1)
{
    vec<1, double> a1{0.00001}, b1{0.00002};
    EXPECT_TRUE(cwEqual(a1, b1, 1e-5).x);

    a1 = vec<1, double>{1.0};
    b1 = vec<1, double>{1.0000001};
    EXPECT_TRUE(cwEqual(a1, b1, 1e-6).x);
}

TEST(CwEqualWithEpsilonTest, CompareVec2)
{
    vec<2, double> a2{0.1, 0.2}, b2{0.1001, 0.1999};
    EXPECT_TRUE(all(cwEqual(a2, b2, 0.01)));

    a2 = vec<2, double>{1.0, 2.0};
    b2 = vec<2, double>{1.0001, 1.9999};
    EXPECT_TRUE(all(cwEqual(a2, b2, 0.01)));
}

TEST(CwEqualWithEpsilonTest, CompareVec3)
{
    vec<3, double> a3 = make_vec(0.1, 0.2, 0.3);
    vec<3, double> b3 = make_vec(0.101, 0.198, 0.301);
    EXPECT_TRUE(all(cwEqual(a3, b3, 0.01)));

    a3 = make_vec(1.0, 2.0, 3.0);
    b3 = make_vec(1.00001, 1.99999, 3.00001);
    EXPECT_TRUE(all(cwEqual(a3, b3, 1.1e-5)));
}

// Continue with vec4 tests
TEST(CwEqualWithEpsilonTest, CompareVec4)
{
    vec<4, double> a4 = make_vec(0.1, 0.2, 0.3, 0.4);
    vec<4, double> b4 = make_vec(0.101, 0.201, 0.301, 0.401);
    EXPECT_TRUE(all(cwEqual(a4, b4, 0.01)));

    a4 = make_vec(1.0, 2.0, 3.0, 4.0);
    b4 = make_vec(0.99999, 2.00001, 2.99999, 4.00001);
    EXPECT_TRUE(all(cwEqual(a4, b4, 1.1e-5)));
}

TEST(LerpTest, Vec1)
{
    auto a      = vec<1, float>{2.0f};
    auto b      = vec<1, float>{5.0f};
    auto result = Lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 3.5f); // (2 + 3) / 2
}

TEST(LerpTest, Vec2)
{
    auto a      = vec<2, float>{1.0f, 4.0f};
    auto b      = vec<2, float>{2.0f, 5.0f};
    auto result = Lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 1.5f); // (1 + 2) / 2
    EXPECT_FLOAT_EQ(result.y, 4.5f); // (4 + 5) / 2
}

TEST(LerpTest, Vec3)
{
    auto a      = vec<3, float>{1.0f, 2.0f, 3.0f};
    auto b      = vec<3, float>{4.0f, 5.0f, 6.0f};
    auto result = Lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 2.5f); // (1 + 4) / 2
    EXPECT_FLOAT_EQ(result.y, 3.5f); // (2 + 5) / 2
    EXPECT_FLOAT_EQ(result.z, 4.5f); // (3 + 6) / 2
}

TEST(LerpTest, Vec4)
{
    auto a      = vec<4, float>{1.0f, 2.0f, 3.0f, 4.0f};
    auto b      = vec<4, float>{5.0f, 6.0f, 7.0f, 8.0f};
    auto result = Lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 3.0f); // (1 + 5) / 2
    EXPECT_FLOAT_EQ(result.y, 4.0f); // (2 + 6) / 2
    EXPECT_FLOAT_EQ(result.z, 5.0f); // (3 + 7) / 2
    EXPECT_FLOAT_EQ(result.w, 6.0f); // (4 + 8) / 2
}

TEST(BiLerpTest, Vec1)
{
    auto v00    = vec<1, float>{0.0f};
    auto v01    = vec<1, float>{1.0f};
    auto v10    = vec<1, float>{2.0f};
    auto v11    = vec<1, float>{3.0f};
    auto result = BiLerp(v00, v01, v10, v11, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 1.5f); // Expected bilinear interpolation result
}

TEST(BiLerpTest, Vec2)
{
    auto v00    = vec<2, float>{0.0f, 0.0f};
    auto v01    = vec<2, float>{1.0f, 1.0f};
    auto v10    = vec<2, float>{2.0f, 2.0f};
    auto v11    = vec<2, float>{3.0f, 3.0f};
    auto result = BiLerp(v00, v01, v10, v11, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 1.5f);
    EXPECT_FLOAT_EQ(result.y, 1.5f);
}

TEST(BiLerpTest, Vec3)
{
    auto v00    = vec<3, float>{0.0f, 0.0f, 0.0f};
    auto v01    = vec<3, float>{1.0f, 1.0f, 0.2f};
    auto v10    = vec<3, float>{2.0f, 0.5f, 1.0f};
    auto v11    = vec<3, float>{3.0f, 2.0f, 0.5f};
    auto result = BiLerp(v00, v01, v10, v11, 0.5f, 0.5f);
    // 期望的结果是四个角点沿着 u 和 v 方向的双线性插值
    auto expected = vec<3, float>{1.5f, 0.875f, 0.425f};
    EXPECT_NEAR(result.x, expected.x, 1e-5f);
    EXPECT_NEAR(result.y, expected.y, 1e-5f);
    EXPECT_NEAR(result.z, expected.z, 1e-5f);
}

TEST(BiLerpTest, Vec4)
{
    auto v00    = vec<4, float>{0.0f, 0.0f, 0.0f, 1.0f};
    auto v01    = vec<4, float>{1.0f, 1.0f, 0.2f, 0.8f};
    auto v10    = vec<4, float>{2.0f, 0.5f, 1.0f, 0.5f};
    auto v11    = vec<4, float>{3.0f, 2.0f, 0.5f, 0.3f};
    auto result = BiLerp(v00, v01, v10, v11, 0.5f, 0.5f);
    // 期望的结果是四个角点沿着 u 和 v 方向的双线性插值
    auto expected = vec<4, float>{1.5f, 0.875f, 0.425f, 0.65f};
    EXPECT_NEAR(result.x, expected.x, 1e-5f);
    EXPECT_NEAR(result.y, expected.y, 1e-5f);
    EXPECT_NEAR(result.z, expected.z, 1e-5f);
    EXPECT_NEAR(result.w, expected.w, 1e-5f);
}

TEST(SmoothStepTest, Vec1)
{
    auto v      = vec<1, float>{0.5f};
    auto lo     = vec<1, float>{0.25f};
    auto hi     = vec<1, float>{0.75f};
    auto result = SmoothStep(v, lo, hi);
    EXPECT_FLOAT_EQ(result.x, 0.5f); // Expected to be at the midpoint
}

TEST(SmoothStepTest, Vec2)
{
    auto v      = vec<2, float>{0.5f, 0.75f};
    auto lo     = vec<2, float>{0.25f, 0.5f};
    auto hi     = vec<2, float>{0.75f, 1.0f};
    auto result = SmoothStep(v, lo, hi);
    EXPECT_FLOAT_EQ(result.x, 0.5f);
    EXPECT_NEAR(result.y, 0.5f, 1e-5f); // Slightly less than 1 due to smoothstep shape
}

TEST(SmoothStepTest, Vec3)
{
    auto v      = vec<3, float>{0.5f, 0.25f, 0.75f};
    auto lo     = vec<3, float>{0.0f, 0.0f, 0.5f};
    auto hi     = vec<3, float>{1.0f, 0.5f, 1.0f};
    auto result = SmoothStep(v, lo, hi);
    EXPECT_NEAR(result.x, 0.5f, 1e-5f);
    EXPECT_FLOAT_EQ(result.y, 0.5f); // Expected to be at the start
    EXPECT_NEAR(result.z, 0.5f, 1e-5f);
}

TEST(SmoothStepTest, Vec4)
{
    auto v      = vec<4, float>{0.5f, 0.75f, 0.25f, 0.0f};
    auto lo     = vec<4, float>{0.25f, 0.5f, 0.0f, 0.75f};
    auto hi     = vec<4, float>{0.75f, 1.0f, 0.5f, 0.25f};
    auto result = SmoothStep(v, lo, hi);
    EXPECT_NEAR(result.x, 0.5f, 1e-5f);
    EXPECT_NEAR(result.y, 0.5f, 1e-5f);
    EXPECT_FLOAT_EQ(result.z, 0.5f);
    EXPECT_FLOAT_EQ(result.w, 0.0f);
}

TEST(MinMaxIndexTest, Vec1)
{
    auto v = vec<1, int>{3};
    EXPECT_EQ(MinIndex(v), 0);
    EXPECT_EQ(MaxIndex(v), 0);
}

TEST(MinMaxIndexTest, Vec2)
{
    auto v = vec<2, int>{5, 3};
    EXPECT_EQ(MinIndex(v), 1);
    EXPECT_EQ(MaxIndex(v), 0);
}

TEST(MinMaxIndexTest, Vec3)
{
    auto v = vec<3, int>{7, 3, 5};
    EXPECT_EQ(MinIndex(v), 1);
    EXPECT_EQ(MaxIndex(v), 0);
}

TEST(MinMaxIndexTest, Vec4)
{
    auto v = vec<4, int>{7, 3, 5, 2};
    EXPECT_EQ(MinIndex(v), 3);
    EXPECT_EQ(MaxIndex(v), 0);
}

TEST(MinMaxValueTest, Vec1)
{
    auto v = vec<1, int>{3};
    EXPECT_EQ(MinValue(v), 3);
    EXPECT_EQ(MaxValue(v), 3);
}

TEST(MinMaxValueTest, Vec2)
{
    auto v = vec<2, int>{5, 3};
    EXPECT_EQ(MinValue(v), 3);
    EXPECT_EQ(MaxValue(v), 5);
}

TEST(MinMaxValueTest, Vec3)
{
    auto v = vec<3, int>{7, 3, 5};
    EXPECT_EQ(MinValue(v), 3);
    EXPECT_EQ(MaxValue(v), 7);
}

TEST(MinMaxValueTest, Vec4)
{
    auto v = vec<4, int>{7, 3, 5, 2};
    EXPECT_EQ(MinValue(v), 2);
    EXPECT_EQ(MaxValue(v), 7);
}

TEST(ReduceMinTest, Vec)
{
    EXPECT_TRUE(ReduceMin(vec<1, int>{3}) == int1(3));
    EXPECT_TRUE(ReduceMin(vec<2, int>{5, 3}) == int2(3));
    EXPECT_TRUE(ReduceMin(vec<3, int>{7, 3, 5}) == int3(3));
    EXPECT_TRUE(ReduceMin(vec<4, int>{7, 3, 5, 2}) == int4(2));
}

TEST(ReduceMaxTest, Vec)
{
    EXPECT_TRUE(ReduceMax(vec<1, int>{3}) == int1(3));
    EXPECT_TRUE(ReduceMax(vec<2, int>{5, 3}) == int2(5));
    EXPECT_TRUE(ReduceMax(vec<3, int>{7, 3, 5}) == int3(7));
    EXPECT_TRUE(ReduceMax(vec<4, int>{7, 3, 5, 2}) == int4(7));
}

TEST(ClampTest, Vec1)
{
    auto v      = vec<1, float>{-1.0f};
    auto lo     = vec<1, float>{0.0f};
    auto hi     = vec<1, float>{1.0f};
    auto result = Clamp(v, lo, hi);
    EXPECT_FLOAT_EQ(result.x, 0.0f); // Should clamp to the minimum value
}

TEST(ClampTest, Vec2)
{
    auto v      = vec<2, float>{-1.0f, 2.0f};
    auto lo     = vec<2, float>{0.0f, 1.0f};
    auto hi     = vec<2, float>{1.0f, 2.0f};
    auto result = Clamp(v, lo, hi);
    EXPECT_FLOAT_EQ(result.x, 0.0f); // Should clamp to the minimum value
    EXPECT_FLOAT_EQ(result.y, 2.0f); // Should be unchanged as it's within the range
}

TEST(ClampTest, Vec3)
{
    auto v      = vec<3, float>{-1.0f, 3.0f, -2.0f};
    auto lo     = vec<3, float>{0.0f, 1.5f, 0.0f};
    auto hi     = vec<3, float>{1.0f, 3.0f, 1.0f};
    auto result = Clamp(v, lo, hi);
    EXPECT_FLOAT_EQ(result.x, 0.0f); // Should clamp to the minimum value
    EXPECT_FLOAT_EQ(result.y, 3.0f); // Should be unchanged as it's within the range
    EXPECT_FLOAT_EQ(result.z, 0.0f); // Should clamp to the minimum value
}

TEST(ClampTest, Vec4)
{
    auto v      = vec<4, float>{-1.0f, 2.0f, 4.0f, -3.0f};
    auto lo     = vec<4, float>{0.0f, 1.0f, 2.0f, 0.0f};
    auto hi     = vec<4, float>{1.0f, 3.0f, 4.0f, 1.0f};
    auto result = Clamp(v, lo, hi);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
    EXPECT_FLOAT_EQ(result.w, 0.0f);
}

TEST(DotTest, Vec1)
{
    vec<1, int> a(5), b(10);
    EXPECT_EQ(Dot(a, b), 50); // 5 * 10
}

TEST(DotTest, Vec2)
{
    vec<2, float> a(3.0f, 4.0f), b(-1.0f, 2.0f);
    EXPECT_NEAR(Dot(a, b), 5.0f, 1e-5f); // 3*-1 + 4*2
}

TEST(DotTest, Vec3)
{
    vec<3, double> a(1.0, -2.0, 3.0), b(-1.0, 1.0, 0.0);
    EXPECT_DOUBLE_EQ(Dot(a, b), -3.0); // 1*-1 + -2*1 + 3*0
}

TEST(DotTest, Vec4)
{
    vec<4, int> a(1, 2, 3, 4), b(-1, 1, 0, 2);
    EXPECT_EQ(Dot(a, b), 9); // 1*-1 + 2*1 + 3*0 + 4*2
}

TEST(NormalizeDotTest, Vec1)
{
    vec<1, float> a(3), b(4);
    EXPECT_NEAR(NormalizeDot(a, b), 3.f * 4 * rSqrt(3.0 * 3 * 4 * 4), 1e-5f);
}

TEST(NormalizeDotTest, Vec2)
{
    vec<2, float> a(3, 4), b(-1, 2);
    auto dot  = Dot(a, b);
    auto lenA = Dot(a, a);
    auto lenB = Dot(b, b);
    EXPECT_NEAR(NormalizeDot(a, b), dot * rSqrt(lenA * lenB), 1e-5f);
}

TEST(NormalizeDotTest, Vec3)
{
    vec<3, double> a(1, 2, 3), b(-1, 1, 0);
    auto dot  = Dot(a, b);
    auto lenA = Dot(a, a);
    auto lenB = Dot(b, b);
    EXPECT_NEAR(NormalizeDot(a, b), dot * rSqrt(lenA * lenB), 1e-6);
}

TEST(NormalizeDotTest, Vec4)
{
    vec<4, float> a(1, 2, 3, 4), b(-1, 1, 0, 2);
    auto dot  = Dot(a, b);
    auto lenA = Dot(a, a);
    auto lenB = Dot(b, b);
    EXPECT_NEAR(NormalizeDot(a, b), dot * rSqrt(lenA * lenB), 1e-5f);
}

TEST(LengthTest, Vec1)
{
    vec<1, int> v(3);
    EXPECT_EQ(Length(v), 3);
}

TEST(LengthTest, Vec2)
{
    vec<2, float> v(3.0f, 4.0f);
    EXPECT_NEAR(Length(v), std::sqrt(3 * 3 + 4 * 4), 1e-5f);
}

TEST(LengthTest, Vec3)
{
    vec<3, double> v(1.0, 2.0, 3.0);
    EXPECT_NEAR(Length(v), std::sqrt(1 * 1 + 2 * 2 + 3 * 3), 1e-10);
}

TEST(LengthTest, Vec4)
{
    vec<4, float> v(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_NEAR(Length(v), std::sqrt(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4), 1e-5f);
}

TEST(DistanceTest, Vec1)
{
    vec<1, int> v1(3), v2(6);
    EXPECT_EQ(Distance(v1, v2), 3);
}

TEST(DistanceTest, Vec2)
{
    vec<2, double> v1(3.0, 4.0), v2(1.0, -2.0);
    auto diff = v1 - v2; // Should be (2, 6)
    EXPECT_NEAR(Distance(v1, v2), Length(diff), 1e-10);
}

TEST(DistanceTest, Vec3)
{
    vec<3, float> v1(1.0f, 2.0f, 3.0f), v2(4.0f, 6.0f, 5.0f);
    auto diff = v1 - v2; // Should be (-3, -4, -2)
    EXPECT_NEAR(Distance(v1, v2), Length(diff), 1e-5f);
}

TEST(DistanceTest, Vec4)
{
    vec<4, int> v1(1, 2, 3, 4), v2(10, 10, 10, 10);
    auto diff = v1 - v2; // Should be (-9, -8, -7, -6)
    EXPECT_NEAR(Distance(v1, v2), Length(diff), 1e-5f);
}

TEST(NormalizeTest, NormalizesCorrectly)
{
    int3 v(1.0, 2.0, 3.0);
    auto normalized_v = Normalize(v);

    EXPECT_EQ(Dot(v, v), LengthSqr(v));
    EXPECT_NEAR(Dot(normalized_v, normalized_v), 1.0, 1e-6);
}

TEST(ProdTest, Vec1)
{
    vec<1, int> v{2};
    EXPECT_EQ(Prod(v), 2);
}

TEST(ProdTest, Vec2)
{
    vec<2, int> v{2, 3};
    EXPECT_EQ(Prod(v), 6);
}

TEST(ProdTest, Vec3)
{
    vec<3, int> v{2, 3, 4};
    EXPECT_EQ(Prod(v), 24);
}

TEST(ProdTest, VecFloat)
{
    vec<3, float> v{2.0f, 3.0f, 4.0f};
    EXPECT_NEAR(Prod(v), 24.0f, 1e-5f);
}

TEST(CrossTest, Vec2)
{
    vec2_t<int> v1{2, 3};
    vec2_t<int> v2{4, 5};
    EXPECT_EQ(Cross(v1, v2), -2); // 2*5 - 3*4
}

TEST(CrossTest, Vec3)
{
    vec3_t<double> v1{1.0, 2.0, 3.0};
    vec3_t<double> v2{4.0, 5.0, 6.0};
    auto cross = Cross(v1, v2);
    EXPECT_DOUBLE_EQ(cross.x, -3.0); // 2*6 - 3*5
    EXPECT_DOUBLE_EQ(cross.y, 6.0);  // 3*4 - 1*6
    EXPECT_DOUBLE_EQ(cross.z, -3.0); // 1*5 - 2*4
}

TEST(ProjectionAndPerpendicularTest, Vec3)
{
    vec<3, double> x{1.0, 2.0, 3.0};
    vec<3, double> Normal{0.0, 1.0, 0.0};

    auto proj = Proj(x, Normal);
    auto perp = Perp(x, Normal);

    // Projection should have zero x and z components, and a non-zero y component
    EXPECT_EQ(proj[0], 0.0);
    EXPECT_EQ(proj[2], 0.0);
    EXPECT_NE(proj[1], 0.0);

    // Perpendicular component should have the same x and z components as x, and a zero y component
    EXPECT_EQ(perp[0], x[0]);
    EXPECT_EQ(perp[2], x[2]);
    EXPECT_EQ(perp[1], 0.0);

    // The original vector x should be the sum of the projection and the perpendicular component
    EXPECT_EQ(x, proj + perp);
}

TEST(GramSchmidtTest, OrthogonalizeVectors)
{
    vec3_t<double> v1{1.0, 2.0, 3.0};
    vec3_t<double> v2{4.0, 5.0, 6.0};

    auto orthogonalized = GramSchmidt(v1, v2);

    // The dot product between the original v1 and the orthogonalized v2 should be close to zero
    EXPECT_NEAR(Dot(v1, orthogonalized), 0.0, 1e-6);
}

TEST(CoordinateSystemTest, ConstructsOrthonormalBasis)
{
    vec3_t<double> v1(1.0, 0.0, 0.0);
    vec3_t<double> v2, v3;

    CoordinateSystem(v1, v2, v3);

    // v2 and v3 should be orthogonal to v1
    EXPECT_NEAR(Dot(v1, v2), 0.0, 1e-6);
    EXPECT_NEAR(Dot(v1, v3), 0.0, 1e-6);

    // v2 and v3 should be orthogonal to each other
    EXPECT_NEAR(Dot(v2, v3), 0.0, 1e-6);

    // The cross product of v2 and v3 should give a vector parallel to v1
    vec3_t<double> cross = Cross(v2, v3);
    double dotProduct    = Dot(cross, v1);
    EXPECT_NEAR(dotProduct, 1.0, 1e-6); // Assuming v1 is normalized
}

TEST(AngleBetweenTest, SameDirection)
{
    vec<3, double> v1{1.0, 0.0, 0.0};
    vec<3, double> v2{1.0, 0.0, 0.0};
    EXPECT_DOUBLE_EQ(AngleBetween(v1, v2), 0.0);
}

TEST(AngleBetweenTest, OppositeDirection)
{
    vec<3, double> v1{1.0, 0.0, 0.0};
    vec<3, double> v2{-1.0, 0.0, 0.0};
    EXPECT_DOUBLE_EQ(AngleBetween(v1, v2), kPi);
}

TEST(AngleBetweenTest, Perpendicular)
{
    vec<3, double> v1{1.0, 0.0, 0.0};
    vec<3, double> v2{0.0, 1.0, 0.0};
    EXPECT_DOUBLE_EQ(AngleBetween(v1, v2), kPi / 2);
}

TEST(AngleBetweenTest, AcuteAngle)
{
    vec<2, float> v1{1.0f, 0.0f};
    vec<2, float> v2{0.5f, 0.5f};
    EXPECT_NEAR(AngleBetween(v1, v2), std::acos(1.0f / std::sqrt(2.0f)), 1e-6f);
}

TEST(OrientedAngleTest, Vec2_Angle_Positive)
{
    float2 x{1.0, 0.0};
    float2 y{0.0, 1.0};
    EXPECT_NEAR(OrientedAngle(x, y), 1.57079632679, 1e-6); // 90 degrees in radians
}

TEST(OrientedAngleTest, Vec2_Angle_Negative)
{
    float2 x{1.0, 0.0};
    float2 y{0.0, -1.0};
    EXPECT_NEAR(OrientedAngle(x, y), -1.57079632679, 1e-6); // -90 degrees in radians
}

TEST(OrientedAngleTest, Vec3_Angle_Positive)
{
    float3 x{1.0, 0.0, 0.0};
    float3 y{0.0, 1.0, 0.0};
    float3 ref{0.0, 0.0, 1.0};
    EXPECT_NEAR(OrientedAngle(x, y, ref), 1.57079632679, 1e-6); // 90 degrees in radians
}

TEST(OrientedAngleTest, Vec3_Angle_Negative)
{
    float3 x{1.0, 0.0, 0.0};
    float3 y{0.0, 1.0, 0.0};
    float3 ref{0.0, 0.0, -1.0};
    EXPECT_NEAR(OrientedAngle(x, y, ref), -1.57079632679, 1e-6); // -90 degrees in radians
}

TEST(ClosestPointTest, PointAtStartOfSegment)
{
    float2 a{0, 0};
    float2 b{1, 1};
    float2 p       = a;
    float2 closest = ClosestPoint(a, b, p);
    EXPECT_EQ(closest, a);
}

TEST(ClosestPointTest, PointAtEndOfSegment)
{
    float2 a{0, 0};
    float2 b{1, 1};
    float2 p       = b;
    float2 closest = ClosestPoint(a, b, p);
    EXPECT_EQ(closest, b);
}

TEST(ClosestPointTest, PointOnLineInsideSegment)
{
    float2 a{0, 0};
    float2 b{1, 1};
    float2 p       = (a + b) / 2.f; // Midpoint
    float2 closest = ClosestPoint(a, b, p);
    EXPECT_NEAR(Dot(closest - a, b - a), Dot(p - a, b - a), 1e-6);
}

TEST(ClosestPointTest, PointOutsideSegment_2D)
{
    float2 a{0, 0};
    float2 b{1, 1};
    float2 p{-1, -1};
    float2 closest = ClosestPoint(a, b, p);
    EXPECT_EQ(closest, a); // Assuming the closest point is clamped to 'a'
}

TEST(ClosestPointTest, PointOutsideSegment_3D)
{
    float3 a{0, 0, 0};
    float3 b{1, 1, 1};
    float3 p{-1, -1, -1};
    float3 closest = ClosestPoint(a, b, p);
    EXPECT_EQ(closest, a); // Assuming the closest point is clamped to 'a'
}

TEST(GaussTest, MeanAtExpectedValue)
{
    vec2_t<double> coord{0.0, 0.0};
    vec2_t<double> expectedValue{0.0, 0.0};
    vec2_t<double> standardDeviation{1.0, 1.0};
    EXPECT_NEAR(Gauss(coord, expectedValue, standardDeviation), 1.0, 1e-6);
}

TEST(GaussTest, NonZeroMean)
{
    vec2_t<double> coord{1.0, 2.0};
    vec2_t<double> expectedValue{1.0, 2.0};
    vec2_t<double> standardDeviation{0.1, 0.1};
    EXPECT_NEAR(Gauss(coord, expectedValue, standardDeviation), Exp(0.0), 1e-6);
}

TEST(GaussTest, StandardDeviationScale)
{
    vec2_t<double> coord{1.0, 1.0};
    vec2_t<double> expectedValue{0.0, 0.0};
    vec2_t<double> standardDeviation{3.0, 4.0};
    const auto a   = 2.0 * 3.0 * 3.0;
    const auto b   = 2.0 * 4.0 * 4.0;
    const auto val = 1.0 / a + 1.0 / b;
    EXPECT_NEAR(Gauss(coord, expectedValue, standardDeviation), Exp(-val), 1e-6);
}

TEST(RadialGradientTest, PositionAtFocalPoint)
{
    vec2_t<double> center{0.0, 0.0};
    double radius = 1.0;
    vec2_t<double> focal{0.0, 0.0};
    vec2_t<double> position{0.0, 0.0};
    EXPECT_DOUBLE_EQ(RadialGradient(center, radius, focal, position), 0.0);
}

TEST(RadialGradientTest, PositionAtCenter)
{
    vec2_t<double> center{0.0, 0.0};
    double radius = 1.0;
    vec2_t<double> focal{0.0, 0.0};
    vec2_t<double> position{1.0, 0.0};
    EXPECT_DOUBLE_EQ(RadialGradient(center, radius, focal, position), 1.0);
}

TEST(LinearGradientTest, PositionAtPoint0)
{
    vec2_t<double> point0{0.0, 0.0};
    vec2_t<double> point1{1.0, 1.0};
    vec2_t<double> position{0.0, 0.0};
    EXPECT_DOUBLE_EQ(LinearGradient(point0, point1, position), 0.0);
}

TEST(LinearGradientTest, PositionAtPoint1)
{
    vec2_t<double> point0{0.0, 0.0};
    vec2_t<double> point1{1.0, 1.0};
    vec2_t<double> position{1.0, 1.0};
    EXPECT_DOUBLE_EQ(LinearGradient(point0, point1, position), 1.0);
}

TEST(LinearGradientTest, PositionOnLineSegment)
{
    vec2_t<double> point0{0.0, 0.0};
    vec2_t<double> point1{2.0, 2.0};
    vec2_t<double> position{1.0, 1.0}; // Midpoint
    EXPECT_DOUBLE_EQ(LinearGradient(point0, point1, position), 0.5);
}

TEST(LinearGradientTest, PositionOutsideLineSegment)
{
    vec2_t<double> point0{0.0, 0.0};
    vec2_t<double> point1{1.0, 1.0};
    vec2_t<double> position{2.0, 2.0};
    // The expected value depends on the direction of the line; here we assume it's positive
    double expected = Dot(point1 - point0, position - point0) / Dot(point1 - point0, point1 - point0);
    EXPECT_DOUBLE_EQ(LinearGradient(point0, point1, position), expected);
}

TEST(IsHandednessTest, isRightHanded)
{
    vec3_t<float> tangent{1.0f, 0.0f, 0.0f};
    vec3_t<float> binormal{0.0f, 1.0f, 0.0f};
    vec3_t<float> normal{0.0f, 0.0f, 1.0f};
    EXPECT_TRUE(isRightHanded(tangent, binormal, normal));
}

TEST(IsHandednessTest, isNotRightHanded)
{
    vec3_t<float> tangent{1.0f, 0.0f, 0.0f};
    vec3_t<float> binormal{0.0f, 1.0f, 0.0f};
    vec3_t<float> normal{0.0f, 0.0f, -1.0f};
    EXPECT_FALSE(isRightHanded(tangent, binormal, normal));
}

// Tests for isLeftHanded function
TEST(IsHandednessTest, isLeftHanded)
{
    vec3_t<double> tangent{1.0, 0.0, 0.0};
    vec3_t<double> binormal{0.0, 1.0, 0.0};
    vec3_t<double> normal{0.0, 0.0, -1.0};
    EXPECT_TRUE(isLeftHanded(tangent, binormal, normal));
}

TEST(IsHandednessTest, isNotLeftHanded)
{
    vec3_t<double> tangent{1.0, 0.0, 0.0};
    vec3_t<double> binormal{0.0, 1.0, 0.0};
    vec3_t<double> normal{0.0, 0.0, 1.0};
    EXPECT_FALSE(isLeftHanded(tangent, binormal, normal));
}

// Tests for isNormalized function
TEST(IsNormalizationTest, isNormalized)
{
    vec3_t<float> v{1.0f / std::sqrt(3.0f), 1.0f / std::sqrt(3.0f), 1.0f / std::sqrt(3.0f)};
    float epsilon = 1e-6f;
    EXPECT_TRUE(isNormalized(v, epsilon));
}

TEST(IsNormalizationTest, IsNotNormalized)
{
    vec3_t<double> v{1.0, 2.0, 3.0};
    double epsilon = 1e-6;
    EXPECT_FALSE(isNormalized(v, epsilon));
}

TEST(FaceForwardTest, ReturnsOppositeDirection)
{
    vec<3, double> N{1.0, 0.0, 0.0};
    vec<3, double> I{-1.0, 0.0, 0.0};
    vec<3, double> refN{0.0, 1.0, 0.0};
    vec<3, double> result = FaceForward(N, I, refN);
    vec<3, double> expect = vec<3, double>{-1.0, 0.0, 0.0};
    EXPECT_EQ(result, expect);
}

TEST(FaceForwardTest, ReturnsSameDirection)
{
    vec<3, double> N{1.0, 0.0, 0.0};
    vec<3, double> I{1.0, 0.0, 0.0};
    vec<3, double> refN{-1.0, 1.0, 0.0};
    vec<3, double> result = FaceForward(N, I, refN);
    EXPECT_EQ(result, N);
}

TEST(FaceForwardTest, HandlesZeroVector)
{
    vec<3, double> N{1.0, 0.0, 0.0};
    vec<3, double> I{0.0, 0.0, 0.0};
    vec<3, double> refN{0.0, 1.0, 0.0};
    vec<3, double> result = FaceForward(N, I, refN);
    vec<3, double> expect = vec<3, double>{-1.0, 0.0, 0.0};
    EXPECT_EQ(result, expect);
}

TEST(ReflectTest, ReflectsVec2)
{
    vec<2, float> I{1.0f, -1.0f};
    vec<2, float> N{0.0f, 1.0f};
    auto R = Reflect(I, N);

    EXPECT_DOUBLE_EQ(R[0], 1.0);
    EXPECT_DOUBLE_EQ(R[1], 1.0);
}

TEST(ReflectTest, ReflectsAcrossNormal)
{
    vec<3, double> I{1.0, -1.0, 0.0};
    vec<3, double> N{0.0, 1.0, 0.0};
    auto R = Reflect(I, N);

    EXPECT_DOUBLE_EQ(R[0], 1.0);
    EXPECT_DOUBLE_EQ(R[1], 1.0);
    EXPECT_DOUBLE_EQ(R[2], 0.0);
}

TEST(ReflectTest, ReflectsAt90DegreeAngle)
{
    vec<3, double> I{1.0, 0.0, 0.0}; // Incoming vector along the x-axis
    vec<3, double> N{0.0, 1.0, 0.0}; // Normal is the y-axis
    auto reflected = Reflect(I, N);

    EXPECT_DOUBLE_EQ(reflected[0], 1.0);
    EXPECT_DOUBLE_EQ(reflected[1], 0.0);
    EXPECT_DOUBLE_EQ(reflected[2], 0.0);
}

TEST(ReflectTest, ReflectsIn3D)
{
    vec<3, double> I{1.0, 1.0, 1.0}; // Incoming vector
    vec<3, double> N{0.0, 1.0, 0.0}; // Normal is the y-axis
    auto reflected = Reflect(-I, N);

    EXPECT_DOUBLE_EQ(reflected[0], -1.0);
    EXPECT_DOUBLE_EQ(reflected[1], 1.0);
    EXPECT_DOUBLE_EQ(reflected[2], -1.0);
}

TEST(ReflectTest, ReflectsOffNonUnitNormal)
{
    vec<3, double> I{1.0, -1.0, 0.0};
    vec<3, double> N{0.0, 1.0, std::sqrt(2.0)}; // Non-unit normal
    auto reflected = Reflect(I, N);

    EXPECT_DOUBLE_EQ(reflected[0], 1.0);
    EXPECT_DOUBLE_EQ(reflected[1], -1.0 / 3.0);
    EXPECT_DOUBLE_EQ(reflected[2], 2.0 * std::sqrt(2.0) / 3.);
}

TEST(ReflectTest, ReflectsTangential)
{
    vec<3, double> I{1.0, 0.0, 0.0};
    vec<3, double> N{1.0, 0.0, 0.0}; // Normal is parallel to the incident vector
    auto reflected = Reflect(-I, N);

    EXPECT_DOUBLE_EQ(reflected[0], I[0]);
    EXPECT_DOUBLE_EQ(reflected[1], I[1]);
    EXPECT_DOUBLE_EQ(reflected[2], I[2]);
}

TEST(PolarTest, ConvertsEuclideanToPolar)
{
    vec3_t<double> euclidean{1.0, 0.0, 0.0};
    auto polar = Polar(euclidean);
    EXPECT_NEAR(polar.x, kHalfPi, 1e-6);
    EXPECT_NEAR(polar.y, 0.0, 1e-6);
    EXPECT_NEAR(polar.z, 1.0, 1e-6); // xz distance should be 1 since x = 1 and z = 0
}

TEST(PolarTest, ConvertsEuclideanToPolarWithLatitude)
{
    vec3_t<double> euclidean{0.0, 1.0, 0.0};
    auto polar = Polar(euclidean);
    EXPECT_NEAR(polar.x, kHalfPi, 1e-6); // 90 degrees in radians
    EXPECT_NEAR(polar.y, kHalfPi, 1e-6);
    EXPECT_NEAR(polar.z, 1.0, 1e-6);
}

TEST(PolarTest, ConvertsEuclideanToPolarWithLongitude)
{
    vec3_t<double> euclidean{1.0, 0.0, 1.0};
    auto polar = Polar(euclidean);
    // longitude is atan(x/z), which should be Pi/4 for x = z = 1
    EXPECT_NEAR(polar.x, kQuarterPi, 1e-6);
    EXPECT_NEAR(polar.y, 0, 1e-6);
    EXPECT_NEAR(polar.z, Sqrt(2.0), 1e-6); // xz distance should be sqrt(1^2 + 1^2) = sqrt(2)
}

TEST(EuclideanTest, ConvertsPolarToEuclidean)
{
    vec2_t<double> polar{0.0, 0.0}; // North Pole
    auto euclidean = Euclidean(polar);
    EXPECT_NEAR(euclidean.x, 0.0, 1e-6);
    EXPECT_NEAR(euclidean.y, 0.0, 1e-6);
    EXPECT_NEAR(euclidean.z, 1.0, 1e-6);
}

TEST(EuclideanTest, ConvertsPolarToEuclideanAtPiOverTwo)
{
    vec2_t<double> polar{kHalfPi, kHalfPi}; // Equator, facing positive x-axis
    auto euclidean = Euclidean(polar);
    EXPECT_NEAR(euclidean.x, 0.0, 1e-6);
    EXPECT_NEAR(euclidean.y, 1.0, 1e-6);
    EXPECT_NEAR(euclidean.z, 0.0, 1e-6);
}

TEST(EuclideanTest, ConvertsPolarToEuclideanAtPi)
{
    vec2_t<double> polar{kPi, 0.0}; // Z-axis, negative direction
    auto euclidean = Euclidean(polar);
    EXPECT_NEAR(euclidean.x, 0.0, 1e-6);
    EXPECT_NEAR(euclidean.y, 0.0, 1e-6);
    EXPECT_NEAR(euclidean.z, -1.0, 1e-6);
}

TEST(EuclideanTest, ConvertsPolarToEuclideanWithLongitude)
{
    vec2_t<double> polar{kQuarterPi, kQuarterPi}; // First quadrant, 45 degrees from both axes
    auto euclidean = Euclidean(polar);
    EXPECT_NEAR(euclidean.x, 1. / 2, 1e-6);
    EXPECT_NEAR(euclidean.y, 1. / 2, 1e-6);
    EXPECT_NEAR(euclidean.z, std::sqrt(2) / 2, 1e-6);
}

class SlerpTest : public ::testing::Test
{
protected:
    vec3_t<float> vecA, vecB;
    float alpha;

    void SetUp() override
    {
        vecA  = {1.0f, 0.0f, 0.0f}; // X-axis unit vector
        vecB  = {0.0f, 1.0f, 0.0f}; // Y-axis unit vector
        alpha = 0.5f;               // Midpoint of interpolation
    }
};

TEST_F(SlerpTest, SlerpWithAlphaZero)
{
    EXPECT_EQ(Slerp(vecA, vecB, 0.0f), vecA);
}

// Test Slerp with alpha = 1
TEST_F(SlerpTest, SlerpWithAlphaOne)
{
    EXPECT_EQ(Slerp(vecA, vecB, 1.0f), vecB);
}

// Test Slerp with alpha = 0.5 (midpoint)
TEST_F(SlerpTest, SlerpWithAlphaMidpoint)
{
    vec3_t<float> expected = {sqrt(0.5), sqrt(0.5), 0.0f}; // Expected result for midpoint
    EXPECT_NEAR(Slerp(vecA, vecB, alpha).x, expected.x, 1e-6);
    EXPECT_NEAR(Slerp(vecA, vecB, alpha).y, expected.y, 1e-6);
    EXPECT_NEAR(Slerp(vecA, vecB, alpha).z, expected.z, 1e-6);
}

// Test Slerp with alpha = 0.25
//TEST_F(SlerpTest, SlerpWithAlphaQuarter)
//{
//    vec3_t<float> result = Slerp(vecA, vecB, 0.25f);
//
//    vec3_t<float> expected = {0, 0, 0};
//    EXPECT_NEAR(result.x, expected.x, 1e-6);
//    EXPECT_NEAR(result.y, expected.y, 1e-6);
//    EXPECT_NEAR(result.z, expected.z, 1e-6);
//}