#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

using namespace bee;

// 便利宏
#define ASSERT_OK(expr)  ASSERT_TRUE((expr).has_value())
#define ASSERT_ERR(expr) ASSERT_FALSE((expr).has_value())

// ─────────────────────────────────────────────────────────────────────────────
// 辅助：从连续 Tensor 取标量
// ─────────────────────────────────────────────────────────────────────────────

template <typename T>
static auto get(const Tensor& t, int64_t i, int64_t j) -> T
{
    const int64_t N = t.shape()[1];
    return static_cast<const T*>(t.data_ptr())[i * N + j];
}

// ─────────────────────────────────────────────────────────────────────────────
// F32 基础计算值验证
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, F32Basic)
{
    // [[1,2],[3,4]] × [[5,6],[7,8]] = [[19,22],[43,50]]
    auto a = Tensor::empty({2, 2}, DType::F32);
    auto b = Tensor::empty({2, 2}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto* pa = static_cast<float*>(a->data_ptr());
    pa[0] = 1.f; pa[1] = 2.f; pa[2] = 3.f; pa[3] = 4.f;

    auto* pb = static_cast<float*>(b->data_ptr());
    pb[0] = 5.f; pb[1] = 6.f; pb[2] = 7.f; pb[3] = 8.f;

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{2, 2}));
    EXPECT_FLOAT_EQ(get<float>(*c, 0, 0), 19.f);
    EXPECT_FLOAT_EQ(get<float>(*c, 0, 1), 22.f);
    EXPECT_FLOAT_EQ(get<float>(*c, 1, 0), 43.f);
    EXPECT_FLOAT_EQ(get<float>(*c, 1, 1), 50.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// F64 基础计算值验证
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, F64Basic)
{
    // [1,2,3] × [[1],[1],[1]] = [[6]]
    auto a = Tensor::empty({1, 3}, DType::F64);
    auto b = Tensor::empty({3, 1}, DType::F64);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto* pa = static_cast<double*>(a->data_ptr());
    pa[0] = 1.0; pa[1] = 2.0; pa[2] = 3.0;

    auto* pb = static_cast<double*>(b->data_ptr());
    pb[0] = 1.0; pb[1] = 1.0; pb[2] = 1.0;

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{1, 1}));
    EXPECT_DOUBLE_EQ(get<double>(*c, 0, 0), 6.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// I32：单位矩阵 × 矩阵 = 矩阵本身
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, I32IdentityMatmul)
{
    // 3×3 单位矩阵 × [[1,2,3],[4,5,6],[7,8,9]] = 原矩阵
    auto eye = Tensor::zeros({3, 3}, DType::I32);
    ASSERT_OK(eye);
    auto* pe = static_cast<int32_t*>(eye->data_ptr());
    pe[0] = 1; pe[4] = 1; pe[8] = 1;   // 对角线

    auto mat = Tensor::empty({3, 3}, DType::I32);
    ASSERT_OK(mat);
    auto* pm = static_cast<int32_t*>(mat->data_ptr());
    for (int i = 0; i < 9; ++i)
        pm[i] = static_cast<int32_t>(i + 1);

    auto c = matmul(*eye, *mat);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{3, 3}));
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(static_cast<const int32_t*>(c->data_ptr())[i], pm[i]);
}

// ─────────────────────────────────────────────────────────────────────────────
// F32 数值对拍：随机矩阵 vs 标量三重循环参考实现
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, F32NumericalAccuracy)
{
    constexpr int M = 17, K = 13, N = 11;

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);

    std::vector<float> ra(M * K), rb(K * N), rc_ref(M * N, 0.f);
    for (auto& x : ra) x = dist(rng);
    for (auto& x : rb) x = dist(rng);

    // 标量参考实现
    for (int i = 0; i < M; ++i)
        for (int k = 0; k < K; ++k)
            for (int j = 0; j < N; ++j)
                rc_ref[i * N + j] += ra[i * K + k] * rb[k * N + j];

    // 构造 Tensor
    auto a = Tensor::empty({M, K}, DType::F32);
    auto b = Tensor::empty({K, N}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);
    std::memcpy(a->data_ptr(), ra.data(), ra.size() * sizeof(float));
    std::memcpy(b->data_ptr(), rb.data(), rb.size() * sizeof(float));

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{M, N}));

    const auto* pc = static_cast<const float*>(c->data_ptr());
    for (int i = 0; i < M * N; ++i)
        EXPECT_NEAR(pc[i], rc_ref[i], 1e-4f);
}

// ─────────────────────────────────────────────────────────────────────────────
// 错误路径：shape 不匹配
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, ErrShapeMismatch)
{
    auto a = Tensor::zeros({3, 4}, DType::F32);
    auto b = Tensor::zeros({5, 2}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    ASSERT_ERR(matmul(*a, *b));
}

// ─────────────────────────────────────────────────────────────────────────────
// 错误路径：dtype 不匹配
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, ErrDtypeMismatch)
{
    auto a = Tensor::zeros({2, 3}, DType::F32);
    auto b = Tensor::zeros({3, 2}, DType::F64);
    ASSERT_OK(a);
    ASSERT_OK(b);

    ASSERT_ERR(matmul(*a, *b));
}

// ─────────────────────────────────────────────────────────────────────────────
// 错误路径：非 2D（1D / 3D）
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, ErrNonTwoD_1D)
{
    auto a = Tensor::zeros({4}, DType::F32);
    auto b = Tensor::zeros({4, 2}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    ASSERT_ERR(matmul(*a, *b));
}

TEST(MatmulTests, ErrNonTwoD_3D)
{
    auto a = Tensor::zeros({2, 3, 4}, DType::F32);
    auto b = Tensor::zeros({4, 2}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    ASSERT_ERR(matmul(*a, *b));
}

// ─────────────────────────────────────────────────────────────────────────────
// 错误路径：Bool / U8 不支持
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, ErrBoolDtype)
{
    auto a = Tensor::zeros({2, 3}, DType::Bool);
    auto b = Tensor::zeros({3, 2}, DType::Bool);
    ASSERT_OK(a);
    ASSERT_OK(b);

    ASSERT_ERR(matmul(*a, *b));
}

TEST(MatmulTests, ErrU8Dtype)
{
    auto a = Tensor::zeros({2, 3}, DType::U8);
    auto b = Tensor::zeros({3, 2}, DType::U8);
    ASSERT_OK(a);
    ASSERT_OK(b);

    ASSERT_ERR(matmul(*a, *b));
}

// ─────────────────────────────────────────────────────────────────────────────
// 非 contiguous 输入（transpose 后 matmul）与 contiguous 结果一致
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, NonContiguousInput)
{
    // 用单次 transpose 制造非连续视图
    // a_cont: [2,3]，a_t = a_cont.T => [3,2]（stride=[1,3]，非连续）
    // b_cont: [3,2]，b_t = b_cont.T => [2,3]（stride=[1,3]，非连续）
    // 验证：matmul(a_t, b_t) == matmul(a_t 连续化, b_t 连续化)
    auto a_cont = Tensor::empty({2, 3}, DType::F32);
    auto b_cont = Tensor::empty({3, 2}, DType::F32);
    ASSERT_OK(a_cont);
    ASSERT_OK(b_cont);

    auto* pa = static_cast<float*>(a_cont->data_ptr());
    pa[0] = 1.f; pa[1] = 2.f; pa[2] = 3.f;
    pa[3] = 4.f; pa[4] = 5.f; pa[5] = 6.f;

    auto* pb = static_cast<float*>(b_cont->data_ptr());
    pb[0] = 7.f;  pb[1] = 8.f;
    pb[2] = 9.f;  pb[3] = 10.f;
    pb[4] = 11.f; pb[5] = 12.f;

    // a_t: [3,2] 非连续
    auto a_t = a_cont->transpose(0, 1);
    ASSERT_OK(a_t);
    EXPECT_FALSE(a_t->is_contiguous());

    // b_t: [2,3] 非连续
    auto b_t = b_cont->transpose(0, 1);
    ASSERT_OK(b_t);
    EXPECT_FALSE(b_t->is_contiguous());

    // 参考：先连续化再 matmul
    auto a_t_c = a_t->contiguous();
    auto b_t_c = b_t->contiguous();
    ASSERT_OK(a_t_c);
    ASSERT_OK(b_t_c);
    auto ref = matmul(*a_t_c, *b_t_c);
    ASSERT_OK(ref);

    // 非连续输入 matmul（内部自动 contiguous 整理）
    auto res = matmul(*a_t, *b_t);
    ASSERT_OK(res);
    EXPECT_EQ(res->shape(), ref->shape());

    const auto* pr  = static_cast<const float*>(ref->data_ptr());
    const auto* prs = static_cast<const float*>(res->data_ptr());
    for (int64_t i = 0; i < ref->numel(); ++i)
        EXPECT_FLOAT_EQ(prs[i], pr[i]);
}

// ─────────────────────────────────────────────────────────────────────────────
// 空维度：K==0 → 输出全 0
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, KZeroOutputAllZero)
{
    auto a = Tensor::empty({3, 0}, DType::F32);
    auto b = Tensor::empty({0, 4}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{3, 4}));
    EXPECT_EQ(c->numel(), 12);

    const auto* pc = static_cast<const float*>(c->data_ptr());
    for (int i = 0; i < 12; ++i)
        EXPECT_FLOAT_EQ(pc[i], 0.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// 空维度：M==0 → 形状正确的空张量
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, MZeroShape)
{
    auto a = Tensor::empty({0, 4}, DType::F32);
    auto b = Tensor::empty({4, 3}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{0, 3}));
    EXPECT_EQ(c->numel(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// 空维度：N==0 → 形状正确的空张量
// ─────────────────────────────────────────────────────────────────────────────

TEST(MatmulTests, NZeroShape)
{
    auto a = Tensor::empty({5, 2}, DType::F32);
    auto b = Tensor::empty({2, 0}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{5, 0}));
    EXPECT_EQ(c->numel(), 0);
}
