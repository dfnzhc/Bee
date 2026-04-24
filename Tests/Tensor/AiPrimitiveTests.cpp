#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "Tensor/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

using namespace bee;

// ── Embedding ─────────────────────────────────────────────────────────────────

// 基本形状正确性：2 个 id，每行 4 维，输出应为 {2, 4}
TEST(AiPrimitiveTests, EmbeddingReturnsExpectedRows)
{
    auto weight = Tensor::arange(0, 12, 1, DType::F32).value().reshape({3, 4}).value();
    auto ids    = Tensor::full({2}, DType::I64, 1.0).value();
    auto out    = bee::embedding(weight, ids).value();

    EXPECT_EQ(out.shape(), (Shape{2, 4}));
}

// 数值正确性：ids=[0,2] → 取 weight 第 0、2 行
TEST(AiPrimitiveTests, EmbeddingValuesCorrect)
{
    // weight: [[0,1,2,3],[4,5,6,7],[8,9,10,11]]
    auto weight = Tensor::arange(0, 12, 1, DType::F32).value().reshape({3, 4}).value();

    // ids=[0,2]
    auto ids = Tensor::zeros({2}, DType::I64).value();
    // ids[0]=0（已是 0），ids[1]=2
    auto* p_ids = static_cast<int64_t*>(ids.data_ptr());
    p_ids[0]    = 0;
    p_ids[1]    = 2;

    auto out = bee::embedding(weight, ids).value();
    ASSERT_EQ(out.shape(), (Shape{2, 4}));

    const auto* p = static_cast<const float*>(out.data_ptr());
    // 第 0 行 = [0,1,2,3]
    EXPECT_FLOAT_EQ(p[0], 0.0f);
    EXPECT_FLOAT_EQ(p[1], 1.0f);
    EXPECT_FLOAT_EQ(p[2], 2.0f);
    EXPECT_FLOAT_EQ(p[3], 3.0f);
    // 第 1 行 = [8,9,10,11]
    EXPECT_FLOAT_EQ(p[4], 8.0f);
    EXPECT_FLOAT_EQ(p[5], 9.0f);
    EXPECT_FLOAT_EQ(p[6], 10.0f);
    EXPECT_FLOAT_EQ(p[7], 11.0f);
}

// 越界 id 应返回错误
TEST(AiPrimitiveTests, EmbeddingOutOfBoundReturnsError)
{
    auto weight = Tensor::arange(0, 12, 1, DType::F32).value().reshape({3, 4}).value();
    auto ids    = Tensor::full({1}, DType::I64, 5.0).value(); // id=5 超出 vocab=3

    auto out = bee::embedding(weight, ids);
    EXPECT_FALSE(out.has_value());
}

// ── RMSNorm ───────────────────────────────────────────────────────────────────

// 基本形状不变
TEST(AiPrimitiveTests, RmsNormPreservesInputShape)
{
    auto x = Tensor::ones({2, 8}, DType::F32).value();
    auto w = Tensor::ones({8}, DType::F32).value();
    auto y = bee::rms_norm(x, w, 1e-5).value();

    EXPECT_EQ(y.shape(), x.shape());
}

// 数值正确性：全 1 输入、全 1 权重 → 全 1 输出
TEST(AiPrimitiveTests, RmsNormOnesInputOnesWeight)
{
    auto x = Tensor::ones({2, 4}, DType::F32).value();
    auto w = Tensor::ones({4}, DType::F32).value();
    auto y = bee::rms_norm(x, w, 1e-5).value();

    const auto* p = static_cast<const float*>(y.data_ptr());
    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR(p[i], 1.0f, 1e-5f);
}

// weight shape 不匹配应返回错误
TEST(AiPrimitiveTests, RmsNormWeightShapeMismatchReturnsError)
{
    auto x   = Tensor::ones({2, 8}, DType::F32).value();
    auto bad = Tensor::ones({4}, DType::F32).value(); // 与 x 最后维 8 不匹配

    auto y = bee::rms_norm(x, bad, 1e-5);
    EXPECT_FALSE(y.has_value());
}

// ── RoPE ──────────────────────────────────────────────────────────────────────

// 基本形状不变
TEST(AiPrimitiveTests, RopePreservesTensorShape)
{
    auto q = Tensor::ones({1, 2, 4, 8}, DType::F32).value();
    auto r = bee::apply_rope(q, 10000.0, 0).value();

    EXPECT_EQ(r.shape(), q.shape());
}

// 奇数最后维应返回错误
TEST(AiPrimitiveTests, RopeOddLastDimReturnsError)
{
    auto q = Tensor::ones({1, 2, 4, 7}, DType::F32).value(); // 7 为奇数

    auto r = bee::apply_rope(q, 10000.0, 0);
    EXPECT_FALSE(r.has_value());
}

// position_offset=0、base=10000 时前两列数值验证
// 对 dim=8，第 i 对的 theta_i = 1/(10000^(2i/8))
// 位置 0：cos(0)=1, sin(0)=0，旋转后 (x0, x1) = (x0, x1)
// 所以全 1 输入在 position_offset=0 时输出应为：
//   (cos*1 - sin*1, sin*1 + cos*1)
TEST(AiPrimitiveTests, RopePosition0OutputCorrect)
{
    // shape={1,1,1,2}：最简情形，只有一对 (x0, x1) = (1, 1)
    // theta_0 = 1/(10000^0) = 1，position=0 → angle=0
    // 旋转：(cos(0)*1 - sin(0)*1, sin(0)*1 + cos(0)*1) = (1, 1)
    auto q   = Tensor::ones({1, 1, 1, 2}, DType::F32).value();
    auto out = bee::apply_rope(q, 10000.0, 0).value();

    const auto* p = static_cast<const float*>(out.data_ptr());
    EXPECT_NEAR(p[0], 1.0f, 1e-5f);
    EXPECT_NEAR(p[1], 1.0f, 1e-5f);
}

// ─── 以下为修复 Task6 复审问题新增的测试 ───────────────────────────────────────

// 1. embedding 拒绝非 2-D weight
TEST(AiPrimitiveTests, EmbeddingRejectsNon2DWeight)
{
    // 3-D weight：原实现允许（ndim >= 2），修复后须拒绝
    auto weight3d = Tensor::arange(0, 24, 1, DType::F32).value().reshape({2, 3, 4}).value();
    auto ids      = Tensor::full({2}, DType::I64, 0.0).value();
    auto out      = bee::embedding(weight3d, ids);
    EXPECT_FALSE(out.has_value());
}

// 2. embedding 在 CUDA 可用时拒绝跨设备输入（cpu weight + cuda ids）
TEST(AiPrimitiveTests, EmbeddingRejectsMixedDeviceInputs)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过混合设备测试";

    auto weight  = Tensor::arange(0, 12, 1, DType::F32).value().reshape({3, 4}).value(); // CPU
    auto ids_gpu = Tensor::full({2}, DType::I64, 0.0, Device::CUDA).value();             // CUDA

    auto out = bee::embedding(weight, ids_gpu);
    EXPECT_FALSE(out.has_value());
}

// 3. rms_norm 在 CUDA 可用时拒绝跨设备输入（cuda x + cpu weight）
TEST(AiPrimitiveTests, RmsNormRejectsMixedDeviceInputs)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过混合设备测试";

    auto x_gpu = Tensor::ones({2, 4}, DType::F32, Device::CUDA).value(); // CUDA
    auto w_cpu = Tensor::ones({4}, DType::F32).value();                  // CPU

    auto y = bee::rms_norm(x_gpu, w_cpu, 1e-5);
    EXPECT_FALSE(y.has_value());
}

// 4. rms_norm 拒绝 eps=0
TEST(AiPrimitiveTests, RmsNormRejectsZeroEps)
{
    auto x = Tensor::ones({2, 4}, DType::F32).value();
    auto w = Tensor::ones({4}, DType::F32).value();
    auto y = bee::rms_norm(x, w, 0.0);
    EXPECT_FALSE(y.has_value());
}

// 5. rms_norm 拒绝 eps < 0
TEST(AiPrimitiveTests, RmsNormRejectsNegativeEps)
{
    auto x = Tensor::ones({2, 4}, DType::F32).value();
    auto w = Tensor::ones({4}, DType::F32).value();
    auto y = bee::rms_norm(x, w, -1e-5);
    EXPECT_FALSE(y.has_value());
}

// 6. apply_rope 拒绝 base <= 0
TEST(AiPrimitiveTests, RopeRejectsNonPositiveBase)
{
    auto q = Tensor::ones({1, 2, 4}, DType::F32).value();
    EXPECT_FALSE(bee::apply_rope(q, 0.0, 0).has_value());
    EXPECT_FALSE(bee::apply_rope(q, -1.0, 0).has_value());
}

// 7. position_offset != 0 的数值验证（锁定 split-half 配对约定）
//
// 约定：split-half，即 (x[i], x[i + dim/2]) 为一对。
// 参数：shape={1,1,4}，全 1 输入，position_offset=1，base=10000
//
// pos = 1，dim=4，half_dim=2
//   i=0: theta = 1 / 10000^(0/4) = 1.0
//         x0=x[0]=1, x1=x[2]=1
//         out[0] = cos(1)-sin(1)，out[2] = sin(1)+cos(1)
//   i=1: theta = 1 / 10000^(2/4) = 0.01
//         x0=x[1]=1, x1=x[3]=1
//         out[1] = cos(0.01)-sin(0.01)，out[3] = sin(0.01)+cos(0.01)
TEST(AiPrimitiveTests, RopePositionOffsetProducesExpectedRotation)
{
    auto q   = Tensor::ones({1, 1, 4}, DType::F32).value();
    auto out = bee::apply_rope(q, 10000.0, 1).value();

    const auto* p = static_cast<const float*>(out.data_ptr());

    // i=0 对：theta=1.0
    const double c0 = std::cos(1.0);
    const double s0 = std::sin(1.0);
    EXPECT_NEAR(p[0], static_cast<float>(c0 - s0), 1e-5f); // out[0]
    EXPECT_NEAR(p[2], static_cast<float>(s0 + c0), 1e-5f); // out[2]

    // i=1 对：theta=0.01
    const double c1 = std::cos(0.01);
    const double s1 = std::sin(0.01);
    EXPECT_NEAR(p[1], static_cast<float>(c1 - s1), 1e-5f); // out[1]
    EXPECT_NEAR(p[3], static_cast<float>(s1 + c1), 1e-5f); // out[3]
}

// ─── 第二轮修复：零维 / NaN / CUDA 正向路径 ───────────────────────────────────

// rms_norm 拒绝最后维为 0（若不显式检查则 n_rows = 0/0 会除零崩溃）
TEST(AiPrimitiveTests, RmsNormRejectsZeroLastDim)
{
    auto x = Tensor::empty({2, 0}, DType::F32).value();
    auto w = Tensor::empty({0}, DType::F32).value();
    auto y = bee::rms_norm(x, w, 1e-5);
    EXPECT_FALSE(y.has_value());
}

// apply_rope 拒绝 seq_len 为 0
TEST(AiPrimitiveTests, RopeRejectsZeroSeqLen)
{
    auto q = Tensor::empty({1, 0, 4}, DType::F32).value();
    auto r = bee::apply_rope(q, 10000.0, 0);
    EXPECT_FALSE(r.has_value());
}

// apply_rope 拒绝最后维为 0（dim=0 为偶数，不会被 dim%2 检查拦截）
TEST(AiPrimitiveTests, RopeRejectsZeroLastDim)
{
    auto q = Tensor::empty({1, 2, 0}, DType::F32).value();
    auto r = bee::apply_rope(q, 10000.0, 0);
    EXPECT_FALSE(r.has_value());
}

// rms_norm 拒绝 NaN eps（NaN <= 0 为 false，须额外判断 isfinite）
TEST(AiPrimitiveTests, RmsNormRejectsNaNEps)
{
    auto x = Tensor::ones({2, 4}, DType::F32).value();
    auto w = Tensor::ones({4}, DType::F32).value();
    auto y = bee::rms_norm(x, w, std::numeric_limits<double>::quiet_NaN());
    EXPECT_FALSE(y.has_value());
}

// apply_rope 拒绝 NaN base
TEST(AiPrimitiveTests, RopeRejectsNaNBase)
{
    auto q = Tensor::ones({1, 1, 4}, DType::F32).value();
    auto r = bee::apply_rope(q, std::numeric_limits<double>::quiet_NaN(), 0);
    EXPECT_FALSE(r.has_value());
}

// embedding CUDA 正向路径：两端都在 CUDA，结果设备为 CUDA，值与 CPU 一致
TEST(AiPrimitiveTests, EmbeddingCudaInputsReturnCudaAndMatchCpu)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 CUDA 正向测试";

    // 构造 CPU 参考
    auto w_cpu   = Tensor::arange(0, 12, 1, DType::F32).value().reshape({3, 4}).value();
    auto ids_cpu = Tensor::zeros({2}, DType::I64).value();
    auto* p_ids  = static_cast<int64_t*>(ids_cpu.data_ptr());
    p_ids[0]     = 0;
    p_ids[1]     = 2;
    auto ref = bee::embedding(w_cpu, ids_cpu).value();

    // CUDA 路径
    auto w_cuda   = w_cpu.to(Device::CUDA).value();
    auto ids_cuda = ids_cpu.to(Device::CUDA).value();
    auto out_cuda = bee::embedding(w_cuda, ids_cuda).value();

    ASSERT_EQ(out_cuda.device(), Device::CUDA);
    ASSERT_EQ(out_cuda.shape(), ref.shape());

    // 搬回 CPU 对比数值
    auto        out_back = out_cuda.to(Device::CPU).value();
    const auto* pa       = static_cast<const float*>(ref.data_ptr());
    const auto* pb       = static_cast<const float*>(out_back.data_ptr());
    for (int64_t i = 0; i < ref.numel(); ++i)
        EXPECT_FLOAT_EQ(pb[i], pa[i]) << "index=" << i;
}

// rms_norm CUDA 正向路径：两端都在 CUDA，结果设备为 CUDA，值与 CPU 一致
TEST(AiPrimitiveTests, RmsNormCudaInputsReturnCudaAndMatchCpu)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 CUDA 正向测试";

    // 构造 CPU 参考（非平凡输入，便于验证数值不全为 1）
    auto x_cpu = Tensor::arange(1, 9, 1, DType::F32).value().reshape({2, 4}).value();
    auto w_cpu = Tensor::ones({4}, DType::F32).value();
    auto ref   = bee::rms_norm(x_cpu, w_cpu, 1e-5).value();

    // CUDA 路径
    auto x_cuda   = x_cpu.to(Device::CUDA).value();
    auto w_cuda   = w_cpu.to(Device::CUDA).value();
    auto out_cuda = bee::rms_norm(x_cuda, w_cuda, 1e-5).value();

    ASSERT_EQ(out_cuda.device(), Device::CUDA);
    ASSERT_EQ(out_cuda.shape(), ref.shape());

    auto        out_back = out_cuda.to(Device::CPU).value();
    const auto* pa       = static_cast<const float*>(ref.data_ptr());
    const auto* pb       = static_cast<const float*>(out_back.data_ptr());
    for (int64_t i = 0; i < ref.numel(); ++i)
        EXPECT_NEAR(pb[i], pa[i], 1e-5f) << "index=" << i;
}

// apply_rope CUDA 正向路径：输入在 CUDA，结果设备为 CUDA，值与 CPU 一致
TEST(AiPrimitiveTests, RopeCudaInputReturnsCudaAndMatchesCpu)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 CUDA 正向测试";

    // 构造 CPU 参考
    auto q_cpu = Tensor::ones({1, 2, 4}, DType::F32).value();
    auto ref   = bee::apply_rope(q_cpu, 10000.0, 0).value();

    // CUDA 路径
    auto q_cuda   = q_cpu.to(Device::CUDA).value();
    auto out_cuda = bee::apply_rope(q_cuda, 10000.0, 0).value();

    ASSERT_EQ(out_cuda.device(), Device::CUDA);
    ASSERT_EQ(out_cuda.shape(), ref.shape());

    auto        out_back = out_cuda.to(Device::CPU).value();
    const auto* pa       = static_cast<const float*>(ref.data_ptr());
    const auto* pb       = static_cast<const float*>(out_back.data_ptr());
    for (int64_t i = 0; i < ref.numel(); ++i)
        EXPECT_NEAR(pb[i], pa[i], 1e-5f) << "index=" << i;
}
