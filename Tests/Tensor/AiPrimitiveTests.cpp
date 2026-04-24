#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

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
