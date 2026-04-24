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

// ── promote_types 规则 ───────────────────────────────────────────────────────

TEST(LowPrecisionTests, PromoteTypesF16PlusBF16IsF32)
{
    // F16 与 BF16 混合 → 无论何种算子均提升到 F32
    EXPECT_EQ(bee::promote_types(bee::DType::F16, bee::DType::BF16, bee::DTypeOpKind::ElementWise), bee::DType::F32);
    EXPECT_EQ(bee::promote_types(bee::DType::BF16, bee::DType::F16, bee::DTypeOpKind::ElementWise), bee::DType::F32);
    EXPECT_EQ(bee::promote_types(bee::DType::F16, bee::DType::BF16, bee::DTypeOpKind::Matmul), bee::DType::F32);
}

TEST(LowPrecisionTests, PromoteTypesSameF16ElementWiseKeepsF16)
{
    // 相同类型 + ElementWise → 保持原类型
    EXPECT_EQ(bee::promote_types(bee::DType::F16, bee::DType::F16, bee::DTypeOpKind::ElementWise), bee::DType::F16);
    EXPECT_EQ(bee::promote_types(bee::DType::BF16, bee::DType::BF16, bee::DTypeOpKind::ElementWise), bee::DType::BF16);
}

TEST(LowPrecisionTests, PromoteTypesSameF16MatmulAccumulatesToF32)
{
    // 相同类型 + Matmul → F16/BF16 累加到 F32
    EXPECT_EQ(bee::promote_types(bee::DType::F16, bee::DType::F16, bee::DTypeOpKind::Matmul), bee::DType::F32);
    EXPECT_EQ(bee::promote_types(bee::DType::BF16, bee::DType::BF16, bee::DTypeOpKind::Matmul), bee::DType::F32);
}

TEST(LowPrecisionTests, PromoteTypesF16WithF32IsF32)
{
    // F16/BF16 与 F32 混合 → F32
    EXPECT_EQ(bee::promote_types(bee::DType::F16, bee::DType::F32, bee::DTypeOpKind::ElementWise), bee::DType::F32);
    EXPECT_EQ(bee::promote_types(bee::DType::F32, bee::DType::BF16, bee::DTypeOpKind::ElementWise), bee::DType::F32);
}

TEST(LowPrecisionTests, PromoteTypesIntegerHierarchy)
{
    // 整型提升：I64 > I32 > U8/I8
    EXPECT_EQ(bee::promote_types(bee::DType::I32, bee::DType::I64, bee::DTypeOpKind::ElementWise), bee::DType::I64);
    EXPECT_EQ(bee::promote_types(bee::DType::U8, bee::DType::I32, bee::DTypeOpKind::ElementWise), bee::DType::I32);
    EXPECT_EQ(bee::promote_types(bee::DType::I32, bee::DType::I32, bee::DTypeOpKind::ElementWise), bee::DType::I32);
}

// ── CUDA 上 F16/BF16 cast（CPU 参考过渡路径）────────────────────────────────

TEST(LowPrecisionTests, CudaCastF32ToF16ViaFallback)
{
    // 若无 CUDA，跳过；有 CUDA 时验证 F32→F16 过渡路径
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 CUDA low-precision cast 测试";

    auto src_cpu = bee::Tensor::full({4}, bee::DType::F32, 2.0).value();
    auto src_gpu = src_cpu.to(bee::Device::CUDA).value();

    // 在 CUDA 张量上 cast 到 F16（走 CPU 参考过渡路径）
    auto f16_gpu = bee::cast(src_gpu, bee::DType::F16).value();
    EXPECT_EQ(f16_gpu.dtype(), bee::DType::F16);
    EXPECT_EQ(f16_gpu.device(), bee::Device::CUDA);

    // 搬回 CPU 验证值
    auto f16_cpu  = f16_gpu.to(bee::Device::CPU).value();
    auto back_f32 = bee::cast(f16_cpu, bee::DType::F32).value();
    const auto* p = static_cast<const float*>(back_f32.data_ptr());
    EXPECT_NEAR(p[0], 2.0f, 1e-3f);
    EXPECT_NEAR(p[3], 2.0f, 1e-3f);
}

TEST(LowPrecisionTests, CudaCastF32ToBF16ViaFallback)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 CUDA low-precision cast 测试";

    auto src_cpu = bee::Tensor::full({4}, bee::DType::F32, 3.5).value();
    auto src_gpu = src_cpu.to(bee::Device::CUDA).value();

    auto bf16_gpu = bee::cast(src_gpu, bee::DType::BF16).value();
    EXPECT_EQ(bf16_gpu.dtype(), bee::DType::BF16);
    EXPECT_EQ(bf16_gpu.device(), bee::Device::CUDA);

    auto bf16_cpu = bf16_gpu.to(bee::Device::CPU).value();
    auto back_f32 = bee::cast(bf16_cpu, bee::DType::F32).value();
    const auto* p = static_cast<const float*>(back_f32.data_ptr());
    EXPECT_NEAR(p[0], 3.5f, 1e-2f);
}
