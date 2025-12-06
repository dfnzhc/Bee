/**
 * @File TestTuple.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/6
 * @Brief This file is part of Bee.
 */

#include "Check.hpp"
#include <Bee/Bee.hpp>

using namespace bee;

template <typename T>
struct C2 : Tuple<C2, T, 2>
{
    using Base = Tuple<C2, T, 2>;
    using Base::Base;
    using Base::operator==;
    using Base::operator!=;
};

template <typename T>
struct C3 : Tuple<C3, T, 3>
{
    using Base = Tuple<C3, T, 3>;
    using Base::Base;
    using Base::operator==;
    using Base::operator!=;
};

template <typename T>
struct C4 : Tuple<C4, T, 4>
{
    using Base = Tuple<C4, T, 4>;
    using Base::Base;
    using Base::operator==;
    using Base::operator!=;
};

BEE_TEST_KERNAL(TestTupleKernal)
{
    {
        C2<f32> a(1.5f, -2.0f);
        EXPECT_EQ(a[0], 1.5f);
        EXPECT_EQ(a[1], -2.0f);
        EXPECT_FALSE(a.hasNaN());
    }

    
    {
        C2<int> a(1, 2), b(1, 2), c(2, 3);
        EXPECT_TRUE(a == b);
        EXPECT_FALSE(a != b);
        EXPECT_TRUE(a != c);
        EXPECT_EQ(a[0], 1);
        EXPECT_EQ(a[1], 2);
    }

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

    {
        C2<int> a(2, -5);
        auto b = -a;
        EXPECT_EQ(b[0], -2);
        EXPECT_EQ(b[1], 5);
    }

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
}

BEE_DEFINE_TEST(TestTuple);