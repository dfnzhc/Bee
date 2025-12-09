/**
 * @File TupleTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/5
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Bee/Bee.hpp>

using namespace bee;

template <typename T>
struct C2 : Tuple<C2, T, 2>
{
    using Base = Tuple<C2, T, 2>;
    using Base::Base;
};

template <typename T>
struct C3 : Tuple<C3, T, 3>
{
    using Base = Tuple<C3, T, 3>;
    using Base::Base;
};

template <typename T>
struct C4 : Tuple<C4, T, 4>
{
    using Base = Tuple<C4, T, 4>;
    using Base::Base;
};

TEST(Tuple2, Basic)
{
    C2<f32> a(1.5f, -2.0f);
    EXPECT_EQ(a[0], 1.5f);
    EXPECT_EQ(a[1], -2.0f);
    EXPECT_FALSE(a.hasNaN());
}

TEST(Tuple2, EqualityAndIndex)
{
    C2<int> a(1, 2), b(1, 2), c(2, 3);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 2);
}

TEST(Tuple2, Arithmetic)
{
    C2<float> a(1.0f, 2.0f), b(3.0f, -1.0f);
    auto c = a + b;
    EXPECT_FLOAT_EQ(c[0], 4.0f);
    EXPECT_FLOAT_EQ(c[1], 1.0f);
    auto d = a - b;
    EXPECT_FLOAT_EQ(d[0], -2.0f);
    EXPECT_FLOAT_EQ(d[1], 3.0f);
    a += b;
    EXPECT_FLOAT_EQ(a[0], 4.0f);
    EXPECT_FLOAT_EQ(a[1], 1.0f);
    a -= b;
    EXPECT_FLOAT_EQ(a[0], 1.0f);
    EXPECT_FLOAT_EQ(a[1], 2.0f);
}

TEST(Tuple2, ScalarOps)
{
    C2<float> a(1.0f, -2.0f);
    auto m1 = a * 2.0f;
    EXPECT_FLOAT_EQ(m1[0], 2.0f);
    EXPECT_FLOAT_EQ(m1[1], -4.0f);
    auto m2 = 2.0f * a;
    EXPECT_FLOAT_EQ(m2[0], 2.0f);
    EXPECT_FLOAT_EQ(m2[1], -4.0f);
    auto d1 = a / 2.0f;
    EXPECT_FLOAT_EQ(d1[0], 0.5f);
    EXPECT_FLOAT_EQ(d1[1], -1.0f);
    a *= 3.0f;
    EXPECT_FLOAT_EQ(a[0], 3.0f);
    EXPECT_FLOAT_EQ(a[1], -6.0f);
    a /= 3.0f;
    EXPECT_FLOAT_EQ(a[0], 1.0f);
    EXPECT_FLOAT_EQ(a[1], -2.0f);
}

TEST(Tuple2, UnaryNeg)
{
    C2<int> a(2, -5);
    auto b = -a;
    EXPECT_EQ(b[0], -2);
    EXPECT_EQ(b[1], 5);
}

TEST(Tuple2, Tools)
{
    C2<float> a(-1.25f, 2.75f);
    auto ab = Abs(a);
    EXPECT_FLOAT_EQ(ab[0], 1.25f);
    EXPECT_FLOAT_EQ(ab[1], 2.75f);
    C2<float> c(1.2f, 2.9f);
    auto cf = Floor(c);
    EXPECT_FLOAT_EQ(cf[0], 1.0f);
    EXPECT_FLOAT_EQ(cf[1], 2.0f);
    auto cc = Ceil(c);
    EXPECT_FLOAT_EQ(cc[0], 2.0f);
    EXPECT_FLOAT_EQ(cc[1], 3.0f);
}

TEST(Tuple2, MinMax)
{
    C2<int> a(1, 5), b(3, 2);
    auto mn = Min(a, b);
    EXPECT_EQ(mn[0], 1);
    EXPECT_EQ(mn[1], 2);
    auto mx = Max(a, b);
    EXPECT_EQ(mx[0], 3);
    EXPECT_EQ(mx[1], 5);
    EXPECT_EQ(MinValue(a), 1);
    EXPECT_EQ(MinIndex(a), 0);
    EXPECT_EQ(MaxValue(a), 5);
    EXPECT_EQ(MaxIndex(a), 1);
    EXPECT_EQ(Product(a), 5);
}

TEST(Tuple2, LerpFMA)
{
    C2<float> a(1.0f, 2.0f), b(3.0f, 4.0f);
    auto l = Lerp(a, b, 0.25f);
    EXPECT_FLOAT_EQ(l[0], 1.5f);
    EXPECT_FLOAT_EQ(l[1], 2.5f);
    auto f = FMA(a, b, 2.3f);
    EXPECT_FLOAT_EQ(f[0], 5.3f);
    EXPECT_FLOAT_EQ(f[1], 10.3f);
}

TEST(Tuple3, Basic)
{
    C3<float> a;
    EXPECT_EQ(a[0], 0);
    EXPECT_EQ(a[1], 0);
    EXPECT_EQ(a[2], 0);
    C3<float> b(1.0f, -2.0f, 3.5f);
    EXPECT_EQ(b[0], 1.0f);
    EXPECT_EQ(b[1], -2.0f);
    EXPECT_EQ(b[2], 3.5f);
    EXPECT_FALSE(b.hasNaN());
}

TEST(Tuple3, Arithmetic)
{
    C3<float> a(1.0f, 2.0f, 3.0f), b(4.0f, -1.0f, 0.5f);
    auto c = a + b;
    EXPECT_FLOAT_EQ(c[0], 5.0f);
    EXPECT_FLOAT_EQ(c[1], 1.0f);
    EXPECT_FLOAT_EQ(c[2], 3.5f);
    auto d = a - b;
    EXPECT_FLOAT_EQ(d[0], -3.0f);
    EXPECT_FLOAT_EQ(d[1], 3.0f);
    EXPECT_FLOAT_EQ(d[2], 2.5f);
    a += b;
    EXPECT_FLOAT_EQ(a[0], 5.0f);
    EXPECT_FLOAT_EQ(a[1], 1.0f);
    EXPECT_FLOAT_EQ(a[2], 3.5f);
    a -= b;
    EXPECT_FLOAT_EQ(a[0], 1.0f);
    EXPECT_FLOAT_EQ(a[1], 2.0f);
    EXPECT_FLOAT_EQ(a[2], 3.0f);
}

TEST(Tuple3, ScalarOps)
{
    C3<float> a(1.0f, -2.0f, 3.0f);
    auto m1 = a * 2.0f;
    EXPECT_FLOAT_EQ(m1[0], 2.0f);
    EXPECT_FLOAT_EQ(m1[1], -4.0f);
    EXPECT_FLOAT_EQ(m1[2], 6.0f);
    auto m2 = 2.0f * a;
    EXPECT_FLOAT_EQ(m2[0], 2.0f);
    EXPECT_FLOAT_EQ(m2[1], -4.0f);
    EXPECT_FLOAT_EQ(m2[2], 6.0f);
    auto d1 = a / 2.0f;
    EXPECT_FLOAT_EQ(d1[0], 0.5f);
    EXPECT_FLOAT_EQ(d1[1], -1.0f);
    EXPECT_FLOAT_EQ(d1[2], 1.5f);
    a *= 3.0f;
    EXPECT_FLOAT_EQ(a[0], 3.0f);
    EXPECT_FLOAT_EQ(a[1], -6.0f);
    EXPECT_FLOAT_EQ(a[2], 9.0f);
    a /= 3.0f;
    EXPECT_FLOAT_EQ(a[0], 1.0f);
    EXPECT_FLOAT_EQ(a[1], -2.0f);
    EXPECT_FLOAT_EQ(a[2], 3.0f);
}

TEST(Tuple3, UnaryNeg)
{
    C3<int> a(2, -5, 7);
    auto b = -a;
    EXPECT_EQ(b[0], -2);
    EXPECT_EQ(b[1], 5);
    EXPECT_EQ(b[2], -7);
}

TEST(Tuple3, Tools)
{
    C3<float> a(-1.25f, 2.75f, -0.5f);
    auto ab = Abs(a);
    EXPECT_FLOAT_EQ(ab[0], 1.25f);
    EXPECT_FLOAT_EQ(ab[1], 2.75f);
    EXPECT_FLOAT_EQ(ab[2], 0.5f);
    C3<float> c(1.2f, 2.9f, -3.1f);
    auto cf = Floor(c);
    EXPECT_FLOAT_EQ(cf[0], 1.0f);
    EXPECT_FLOAT_EQ(cf[1], 2.0f);
    EXPECT_FLOAT_EQ(cf[2], -4.0f);
    auto cc = Ceil(c);
    EXPECT_FLOAT_EQ(cc[0], 2.0f);
    EXPECT_FLOAT_EQ(cc[1], 3.0f);
    EXPECT_FLOAT_EQ(cc[2], -3.0f);
}

TEST(Tuple3, LerpFMA)
{
    C3<float> a(0.0f, 2.0f, -2.0f), b(2.0f, 0.0f, 2.0f);
    auto l = Lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(l[0], 1.0f);
    EXPECT_FLOAT_EQ(l[1], 1.0f);
    EXPECT_FLOAT_EQ(l[2], 0.0f);
    auto f = FMA(a, b, 2.0f);
    EXPECT_FLOAT_EQ(f[0], 2.0f);
    EXPECT_FLOAT_EQ(f[1], 2.0f);
    EXPECT_FLOAT_EQ(f[2], -2.0f);
}

TEST(Tuple3, MinMax)
{
    C3<int> a(1, 5, -2), b(3, 2, 4);
    auto mn = Min(a, b);
    EXPECT_EQ(mn[0], 1);
    EXPECT_EQ(mn[1], 2);
    EXPECT_EQ(mn[2], -2);
    auto mx = Max(a, b);
    EXPECT_EQ(mx[0], 3);
    EXPECT_EQ(mx[1], 5);
    EXPECT_EQ(mx[2], 4);
    EXPECT_EQ(MinValue(a), -2);
    EXPECT_EQ(MinIndex(a), 2);
    EXPECT_EQ(MaxValue(a), 5);
    EXPECT_EQ(MaxIndex(a), 1);
    EXPECT_EQ(Product(a), -10);
}

TEST(Tuple4, Basic)
{
    C4<float> a;
    EXPECT_EQ(a[0], 0);
    EXPECT_EQ(a[1], 0);
    EXPECT_EQ(a[2], 0);
    EXPECT_EQ(a[3], 0);
    C4<float> b(1.0f, -2.0f, 3.5f, -4.5f);
    EXPECT_EQ(b[0], 1.0f);
    EXPECT_EQ(b[1], -2.0f);
    EXPECT_EQ(b[2], 3.5f);
    EXPECT_EQ(b[3], -4.5f);
    EXPECT_FALSE(b.hasNaN());
}

TEST(Tuple4, Arithmetic)
{
    C4<float> a(1.0f, 2.0f, 3.0f, -1.0f), b(4.0f, -1.0f, 0.5f, 2.0f);
    auto c = a + b;
    EXPECT_FLOAT_EQ(c[0], 5.0f);
    EXPECT_FLOAT_EQ(c[1], 1.0f);
    EXPECT_FLOAT_EQ(c[2], 3.5f);
    EXPECT_FLOAT_EQ(c[3], 1.0f);
    auto d = a - b;
    EXPECT_FLOAT_EQ(d[0], -3.0f);
    EXPECT_FLOAT_EQ(d[1], 3.0f);
    EXPECT_FLOAT_EQ(d[2], 2.5f);
    EXPECT_FLOAT_EQ(d[3], -3.0f);
    a += b;
    EXPECT_FLOAT_EQ(a[0], 5.0f);
    EXPECT_FLOAT_EQ(a[1], 1.0f);
    EXPECT_FLOAT_EQ(a[2], 3.5f);
    EXPECT_FLOAT_EQ(a[3], 1.0f);
    a -= b;
    EXPECT_FLOAT_EQ(a[0], 1.0f);
    EXPECT_FLOAT_EQ(a[1], 2.0f);
    EXPECT_FLOAT_EQ(a[2], 3.0f);
    EXPECT_FLOAT_EQ(a[3], -1.0f);
}

TEST(Tuple4, ScalarOps)
{
    C4<float> a(1.0f, -2.0f, 3.0f, -4.0f);
    auto m1 = a * 2.0f;
    EXPECT_FLOAT_EQ(m1[0], 2.0f);
    EXPECT_FLOAT_EQ(m1[1], -4.0f);
    EXPECT_FLOAT_EQ(m1[2], 6.0f);
    EXPECT_FLOAT_EQ(m1[3], -8.0f);
    auto m2 = 2.0f * a;
    EXPECT_FLOAT_EQ(m2[0], 2.0f);
    EXPECT_FLOAT_EQ(m2[1], -4.0f);
    EXPECT_FLOAT_EQ(m2[2], 6.0f);
    EXPECT_FLOAT_EQ(m2[3], -8.0f);
    auto d1 = a / 2.0f;
    EXPECT_FLOAT_EQ(d1[0], 0.5f);
    EXPECT_FLOAT_EQ(d1[1], -1.0f);
    EXPECT_FLOAT_EQ(d1[2], 1.5f);
    EXPECT_FLOAT_EQ(d1[3], -2.0f);
    a *= 3.0f;
    EXPECT_FLOAT_EQ(a[0], 3.0f);
    EXPECT_FLOAT_EQ(a[1], -6.0f);
    EXPECT_FLOAT_EQ(a[2], 9.0f);
    EXPECT_FLOAT_EQ(a[3], -12.0f);
    a /= 3.0f;
    EXPECT_FLOAT_EQ(a[0], 1.0f);
    EXPECT_FLOAT_EQ(a[1], -2.0f);
    EXPECT_FLOAT_EQ(a[2], 3.0f);
    EXPECT_FLOAT_EQ(a[3], -4.0f);
}

TEST(Tuple4, UnaryNeg)
{
    C4<int> a(2, -5, 7, -9);
    auto b = -a;
    EXPECT_EQ(b[0], -2);
    EXPECT_EQ(b[1], 5);
    EXPECT_EQ(b[2], -7);
    EXPECT_EQ(b[3], 9);
}

TEST(Tuple4, Tools)
{
    C4<float> a(-1.25f, 2.75f, -0.5f, 4.1f);
    auto ab = Abs(a);
    EXPECT_FLOAT_EQ(ab[0], 1.25f);
    EXPECT_FLOAT_EQ(ab[1], 2.75f);
    EXPECT_FLOAT_EQ(ab[2], 0.5f);
    EXPECT_FLOAT_EQ(ab[3], 4.1f);
    C4<float> c(1.2f, 2.9f, -3.1f, 0.9f);
    auto cf = Floor(c);
    EXPECT_FLOAT_EQ(cf[0], 1.0f);
    EXPECT_FLOAT_EQ(cf[1], 2.0f);
    EXPECT_FLOAT_EQ(cf[2], -4.0f);
    EXPECT_FLOAT_EQ(cf[3], 0.0f);
    auto cc = Ceil(c);
    EXPECT_FLOAT_EQ(cc[0], 2.0f);
    EXPECT_FLOAT_EQ(cc[1], 3.0f);
    EXPECT_FLOAT_EQ(cc[2], -3.0f);
    EXPECT_FLOAT_EQ(cc[3], 1.0f);
}

TEST(Tuple4, LerpFMA)
{
    C4<float> a(0.0f, 2.0f, -2.0f, 1.0f), b(2.0f, 0.0f, 2.0f, -1.0f);
    auto l = Lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(l[0], 1.0f);
    EXPECT_FLOAT_EQ(l[1], 1.0f);
    EXPECT_FLOAT_EQ(l[2], 0.0f);
    EXPECT_FLOAT_EQ(l[3], 0.0f);
    auto f = FMA(a, b, 2.0f);
    EXPECT_FLOAT_EQ(f[0], 2.0f);
    EXPECT_FLOAT_EQ(f[1], 2.0f);
    EXPECT_FLOAT_EQ(f[2], -2.0f);
    EXPECT_FLOAT_EQ(f[3], 1.0f);
}

TEST(Tuple4, MinMax)
{
    C4<int> a(1, 5, -2, 7), b(3, 2, 4, -1);
    auto mn = Min(a, b);
    EXPECT_EQ(mn[0], 1);
    EXPECT_EQ(mn[1], 2);
    EXPECT_EQ(mn[2], -2);
    EXPECT_EQ(mn[3], -1);
    auto mx = Max(a, b);
    EXPECT_EQ(mx[0], 3);
    EXPECT_EQ(mx[1], 5);
    EXPECT_EQ(mx[2], 4);
    EXPECT_EQ(mx[3], 7);
    EXPECT_EQ(MinValue(a), -2);
    EXPECT_EQ(MinIndex(a), 2);
    EXPECT_EQ(MaxValue(a), 7);
    EXPECT_EQ(MaxIndex(a), 3);
    EXPECT_EQ(Product(a), -70);
}

