#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

using namespace bee;

// ─────────────────────────────────────────────────────────────────────────────
// 批次/N-D matmul 形状推导
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, ThreeDimensionalInputsProduceExpectedOutputShape)
{
    auto a = bee::Tensor::ones({2, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 5}));
}

TEST(BatchedMatmulTests, BroadcastedBatchDimensionIsSupported)
{
    auto a = bee::Tensor::ones({1, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 5}));
}

TEST(BatchedMatmulTests, FourDimensionalInputsProduceExpectedShape)
{
    auto a = bee::Tensor::ones({2, 3, 4, 5}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 3, 5, 6}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 4, 6}));
}

TEST(BatchedMatmulTests, BatchBroadcastBothDimensions)
{
    // a: (2, 1, 3, 4), b: (1, 3, 4, 5) → (2, 3, 3, 5)
    auto a = bee::Tensor::ones({2, 1, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({1, 3, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 3, 5}));
}

// ─────────────────────────────────────────────────────────────────────────────
// 数值正确性验证
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, ThreeDMatmulNumericalCorrectness)
{
    // ones(2,3,4) × ones(2,4,5) → 每元素 = 4.0（K=4 个 1×1 累加）
    auto a = bee::Tensor::ones({2, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{2, 3, 5}));
    const auto* ptr = static_cast<const float*>(c.data_ptr());
    for (int i = 0; i < 2 * 3 * 5; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 4.0f) << "i=" << i;
}

TEST(BatchedMatmulTests, BroadcastedBatchNumericalCorrectness)
{
    // a: (1,3,4) × b: (2,4,5) → 每元素 = 4.0
    auto a = bee::Tensor::ones({1, 3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    const auto* ptr = static_cast<const float*>(c.data_ptr());
    for (int i = 0; i < 2 * 3 * 5; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 4.0f) << "i=" << i;
}

// ─────────────────────────────────────────────────────────────────────────────
// 回归：2D 路径不受影响
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, TwoDimensionalInputsStillWork)
{
    auto a = bee::Tensor::ones({3, 4}, bee::DType::F32).value();
    auto b = bee::Tensor::ones({4, 5}, bee::DType::F32).value();
    auto c = bee::matmul(a, b).value();

    EXPECT_EQ(c.shape(), (bee::Shape{3, 5}));
    const auto* ptr = static_cast<const float*>(c.data_ptr());
    for (int i = 0; i < 3 * 5; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 4.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// 错误路径：1D 输入仍应拒绝
// ─────────────────────────────────────────────────────────────────────────────

TEST(BatchedMatmulTests, OneDimensionalInputIsRejected)
{
    auto a_r = bee::Tensor::ones({4}, bee::DType::F32);
    auto b_r = bee::Tensor::ones({4, 5}, bee::DType::F32);
    ASSERT_TRUE(a_r.has_value());
    ASSERT_TRUE(b_r.has_value());

    // ndim=1 < 2，应返回错误
    auto c = bee::matmul(*a_r, *b_r);
    EXPECT_FALSE(c.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// 低精度 matmul：F16/BF16 输入 → 应自动提升至 F32 并返回 F32 结果
// ─────────────────────────────────────────────────────────────────────────────

// F16 2D matmul：输出 dtype 应为 F32，数值应正确
TEST(BatchedMatmulTests, F16MatmulOutputIsF32AndNumericallyCorrect)
{
    // ones(2,3) × ones(3,4) → 每元素 = 3.0（K=3）
    auto af32 = bee::Tensor::ones({2, 3}, bee::DType::F32).value();
    auto bf32 = bee::Tensor::ones({3, 4}, bee::DType::F32).value();
    auto a    = bee::cast(af32, bee::DType::F16).value();
    auto b    = bee::cast(bf32, bee::DType::F16).value();

    auto c = bee::matmul(a, b);
    ASSERT_TRUE(c.has_value()) << "F16 matmul 应当成功";

    // 输出 dtype 必须为 F32（与 dtype_accumulate_type(Matmul, F16)==F32 一致）
    EXPECT_EQ(c->dtype(), bee::DType::F32);
    EXPECT_EQ(c->shape(), (bee::Shape{2, 4}));

    const auto* p = static_cast<const float*>(c->data_ptr());
    for (int i = 0; i < 2 * 4; ++i)
        EXPECT_NEAR(p[i], 3.0f, 0.05f) << "i=" << i;
}

// BF16 2D matmul：输出 dtype 应为 F32，数值应正确
TEST(BatchedMatmulTests, BF16MatmulOutputIsF32AndNumericallyCorrect)
{
    // ones(3,2) × ones(2,5) → 每元素 = 2.0（K=2）
    auto af32 = bee::Tensor::ones({3, 2}, bee::DType::F32).value();
    auto bf32 = bee::Tensor::ones({2, 5}, bee::DType::F32).value();
    auto a    = bee::cast(af32, bee::DType::BF16).value();
    auto b    = bee::cast(bf32, bee::DType::BF16).value();

    auto c = bee::matmul(a, b);
    ASSERT_TRUE(c.has_value()) << "BF16 matmul 应当成功";

    EXPECT_EQ(c->dtype(), bee::DType::F32);
    EXPECT_EQ(c->shape(), (bee::Shape{3, 5}));

    const auto* p = static_cast<const float*>(c->data_ptr());
    for (int i = 0; i < 3 * 5; ++i)
        EXPECT_NEAR(p[i], 2.0f, 0.05f) << "i=" << i;
}

// F16 batched matmul：3D 输入，输出 dtype 应为 F32
TEST(BatchedMatmulTests, F16BatchedMatmulOutputIsF32)
{
    // ones(2,3,4) × ones(2,4,5) → shape=(2,3,5)，每元素=4.0，dtype=F32
    auto af32 = bee::Tensor::ones({2, 3, 4}, bee::DType::F32).value();
    auto bf32 = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto a    = bee::cast(af32, bee::DType::F16).value();
    auto b    = bee::cast(bf32, bee::DType::F16).value();

    auto c = bee::matmul(a, b);
    ASSERT_TRUE(c.has_value()) << "F16 batched matmul 应当成功";

    EXPECT_EQ(c->dtype(), bee::DType::F32);
    EXPECT_EQ(c->shape(), (bee::Shape{2, 3, 5}));

    const auto* p = static_cast<const float*>(c->data_ptr());
    for (int i = 0; i < 2 * 3 * 5; ++i)
        EXPECT_NEAR(p[i], 4.0f, 0.1f) << "i=" << i;
}

// BF16 broadcast batched matmul：输出 dtype 应为 F32
TEST(BatchedMatmulTests, BF16BroadcastBatchedMatmulOutputIsF32)
{
    // ones(1,3,4) × ones(2,4,5) → shape=(2,3,5)，每元素=4.0，dtype=F32
    auto af32 = bee::Tensor::ones({1, 3, 4}, bee::DType::F32).value();
    auto bf32 = bee::Tensor::ones({2, 4, 5}, bee::DType::F32).value();
    auto a    = bee::cast(af32, bee::DType::BF16).value();
    auto b    = bee::cast(bf32, bee::DType::BF16).value();

    auto c = bee::matmul(a, b);
    ASSERT_TRUE(c.has_value()) << "BF16 broadcast batched matmul 应当成功";

    EXPECT_EQ(c->dtype(), bee::DType::F32);
    EXPECT_EQ(c->shape(), (bee::Shape{2, 3, 5}));

    const auto* p = static_cast<const float*>(c->data_ptr());
    for (int i = 0; i < 2 * 3 * 5; ++i)
        EXPECT_NEAR(p[i], 4.0f, 0.1f) << "i=" << i;
}

// CUDA 上 F16 matmul 不应把低精度直接落入不支持的后端
TEST(BatchedMatmulTests, F16MatmulOnCudaUpcastsToF32)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    auto af32 = bee::Tensor::ones({2, 3}, bee::DType::F32).value();
    auto bf32 = bee::Tensor::ones({3, 4}, bee::DType::F32).value();
    auto a_cpu = bee::cast(af32, bee::DType::F16).value();
    auto b_cpu = bee::cast(bf32, bee::DType::F16).value();
    auto a     = a_cpu.to(bee::Device::CUDA).value();
    auto b     = b_cpu.to(bee::Device::CUDA).value();

    auto c = bee::matmul(a, b);
    ASSERT_TRUE(c.has_value()) << "CUDA F16 matmul 应当成功（经 F32 桥接）";

    // 输出设备应为 CUDA，dtype 应为 F32
    EXPECT_EQ(c->device(), bee::Device::CUDA);
    EXPECT_EQ(c->dtype(), bee::DType::F32);
    EXPECT_EQ(c->shape(), (bee::Shape{2, 4}));

    // 搬回 CPU 验证数值
    auto c_cpu = c->to(bee::Device::CPU).value();
    const auto* p = static_cast<const float*>(c_cpu.data_ptr());
    for (int i = 0; i < 2 * 4; ++i)
        EXPECT_NEAR(p[i], 3.0f, 0.1f) << "i=" << i;
}
