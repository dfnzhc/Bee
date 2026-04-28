#include <gtest/gtest.h>

#include <cstring>
#include <limits>

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
    float         orig = 2.0f;
    std::uint16_t bits = bee::float_to_f16_bits(orig);
    float         back = bee::f16_bits_to_float(bits);
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
    auto        src  = bee::Tensor::full({4}, bee::DType::F32, 2.0).value();
    auto        f16  = bee::cast(src, bee::DType::F16).value();
    auto        back = bee::cast(f16, bee::DType::F32).value();
    const auto* p    = static_cast<const float*>(back.data_ptr());
    EXPECT_NEAR(p[0], 2.0f, 1e-3f);
    EXPECT_NEAR(p[3], 2.0f, 1e-3f);
}

TEST(LowPrecisionTests, CastRoundTripBetweenF32AndBF16)
{
    auto        src  = bee::Tensor::full({4}, bee::DType::F32, 3.5).value();
    auto        bf16 = bee::cast(src, bee::DType::BF16).value();
    auto        back = bee::cast(bf16, bee::DType::F32).value();
    const auto* p    = static_cast<const float*>(back.data_ptr());
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
    auto        f32 = bee::cast(t, bee::DType::F32).value();
    const auto* p   = static_cast<const float*>(f32.data_ptr());
    EXPECT_NEAR(p[0], 1.5f, 1e-3f);
}

TEST(LowPrecisionTests, FullWithBF16CreatesCorrectTensor)
{
    auto t = bee::Tensor::full({4}, bee::DType::BF16, 2.5).value();
    EXPECT_EQ(t.dtype(), bee::DType::BF16);
    auto        f32 = bee::cast(t, bee::DType::F32).value();
    const auto* p   = static_cast<const float*>(f32.data_ptr());
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

// ── CUDA 上 F16/BF16 cast（原生低精度路径）──────────────────────────────────

TEST(LowPrecisionTests, CudaCastF32ToF16Native)
{
    // 若无 CUDA，跳过；有 CUDA 时验证 F32→F16 原生路径
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 CUDA low-precision cast 测试";

    auto src_cpu = bee::Tensor::full({4}, bee::DType::F32, 2.0).value();
    auto src_gpu = src_cpu.to(bee::Device::CUDA).value();

    // 在 CUDA 张量上 cast 到 F16（走原生低精度路径）
    auto f16_gpu = bee::cast(src_gpu, bee::DType::F16).value();
    EXPECT_EQ(f16_gpu.dtype(), bee::DType::F16);
    EXPECT_EQ(f16_gpu.device(), bee::Device::CUDA);

    // 搬回 CPU 验证值
    auto        f16_cpu  = f16_gpu.to(bee::Device::CPU).value();
    auto        back_f32 = bee::cast(f16_cpu, bee::DType::F32).value();
    const auto* p        = static_cast<const float*>(back_f32.data_ptr());
    EXPECT_NEAR(p[0], 2.0f, 1e-3f);
    EXPECT_NEAR(p[3], 2.0f, 1e-3f);
}

TEST(LowPrecisionTests, CudaCastF32ToBF16Native)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 CUDA low-precision cast 测试";

    auto src_cpu = bee::Tensor::full({4}, bee::DType::F32, 3.5).value();
    auto src_gpu = src_cpu.to(bee::Device::CUDA).value();

    auto bf16_gpu = bee::cast(src_gpu, bee::DType::BF16).value();
    EXPECT_EQ(bf16_gpu.dtype(), bee::DType::BF16);
    EXPECT_EQ(bf16_gpu.device(), bee::Device::CUDA);

    auto        bf16_cpu = bf16_gpu.to(bee::Device::CPU).value();
    auto        back_f32 = bee::cast(bf16_cpu, bee::DType::F32).value();
    const auto* p        = static_cast<const float*>(back_f32.data_ptr());
    EXPECT_NEAR(p[0], 3.5f, 1e-2f);
}

// ── F16 次规格数舍入边界测试 ────────────────────────────────────────────────

TEST(LowPrecisionTests, F16SubnormalMinPositive)
{
    // 最小正 F16 次规格数 = 2^(-24)；F32 bits = 0x33800000（exp=103, mant=0）
    std::uint32_t bits_f32 = 0x33800000u;
    float         val;
    std::memcpy(&val, &bits_f32, sizeof(val));
    EXPECT_EQ(bee::float_to_f16_bits(val), std::uint16_t{0x0001u});
}

TEST(LowPrecisionTests, F16SubnormalMidpointTieRoundsToEven)
{
    // 0 与最小次规格数之间的中点 tie = 2^(-25)；F32 bits = 0x33000000
    // round-to-even → 0（偶数）
    std::uint32_t bits_f32 = 0x33000000u;
    float         val;
    std::memcpy(&val, &bits_f32, sizeof(val));
    EXPECT_EQ(bee::float_to_f16_bits(val), std::uint16_t{0x0000u});
}

TEST(LowPrecisionTests, F16SubnormalTieN1N2RoundsToEven)
{
    // 1.5 × min_subnormal = 1.5 × 2^(-24) = midpoint 介于 n=1 与 n=2 之间
    // F32 bits = 0x33C00000（exp=103, mant=0x400000）
    // round-to-even → n=2（偶数），即 0x0002
    std::uint32_t bits_f32 = 0x33C00000u;
    float         val;
    std::memcpy(&val, &bits_f32, sizeof(val));
    EXPECT_EQ(bee::float_to_f16_bits(val), std::uint16_t{0x0002u});
}

TEST(LowPrecisionTests, F16SubnormalAboveMidpointRoundsUp)
{
    // 1.5 × 2^(-25) 位于 0 与 min_subnormal 中点之上（sticky ≠ 0）→ 向上舍入至 0x0001
    // F32 bits = 0x33400000（exp=102, mant=0x400000）
    std::uint32_t bits_f32 = 0x33400000u;
    float         val;
    std::memcpy(&val, &bits_f32, sizeof(val));
    EXPECT_EQ(bee::float_to_f16_bits(val), std::uint16_t{0x0001u});
}

TEST(LowPrecisionTests, F16NegativeZeroPreserved)
{
    // -0.0f 在 F16 中应编码为 0x8000
    EXPECT_EQ(bee::float_to_f16_bits(-0.0f), std::uint16_t{0x8000u});
}

TEST(LowPrecisionTests, F16PositiveInfinity)
{
    // +Inf 应编码为 0x7C00
    EXPECT_EQ(bee::float_to_f16_bits(std::numeric_limits<float>::infinity()), std::uint16_t{0x7C00u});
}

// ── promote_types 对称性测试 ─────────────────────────────────────────────────

TEST(LowPrecisionTests, PromoteTypesSymmetry_BoolI8)
{
    // Bool + I8 与 I8 + Bool 结果必须相同（对称性）
    const auto op = bee::DTypeOpKind::ElementWise;
    EXPECT_EQ(bee::promote_types(bee::DType::Bool, bee::DType::I8, op), bee::promote_types(bee::DType::I8, bee::DType::Bool, op));
}

TEST(LowPrecisionTests, PromoteTypesSymmetry_U8I8)
{
    // U8 + I8 与 I8 + U8 结果必须相同
    const auto op = bee::DTypeOpKind::ElementWise;
    EXPECT_EQ(bee::promote_types(bee::DType::U8, bee::DType::I8, op), bee::promote_types(bee::DType::I8, bee::DType::U8, op));
}

TEST(LowPrecisionTests, PromoteTypesBoolI8ResultIsI8)
{
    // Bool 与 I8 混合 → I8（最宽有符号小整型）
    const auto op = bee::DTypeOpKind::ElementWise;
    EXPECT_EQ(bee::promote_types(bee::DType::Bool, bee::DType::I8, op), bee::DType::I8);
    EXPECT_EQ(bee::promote_types(bee::DType::I8, bee::DType::Bool, op), bee::DType::I8);
}

TEST(LowPrecisionTests, PromoteTypesU8I8ResultIsI8)
{
    // U8 与 I8 混合 → I8
    const auto op = bee::DTypeOpKind::ElementWise;
    EXPECT_EQ(bee::promote_types(bee::DType::U8, bee::DType::I8, op), bee::DType::I8);
    EXPECT_EQ(bee::promote_types(bee::DType::I8, bee::DType::U8, op), bee::DType::I8);
}

TEST(LowPrecisionTests, PromoteTypesBoolU8ResultIsU8)
{
    // Bool 与 U8 混合 → U8
    const auto op = bee::DTypeOpKind::ElementWise;
    EXPECT_EQ(bee::promote_types(bee::DType::Bool, bee::DType::U8, op), bee::DType::U8);
    EXPECT_EQ(bee::promote_types(bee::DType::U8, bee::DType::Bool, op), bee::DType::U8);
}

// ── 低精度组合 cast 测试（F32 桥接路线）────────────────────────────────────

TEST(LowPrecisionTests, CastF16ToBF16Bridge)
{
    // F16 → BF16（经 F32 桥接）：值应基本保留
    auto src  = bee::Tensor::full({4}, bee::DType::F32, 1.5).value();
    auto f16  = bee::cast(src, bee::DType::F16).value();
    auto bf16 = bee::cast(f16, bee::DType::BF16).value();
    EXPECT_EQ(bf16.dtype(), bee::DType::BF16);
    auto        back = bee::cast(bf16, bee::DType::F32).value();
    const auto* p    = static_cast<const float*>(back.data_ptr());
    EXPECT_NEAR(p[0], 1.5f, 0.01f);
    EXPECT_NEAR(p[3], 1.5f, 0.01f);
}

TEST(LowPrecisionTests, CastBF16ToF16Bridge)
{
    // BF16 → F16（经 F32 桥接）：值应基本保留
    auto src  = bee::Tensor::full({4}, bee::DType::F32, 2.5).value();
    auto bf16 = bee::cast(src, bee::DType::BF16).value();
    auto f16  = bee::cast(bf16, bee::DType::F16).value();
    EXPECT_EQ(f16.dtype(), bee::DType::F16);
    auto        back = bee::cast(f16, bee::DType::F32).value();
    const auto* p    = static_cast<const float*>(back.data_ptr());
    EXPECT_NEAR(p[0], 2.5f, 0.02f);
    EXPECT_NEAR(p[3], 2.5f, 0.02f);
}

TEST(LowPrecisionTests, CastF16ToI32Bridge)
{
    // F16 → I32（经 F32 桥接）：整数值应精确
    auto src = bee::Tensor::full({2}, bee::DType::F32, 5.0).value();
    auto f16 = bee::cast(src, bee::DType::F16).value();
    auto i32 = bee::cast(f16, bee::DType::I32).value();
    EXPECT_EQ(i32.dtype(), bee::DType::I32);
    const auto* p = static_cast<const int32_t*>(i32.data_ptr());
    EXPECT_EQ(p[0], 5);
    EXPECT_EQ(p[1], 5);
}

TEST(LowPrecisionTests, CastI32ToBF16Bridge)
{
    // I32 → BF16（经 F32 桥接）：值应基本保留
    auto  src = bee::Tensor::zeros({2}, bee::DType::I32).value();
    auto* ip  = static_cast<int32_t*>(src.data_ptr());
    ip[0]     = 3;
    ip[1]     = 7;
    auto bf16 = bee::cast(src, bee::DType::BF16).value();
    EXPECT_EQ(bf16.dtype(), bee::DType::BF16);
    auto        back = bee::cast(bf16, bee::DType::F32).value();
    const auto* fp   = static_cast<const float*>(back.data_ptr());
    EXPECT_NEAR(fp[0], 3.0f, 0.1f);
    EXPECT_NEAR(fp[1], 7.0f, 0.1f);
}

TEST(LowPrecisionTests, CudaCastF16ToBF16ReportsUnsupported)
{
    // CUDA F16 → BF16 不再走 CPU 过渡；该组合等待后续原生 kernel 支持
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    auto cpu_f32 = bee::Tensor::full({4}, bee::DType::F32, 1.5).value();
    auto cpu_f16 = bee::cast(cpu_f32, bee::DType::F16).value();
    auto gpu_f16 = cpu_f16.to(bee::Device::CUDA).value();

    auto gpu_bf16 = bee::cast(gpu_f16, bee::DType::BF16);
    EXPECT_FALSE(gpu_bf16.has_value());
}

// ── F16/BF16 sum 累加类型 ────────────────────────────────────────────────────

TEST(LowPrecisionTests, SumOfF16ReturnsF32Scalar)
{
    // F16 全局 sum 应提升到 F32 累加，返回 F32 标量
    auto src    = bee::cast(bee::Tensor::ones({8}, bee::DType::F32).value(), bee::DType::F16).value();
    auto result = bee::sum(src).value();

    EXPECT_EQ(result.dtype(), bee::DType::F32);
    const auto* p = static_cast<const float*>(result.data_ptr());
    EXPECT_NEAR(p[0], 8.0f, 0.1f);
}

TEST(LowPrecisionTests, SumOfBF16ReturnsF32Scalar)
{
    // BF16 全局 sum 同样提升到 F32 累加
    auto src    = bee::cast(bee::Tensor::ones({4}, bee::DType::F32).value(), bee::DType::BF16).value();
    auto result = bee::sum(src).value();

    EXPECT_EQ(result.dtype(), bee::DType::F32);
    const auto* p = static_cast<const float*>(result.data_ptr());
    EXPECT_NEAR(p[0], 4.0f, 0.1f);
}

// ── 按轴 sum 低精度累加类型 ──────────────────────────────────────────────────

TEST(LowPrecisionTests, AxisSumOfF16ReturnsF32)
{
    // F16 按轴 sum 应提升到 F32，输出 dtype 为 F32，值正确
    // 构造 [2, 3] 全 1 的 F16 张量，沿 dim=1 求和，期望输出 shape=[2]，值均为 3.0f
    auto src_f32 = bee::Tensor::ones({2, 3}, bee::DType::F32).value();
    auto src     = bee::cast(src_f32, bee::DType::F16).value();
    auto result  = bee::sum(src, 1, false).value();

    EXPECT_EQ(result.dtype(), bee::DType::F32);
    EXPECT_EQ(result.shape(), (bee::Shape{2}));
    const auto* p = static_cast<const float*>(result.data_ptr());
    EXPECT_NEAR(p[0], 3.0f, 0.05f);
    EXPECT_NEAR(p[1], 3.0f, 0.05f);
}

TEST(LowPrecisionTests, MeanOfF16ReturnsF32Scalar)
{
    auto src    = bee::cast(bee::Tensor::full({8}, bee::DType::F32, 2.0).value(), bee::DType::F16).value();
    auto result = bee::mean(src).value();

    EXPECT_EQ(result.dtype(), bee::DType::F32);
    const auto* p = static_cast<const float*>(result.data_ptr());
    EXPECT_NEAR(p[0], 2.0f, 0.05f);
}

TEST(LowPrecisionTests, AxisMeanOfBF16ReturnsF32)
{
    auto src_f32 = bee::Tensor::full({2, 4}, bee::DType::F32, 3.0).value();
    auto src     = bee::cast(src_f32, bee::DType::BF16).value();
    auto result  = bee::mean(src, 1, false).value();

    EXPECT_EQ(result.dtype(), bee::DType::F32);
    EXPECT_EQ(result.shape(), (bee::Shape{2}));
    const auto* p = static_cast<const float*>(result.data_ptr());
    EXPECT_NEAR(p[0], 3.0f, 0.05f);
    EXPECT_NEAR(p[1], 3.0f, 0.05f);
}

TEST(LowPrecisionTests, AxisSumOfBF16ReturnsF32WithKeepdim)
{
    // BF16 按轴 sum（keepdim=true）应提升到 F32，shape 保留被 reduce 的维度为 1
    // 构造 [3, 4] 全 1 的 BF16 张量，沿 dim=0 求和（keepdim），期望 shape=[1,4]，值均为 3.0f
    auto src_f32 = bee::Tensor::ones({3, 4}, bee::DType::F32).value();
    auto src     = bee::cast(src_f32, bee::DType::BF16).value();
    auto result  = bee::sum(src, 0, true).value();

    EXPECT_EQ(result.dtype(), bee::DType::F32);
    EXPECT_EQ(result.shape(), (bee::Shape{1, 4}));
    const auto* p = static_cast<const float*>(result.data_ptr());
    for (int i = 0; i < 4; ++i)
        EXPECT_NEAR(p[i], 3.0f, 0.05f);
}

// ── CUDA 原生低精度 matmul：F16/BF16 输入 → F32 输出 ─────────────────────────

TEST(LowPrecisionTests, CudaF16MatmulReturnsF32)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    // A = [[0,1,2],[3,4,5]]（arange 0..5，reshape [2,3]）
    auto a_cpu = bee::Tensor::zeros({2, 3}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(a_cpu.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }
    // B = [[0,1],[2,3],[4,5]]（arange 0..5，reshape [3,2]）
    auto b_cpu = bee::Tensor::zeros({3, 2}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(b_cpu.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }

    // 上传到 CUDA 并 cast 为 F16
    auto a_gpu_f32 = a_cpu.to(bee::Device::CUDA).value();
    auto b_gpu_f32 = b_cpu.to(bee::Device::CUDA).value();
    auto a_gpu     = bee::cast(a_gpu_f32, bee::DType::F16).value();
    auto b_gpu     = bee::cast(b_gpu_f32, bee::DType::F16).value();

    // 调用 matmul（应走原生 CUDA lowp 路径）
    auto result = bee::matmul(a_gpu, b_gpu).value();

    // 输出 dtype 应为 F32，device 应为 CUDA
    EXPECT_EQ(result.dtype(), bee::DType::F32);
    EXPECT_EQ(result.device(), bee::Device::CUDA);
    EXPECT_EQ(result.shape(), (bee::Shape{2, 2}));

    // 搬回 CPU 验证值：C = [[10,13],[28,40]]
    auto        result_cpu = result.to(bee::Device::CPU).value();
    const auto* p          = static_cast<const float*>(result_cpu.data_ptr());
    EXPECT_NEAR(p[0], 10.0f, 1e-2f);
    EXPECT_NEAR(p[1], 13.0f, 1e-2f);
    EXPECT_NEAR(p[2], 28.0f, 1e-2f);
    EXPECT_NEAR(p[3], 40.0f, 1e-2f);
}

TEST(LowPrecisionTests, CudaBF16MatmulReturnsF32)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    // 同 CudaF16MatmulReturnsF32，但使用 BF16 精度
    auto a_cpu = bee::Tensor::zeros({2, 3}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(a_cpu.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }
    auto b_cpu = bee::Tensor::zeros({3, 2}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(b_cpu.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }

    auto a_gpu_f32 = a_cpu.to(bee::Device::CUDA).value();
    auto b_gpu_f32 = b_cpu.to(bee::Device::CUDA).value();
    auto a_gpu     = bee::cast(a_gpu_f32, bee::DType::BF16).value();
    auto b_gpu     = bee::cast(b_gpu_f32, bee::DType::BF16).value();

    auto result = bee::matmul(a_gpu, b_gpu).value();

    EXPECT_EQ(result.dtype(), bee::DType::F32);
    EXPECT_EQ(result.device(), bee::Device::CUDA);
    EXPECT_EQ(result.shape(), (bee::Shape{2, 2}));

    // BF16 精度容忍更大（5e-2）
    auto        result_cpu = result.to(bee::Device::CPU).value();
    const auto* p          = static_cast<const float*>(result_cpu.data_ptr());
    EXPECT_NEAR(p[0], 10.0f, 5e-2f);
    EXPECT_NEAR(p[1], 13.0f, 5e-2f);
    EXPECT_NEAR(p[2], 28.0f, 5e-2f);
    EXPECT_NEAR(p[3], 40.0f, 5e-2f);
}

// ── CUDA 批次低精度 matmul 应返回 Recoverable 错误（第一阶段限制）───────────

TEST(LowPrecisionTests, CudaBatchedF16MatmulReturnsRecoverableError)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    // 构造批次 F16 张量 [2, 2, 2]
    auto a_cpu = bee::Tensor::ones({2, 2, 2}, bee::DType::F32).value();
    auto b_cpu = bee::Tensor::ones({2, 2, 2}, bee::DType::F32).value();

    auto a_gpu_f32 = a_cpu.to(bee::Device::CUDA).value();
    auto b_gpu_f32 = b_cpu.to(bee::Device::CUDA).value();
    auto a_gpu     = bee::cast(a_gpu_f32, bee::DType::F16).value();
    auto b_gpu     = bee::cast(b_gpu_f32, bee::DType::F16).value();

    // 批次低精度 CUDA matmul 第一阶段应返回错误
    auto result = bee::matmul(a_gpu, b_gpu);
    EXPECT_FALSE(result.has_value()) << "CUDA 批次低精度 matmul 应返回错误";
    if (!result.has_value()) {
        // 验证错误消息包含预期提示
        const std::string_view msg = result.error().message;
        EXPECT_NE(msg.find("批次"), std::string_view::npos) << "错误消息应提及批次限制";
    }
}

// ── CPU 低精度 matmul 仍通过 F32 cast 语义正常工作 ────────────────────────────

TEST(LowPrecisionTests, CpuF16MatmulCastToF32)
{
    // CPU 上 F16 matmul 走 cast→F32 递归路径，输出 F32
    auto a = bee::Tensor::zeros({2, 3}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(a.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }
    auto b = bee::Tensor::zeros({3, 2}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(b.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }
    auto af16 = bee::cast(a, bee::DType::F16).value();
    auto bf16 = bee::cast(b, bee::DType::F16).value();

    auto result = bee::matmul(af16, bf16).value();
    EXPECT_EQ(result.dtype(), bee::DType::F32);
    EXPECT_EQ(result.device(), bee::Device::CPU);

    const auto* p = static_cast<const float*>(result.data_ptr());
    EXPECT_NEAR(p[0], 10.0f, 0.05f);
    EXPECT_NEAR(p[1], 13.0f, 0.05f);
    EXPECT_NEAR(p[2], 28.0f, 0.05f);
    EXPECT_NEAR(p[3], 40.0f, 0.05f);
}

TEST(LowPrecisionTests, CpuBF16MatmulCastToF32)
{
    // CPU 上 BF16 matmul 同样走 cast→F32 递归路径，输出 F32
    auto a = bee::Tensor::zeros({2, 3}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(a.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }
    auto b = bee::Tensor::zeros({3, 2}, bee::DType::F32).value();
    {
        auto* p = static_cast<float*>(b.data_ptr());
        for (int i = 0; i < 6; ++i)
            p[i] = static_cast<float>(i);
    }
    auto abf16 = bee::cast(a, bee::DType::BF16).value();
    auto bbf16 = bee::cast(b, bee::DType::BF16).value();

    auto result = bee::matmul(abf16, bbf16).value();
    EXPECT_EQ(result.dtype(), bee::DType::F32);
    EXPECT_EQ(result.device(), bee::Device::CPU);

    const auto* p = static_cast<const float*>(result.data_ptr());
    EXPECT_NEAR(p[0], 10.0f, 0.05f);
    EXPECT_NEAR(p[1], 13.0f, 0.05f);
    EXPECT_NEAR(p[2], 28.0f, 0.05f);
    EXPECT_NEAR(p[3], 40.0f, 0.05f);
}
