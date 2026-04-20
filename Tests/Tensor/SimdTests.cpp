#include <gtest/gtest.h>

#include "Tensor/Cpu/Simd/Simd.hpp"

#include <cmath>
#include <cstdint>

using namespace bee::simd;

// =====================================================================
// Scalar 后端测试（不依赖任何 SIMD 宏，始终编译）
// =====================================================================

TEST(SimdScalar, WidthIsOne)
{
    EXPECT_EQ((SimdBackend<float,   IsaScalar>::width), 1u);
    EXPECT_EQ((SimdBackend<double,  IsaScalar>::width), 1u);
    EXPECT_EQ((SimdBackend<int32_t, IsaScalar>::width), 1u);
    EXPECT_EQ((SimdBackend<int64_t, IsaScalar>::width), 1u);
    EXPECT_EQ((SimdBackend<uint8_t, IsaScalar>::width), 1u);
}

TEST(SimdScalar, Float_LoadStore)
{
    using B = SimdBackend<float, IsaScalar>;
    float src = 3.14f;
    float dst = 0.0f;
    B::store(&dst, B::load(&src));
    EXPECT_FLOAT_EQ(dst, 3.14f);
}

TEST(SimdScalar, Double_LoadStore)
{
    using B = SimdBackend<double, IsaScalar>;
    double src = 2.718281828;
    double dst = 0.0;
    B::storeu(&dst, B::loadu(&src));
    EXPECT_DOUBLE_EQ(dst, 2.718281828);
}

TEST(SimdScalar, I32_LoadStore)
{
    using B = SimdBackend<int32_t, IsaScalar>;
    int32_t src = -42;
    int32_t dst = 0;
    B::store(&dst, B::load(&src));
    EXPECT_EQ(dst, -42);
}

TEST(SimdScalar, I64_LoadStore)
{
    using B = SimdBackend<int64_t, IsaScalar>;
    int64_t src = 9000000000LL;
    int64_t dst = 0;
    B::store(&dst, B::load(&src));
    EXPECT_EQ(dst, 9000000000LL);
}

TEST(SimdScalar, Float_Set1)
{
    using B = SimdBackend<float, IsaScalar>;
    auto r = B::set1(7.5f);
    EXPECT_FLOAT_EQ(r, 7.5f);
}

TEST(SimdScalar, Float_AddSubMulDiv)
{
    using B = SimdBackend<float, IsaScalar>;
    EXPECT_FLOAT_EQ(B::add(B::set1(3.0f), B::set1(2.0f)), 5.0f);
    EXPECT_FLOAT_EQ(B::sub(B::set1(3.0f), B::set1(2.0f)), 1.0f);
    EXPECT_FLOAT_EQ(B::mul(B::set1(3.0f), B::set1(2.0f)), 6.0f);
    EXPECT_FLOAT_EQ(B::div(B::set1(6.0f), B::set1(2.0f)), 3.0f);
}

TEST(SimdScalar, Float_MinMax)
{
    using B = SimdBackend<float, IsaScalar>;
    EXPECT_FLOAT_EQ(B::min(B::set1(1.0f), B::set1(2.0f)), 1.0f);
    EXPECT_FLOAT_EQ(B::max(B::set1(1.0f), B::set1(2.0f)), 2.0f);
}

TEST(SimdScalar, Float_NegAbs)
{
    using B = SimdBackend<float, IsaScalar>;
    EXPECT_FLOAT_EQ(B::neg(B::set1(5.0f)), -5.0f);
    EXPECT_FLOAT_EQ(B::abs(B::set1(-5.0f)), 5.0f);
}

TEST(SimdScalar, Float_Sqrt)
{
    using B = SimdBackend<float, IsaScalar>;
    EXPECT_NEAR(B::sqrt(B::set1(9.0f)), 3.0f, 1e-6f);
}

TEST(SimdScalar, Float_ExpLog)
{
    using B = SimdBackend<float, IsaScalar>;
    EXPECT_NEAR(B::exp(B::set1(0.0f)), 1.0f, 1e-6f);
    EXPECT_NEAR(B::log(B::set1(1.0f)), 0.0f, 1e-6f);
}

TEST(SimdScalar, Float_ReduceSum)
{
    using B = SimdBackend<float, IsaScalar>;
    EXPECT_FLOAT_EQ(B::reduce_sum(B::set1(4.0f)), 4.0f);
    EXPECT_FLOAT_EQ(B::reduce_min(B::set1(4.0f)), 4.0f);
    EXPECT_FLOAT_EQ(B::reduce_max(B::set1(4.0f)), 4.0f);
}

TEST(SimdScalar, I32_AddSubMulDiv)
{
    using B = SimdBackend<int32_t, IsaScalar>;
    EXPECT_EQ(B::add(B::set1(10), B::set1(3)), 13);
    EXPECT_EQ(B::sub(B::set1(10), B::set1(3)), 7);
    EXPECT_EQ(B::mul(B::set1(4),  B::set1(5)), 20);
    EXPECT_EQ(B::div(B::set1(10), B::set1(2)), 5);
}

TEST(SimdScalar, I32_NegAbs)
{
    using B = SimdBackend<int32_t, IsaScalar>;
    EXPECT_EQ(B::neg(B::set1(7)), -7);
    EXPECT_EQ(B::abs(B::set1(-7)), 7);
}

TEST(SimdScalar, U8_AddSubMinMax)
{
    using B = SimdBackend<uint8_t, IsaScalar>;
    EXPECT_EQ(B::add(B::set1(100), B::set1(50)), uint8_t(150));
    EXPECT_EQ(B::sub(B::set1(100), B::set1(50)), uint8_t(50));
    EXPECT_EQ(B::min(B::set1(10),  B::set1(20)), uint8_t(10));
    EXPECT_EQ(B::max(B::set1(10),  B::set1(20)), uint8_t(20));
}

// =====================================================================
// AVX2 后端测试（需要 BEE_TENSOR_SIMD_AVX2）
// =====================================================================
#ifdef BEE_TENSOR_SIMD_AVX2

TEST(SimdAvx2, Widths)
{
    EXPECT_EQ((SimdBackend<float,   IsaAvx2>::width), 8u);
    EXPECT_EQ((SimdBackend<double,  IsaAvx2>::width), 4u);
    EXPECT_EQ((SimdBackend<int32_t, IsaAvx2>::width), 8u);
    EXPECT_EQ((SimdBackend<int64_t, IsaAvx2>::width), 4u);
    EXPECT_EQ((SimdBackend<uint8_t, IsaAvx2>::width), 32u);
}

TEST(SimdAvx2, Float_LoadStoreu)
{
    using B = SimdBackend<float, IsaAvx2>;
    // 非对齐缓冲区
    float src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    float dst[8] = {};
    B::storeu(dst, B::loadu(src));
    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ(dst[i], static_cast<float>(i + 1));
}

TEST(SimdAvx2, Float_Set1)
{
    using B = SimdBackend<float, IsaAvx2>;
    alignas(32) float buf[8];
    B::store(buf, B::set1(99.0f));
    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ(buf[i], 99.0f);
}

TEST(SimdAvx2, Float_AddSubMulDiv)
{
    using B = SimdBackend<float, IsaAvx2>;
    auto a = B::set1(6.0f);
    auto b = B::set1(2.0f);
    alignas(32) float buf[8];

    B::store(buf, B::add(a, b));
    EXPECT_FLOAT_EQ(buf[0], 8.0f);

    B::store(buf, B::sub(a, b));
    EXPECT_FLOAT_EQ(buf[0], 4.0f);

    B::store(buf, B::mul(a, b));
    EXPECT_FLOAT_EQ(buf[0], 12.0f);

    B::store(buf, B::div(a, b));
    EXPECT_FLOAT_EQ(buf[0], 3.0f);
}

TEST(SimdAvx2, Float_MinMax)
{
    using B = SimdBackend<float, IsaAvx2>;
    alignas(32) float buf[8];
    B::store(buf, B::min(B::set1(3.0f), B::set1(5.0f)));
    EXPECT_FLOAT_EQ(buf[0], 3.0f);
    B::store(buf, B::max(B::set1(3.0f), B::set1(5.0f)));
    EXPECT_FLOAT_EQ(buf[0], 5.0f);
}

TEST(SimdAvx2, Float_NegAbs)
{
    using B = SimdBackend<float, IsaAvx2>;
    alignas(32) float buf[8];
    B::store(buf, B::neg(B::set1(4.0f)));
    EXPECT_FLOAT_EQ(buf[0], -4.0f);
    B::store(buf, B::abs(B::set1(-4.0f)));
    EXPECT_FLOAT_EQ(buf[0], 4.0f);
}

TEST(SimdAvx2, Float_Sqrt)
{
    using B = SimdBackend<float, IsaAvx2>;
    alignas(32) float buf[8];
    B::store(buf, B::sqrt(B::set1(16.0f)));
    EXPECT_NEAR(buf[0], 4.0f, 1e-5f);
}

TEST(SimdAvx2, Float_ReduceSum)
{
    using B = SimdBackend<float, IsaAvx2>;
    // {1,2,3,4,5,6,7,8} → 36
    alignas(32) float src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto v = B::load(src);
    EXPECT_NEAR(B::reduce_sum(v), 36.0f, 1e-4f);
}

TEST(SimdAvx2, Float_ReduceMinMax)
{
    using B = SimdBackend<float, IsaAvx2>;
    alignas(32) float src[8] = {5, 2, 8, 1, 7, 3, 9, 4};
    auto v = B::load(src);
    EXPECT_FLOAT_EQ(B::reduce_min(v), 1.0f);
    EXPECT_FLOAT_EQ(B::reduce_max(v), 9.0f);
}

TEST(SimdAvx2, Double_LoadStore)
{
    using B = SimdBackend<double, IsaAvx2>;
    double src[4] = {1.0, 2.0, 3.0, 4.0};
    double dst[4] = {};
    B::storeu(dst, B::loadu(src));
    for (int i = 0; i < 4; ++i)
        EXPECT_DOUBLE_EQ(dst[i], static_cast<double>(i + 1));
}

TEST(SimdAvx2, Double_ReduceSum)
{
    using B = SimdBackend<double, IsaAvx2>;
    alignas(32) double src[4] = {1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(B::reduce_sum(B::load(src)), 10.0, 1e-12);
}

TEST(SimdAvx2, Double_ReduceMinMax)
{
    using B = SimdBackend<double, IsaAvx2>;
    alignas(32) double src[4] = {3.0, 1.0, 4.0, 2.0};
    auto v = B::load(src);
    EXPECT_DOUBLE_EQ(B::reduce_min(v), 1.0);
    EXPECT_DOUBLE_EQ(B::reduce_max(v), 4.0);
}

TEST(SimdAvx2, I32_LoadStore)
{
    using B = SimdBackend<int32_t, IsaAvx2>;
    alignas(32) int32_t src[8] = {1, -2, 3, -4, 5, -6, 7, -8};
    alignas(32) int32_t dst[8] = {};
    B::store(dst, B::load(src));
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(dst[i], src[i]);
}

TEST(SimdAvx2, I32_AddSub)
{
    using B = SimdBackend<int32_t, IsaAvx2>;
    auto a = B::set1(10);
    auto b = B::set1(3);
    alignas(32) int32_t buf[8];
    B::store(buf, B::add(a, b));
    EXPECT_EQ(buf[0], 13);
    B::store(buf, B::sub(a, b));
    EXPECT_EQ(buf[0], 7);
}

TEST(SimdAvx2, I32_MinMax)
{
    using B = SimdBackend<int32_t, IsaAvx2>;
    alignas(32) int32_t buf[8];
    B::store(buf, B::min(B::set1(-5), B::set1(3)));
    EXPECT_EQ(buf[0], -5);
    B::store(buf, B::max(B::set1(-5), B::set1(3)));
    EXPECT_EQ(buf[0], 3);
}

TEST(SimdAvx2, I32_NegAbs)
{
    using B = SimdBackend<int32_t, IsaAvx2>;
    alignas(32) int32_t buf[8];
    B::store(buf, B::neg(B::set1(9)));
    EXPECT_EQ(buf[0], -9);
    B::store(buf, B::abs(B::set1(-9)));
    EXPECT_EQ(buf[0], 9);
}

TEST(SimdAvx2, I32_ReduceSum)
{
    using B = SimdBackend<int32_t, IsaAvx2>;
    alignas(32) int32_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    EXPECT_EQ(B::reduce_sum(B::load(src)), 36);
}

TEST(SimdAvx2, I32_ReduceMinMax)
{
    using B = SimdBackend<int32_t, IsaAvx2>;
    alignas(32) int32_t src[8] = {4, -1, 7, 2, -3, 8, 0, 5};
    auto v = B::load(src);
    EXPECT_EQ(B::reduce_min(v), -3);
    EXPECT_EQ(B::reduce_max(v), 8);
}

TEST(SimdAvx2, I64_LoadStore)
{
    using B = SimdBackend<int64_t, IsaAvx2>;
    alignas(32) int64_t src[4] = {100LL, -200LL, 300LL, -400LL};
    alignas(32) int64_t dst[4] = {};
    B::store(dst, B::load(src));
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(dst[i], src[i]);
}

TEST(SimdAvx2, I64_AddSub)
{
    using B = SimdBackend<int64_t, IsaAvx2>;
    alignas(32) int64_t buf[4];
    B::store(buf, B::add(B::set1(5LL), B::set1(3LL)));
    EXPECT_EQ(buf[0], 8LL);
    B::store(buf, B::sub(B::set1(5LL), B::set1(3LL)));
    EXPECT_EQ(buf[0], 2LL);
}

TEST(SimdAvx2, I64_NegAbs)
{
    using B = SimdBackend<int64_t, IsaAvx2>;
    alignas(32) int64_t buf[4];
    B::store(buf, B::neg(B::set1(42LL)));
    EXPECT_EQ(buf[0], -42LL);
    B::store(buf, B::abs(B::set1(-42LL)));
    EXPECT_EQ(buf[0], 42LL);
}

TEST(SimdAvx2, I64_ReduceSum)
{
    using B = SimdBackend<int64_t, IsaAvx2>;
    alignas(32) int64_t src[4] = {10LL, 20LL, 30LL, 40LL};
    EXPECT_EQ(B::reduce_sum(B::load(src)), 100LL);
}

TEST(SimdAvx2, U8_LoadStore)
{
    using B = SimdBackend<uint8_t, IsaAvx2>;
    // 使用 32 字节对齐缓冲
    alignas(32) uint8_t src[32];
    alignas(32) uint8_t dst[32] = {};
    for (int i = 0; i < 32; ++i)
        src[i] = static_cast<uint8_t>(i);
    B::store(dst, B::load(src));
    for (int i = 0; i < 32; ++i)
        EXPECT_EQ(dst[i], static_cast<uint8_t>(i));
}

TEST(SimdAvx2, U8_AddSub)
{
    using B = SimdBackend<uint8_t, IsaAvx2>;
    alignas(32) uint8_t buf[32];
    B::store(buf, B::add(B::set1(100), B::set1(50)));
    EXPECT_EQ(buf[0], uint8_t(150));
    B::store(buf, B::sub(B::set1(200), B::set1(50)));
    EXPECT_EQ(buf[0], uint8_t(150));
}

TEST(SimdAvx2, U8_MinMax)
{
    using B = SimdBackend<uint8_t, IsaAvx2>;
    alignas(32) uint8_t buf[32];
    B::store(buf, B::min(B::set1(10), B::set1(20)));
    EXPECT_EQ(buf[0], uint8_t(10));
    B::store(buf, B::max(B::set1(10), B::set1(20)));
    EXPECT_EQ(buf[0], uint8_t(20));
}

TEST(SimdAvx2, U8_ReduceSum)
{
    using B = SimdBackend<uint8_t, IsaAvx2>;
    // 32 个值全为 1，总和为 32，截断到 uint8_t 仍为 32
    alignas(32) uint8_t src[32];
    for (int i = 0; i < 32; ++i)
        src[i] = 1;
    EXPECT_EQ(B::reduce_sum(B::load(src)), uint8_t(32));
}

#endif // BEE_TENSOR_SIMD_AVX2

// =====================================================================
// Selector 测试
// =====================================================================

TEST(SimdSelector, DefaultIsaName)
{
#ifdef BEE_TENSOR_SIMD_AVX2
    EXPECT_STREQ(bee::simd::default_isa_name(), "Avx2");
#else
    EXPECT_STREQ(bee::simd::default_isa_name(), "Scalar");
#endif
}

#ifdef BEE_TENSOR_SIMD_AVX2
TEST(SimdSelector, DefaultIsaIsAvx2)
{
    // DefaultIsa 应为 IsaAvx2；验证其 float backend 宽度为 8
    EXPECT_EQ((SimdBackend<float, bee::simd::DefaultIsa>::width), 8u);
}
#else
TEST(SimdSelector, DefaultIsaIsScalar)
{
    // DefaultIsa 应为 IsaScalar；验证其 float backend 宽度为 1
    EXPECT_EQ((SimdBackend<float, bee::simd::DefaultIsa>::width), 1u);
}
#endif
