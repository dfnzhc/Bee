#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"
#include "Tensor/Core/LowPrecision.hpp"

using namespace bee;

// ── 低精度类型 CUDA 可计算性规则 ─────────────────────────────────────────────

TEST(LowPrecisionTests, F16AndBF16AreCudaComputable)
{
    EXPECT_TRUE(bee::dtype_is_cuda_computable(bee::DType::F16));
    EXPECT_TRUE(bee::dtype_is_cuda_computable(bee::DType::BF16));
}

TEST(LowPrecisionTests, FP8AndFP4AreNotCudaComputable)
{
    EXPECT_FALSE(bee::dtype_is_cuda_computable(bee::DType::FP8E4M3));
    EXPECT_FALSE(bee::dtype_is_cuda_computable(bee::DType::FP8E5M2));
    EXPECT_FALSE(bee::dtype_is_cuda_computable(bee::DType::FP4));
}

// ── 低精度累加类型规则 ────────────────────────────────────────────────────────

TEST(LowPrecisionTests, MatmulAccumulatesLowPrecisionIntoF32)
{
    EXPECT_EQ(bee::dtype_accumulate_type(bee::DTypeOpKind::Matmul, bee::DType::F16), bee::DType::F32);
    EXPECT_EQ(bee::dtype_accumulate_type(bee::DTypeOpKind::Reduce, bee::DType::BF16), bee::DType::F32);
}

TEST(LowPrecisionTests, ElementWisePreservesLowPrecisionType)
{
    EXPECT_EQ(bee::dtype_accumulate_type(bee::DTypeOpKind::ElementWise, bee::DType::F16), bee::DType::F16);
    EXPECT_EQ(bee::dtype_accumulate_type(bee::DTypeOpKind::ElementWise, bee::DType::BF16), bee::DType::BF16);
}

TEST(LowPrecisionTests, F32AccumulatesAsF32)
{
    EXPECT_EQ(bee::dtype_accumulate_type(bee::DTypeOpKind::Matmul, bee::DType::F32), bee::DType::F32);
    EXPECT_EQ(bee::dtype_accumulate_type(bee::DTypeOpKind::Reduce, bee::DType::F64), bee::DType::F64);
}

// ── 位级转换辅助函数 ─────────────────────────────────────────────────────────

TEST(LowPrecisionTests, F16BitsRoundTripFloat)
{
    // 2.0f 在 F16 中可精确表示
    float          orig = 2.0f;
    std::uint16_t  bits = bee::float_to_f16_bits(orig);
    float          back = bee::f16_bits_to_float(bits);
    EXPECT_NEAR(back, orig, 1e-4f);
}

TEST(LowPrecisionTests, BF16BitsRoundTripFloat)
{
    float         orig = 2.0f;
    std::uint16_t bits = bee::float_to_bf16_bits(orig);
    float         back = bee::bf16_bits_to_float(bits);
    EXPECT_NEAR(back, orig, 1e-4f);
}

TEST(LowPrecisionTests, F16ZeroPreserved)
{
    EXPECT_EQ(bee::float_to_f16_bits(0.0f), std::uint16_t{0});
    EXPECT_EQ(bee::f16_bits_to_float(0), 0.0f);
}

// ── F32↔F16 cast 路径 ────────────────────────────────────────────────────────

TEST(LowPrecisionTests, CastRoundTripBetweenF32AndF16)
{
    auto src  = bee::Tensor::full({4}, bee::DType::F32, 2.0).value();
    auto f16  = bee::cast(src, bee::DType::F16).value();
    auto back = bee::cast(f16, bee::DType::F32).value();
    const auto* p = static_cast<const float*>(back.data_ptr());
    EXPECT_NEAR(p[0], 2.0f, 1e-3f);
    EXPECT_NEAR(p[3], 2.0f, 1e-3f);
}

TEST(LowPrecisionTests, CastRoundTripBetweenF32AndBF16)
{
    auto src  = bee::Tensor::full({4}, bee::DType::F32, 3.5).value();
    auto bf16 = bee::cast(src, bee::DType::BF16).value();
    auto back = bee::cast(bf16, bee::DType::F32).value();
    const auto* p = static_cast<const float*>(back.data_ptr());
    EXPECT_NEAR(p[0], 3.5f, 1e-2f);
    EXPECT_NEAR(p[3], 3.5f, 1e-2f);
}

TEST(LowPrecisionTests, F16TensorHasCorrectDTypeAndShape)
{
    auto src = bee::Tensor::full({2, 4}, bee::DType::F32, 1.0).value();
    auto f16 = bee::cast(src, bee::DType::F16).value();
    EXPECT_EQ(f16.dtype(), bee::DType::F16);
    EXPECT_EQ(f16.numel(), 8);
}

// ── F16/BF16 full() 创建 ─────────────────────────────────────────────────────

TEST(LowPrecisionTests, FullWithF16CreatesCorrectTensor)
{
    auto t = bee::Tensor::full({4}, bee::DType::F16, 1.5).value();
    EXPECT_EQ(t.dtype(), bee::DType::F16);
    EXPECT_EQ(t.numel(), 4);
    // 通过 cast 回 F32 验证值
    auto f32 = bee::cast(t, bee::DType::F32).value();
    const auto* p = static_cast<const float*>(f32.data_ptr());
    EXPECT_NEAR(p[0], 1.5f, 1e-3f);
}

TEST(LowPrecisionTests, FullWithBF16CreatesCorrectTensor)
{
    auto t = bee::Tensor::full({4}, bee::DType::BF16, 2.5).value();
    EXPECT_EQ(t.dtype(), bee::DType::BF16);
    auto f32 = bee::cast(t, bee::DType::F32).value();
    const auto* p = static_cast<const float*>(f32.data_ptr());
    EXPECT_NEAR(p[0], 2.5f, 1e-2f);
}
