#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cstdint>

using namespace bee;

// 测试辅助宏
#define ASSERT_OK(expr)  ASSERT_TRUE((expr).has_value())
#define ASSERT_ERR(expr) ASSERT_FALSE((expr).has_value())

// ─────────────────────────────────────────────────────────────────────────────
// 1. I64 矩阵乘法：单位矩阵 × 目标矩阵 = 目标矩阵
// ─────────────────────────────────────────────────────────────────────────────

TEST(IntegrationTests, I64IdentityMatmul)
{
    // 2×2 I64 单位矩阵
    auto eye = Tensor::zeros({2, 2}, DType::I64);
    ASSERT_OK(eye);
    auto* pe = static_cast<int64_t*>(eye->data_ptr());
    pe[0]    = 1LL;
    pe[3]    = 1LL;

    // 目标矩阵 [[10, 20], [30, 40]]
    auto mat = Tensor::empty({2, 2}, DType::I64);
    ASSERT_OK(mat);
    auto* pm = static_cast<int64_t*>(mat->data_ptr());
    pm[0]    = 10LL;
    pm[1]    = 20LL;
    pm[2]    = 30LL;
    pm[3]    = 40LL;

    auto c = matmul(*eye, *mat);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{2, 2}));

    const auto* pc = static_cast<const int64_t*>(c->data_ptr());
    EXPECT_EQ(pc[0], 10LL);
    EXPECT_EQ(pc[1], 20LL);
    EXPECT_EQ(pc[2], 30LL);
    EXPECT_EQ(pc[3], 40LL);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. 链式操作：arange → reshape → transpose → contiguous 数据正确
// ─────────────────────────────────────────────────────────────────────────────

TEST(IntegrationTests, ChainArangeReshapeTransposeContiguous)
{
    // arange(0,12) → reshape({3,4}) → transpose(0,1) → contiguous()
    // 原始矩阵（行优先）:
    //   [[0,1,2,3],[4,5,6,7],[8,9,10,11]]
    // 转置后（4×3 逻辑视图）:
    //   [[0,4,8],[1,5,9],[2,6,10],[3,7,11]]
    // contiguous() 后内存布局也应满足此顺序
    auto ar = Tensor::arange(0, 12);
    ASSERT_OK(ar);

    auto rs = ar->reshape({3, 4});
    ASSERT_OK(rs);

    auto tr = rs->transpose(0, 1);
    ASSERT_OK(tr);
    EXPECT_EQ(tr->shape(), (Shape{4, 3}));
    EXPECT_FALSE(tr->is_contiguous());

    auto cont = tr->contiguous();
    ASSERT_OK(cont);
    EXPECT_TRUE(cont->is_contiguous());
    EXPECT_EQ(cont->shape(), (Shape{4, 3}));

    // 验证逐元素：转置后 (i,j) 对应原始 (j,i)，即值为 j*4+i
    const auto* p   = static_cast<const int64_t*>(cont->data_ptr());
    int         idx = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            int64_t expected = static_cast<int64_t>(j * 4 + i);
            EXPECT_EQ(p[idx++], expected) << "i=" << i << " j=" << j;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. cast + matmul：I32 cast F32 后做 matmul，与直接 F32 结果一致
// ─────────────────────────────────────────────────────────────────────────────

TEST(IntegrationTests, CastThenMatmulMatchesNativeF32)
{
    // 构造 2×3 I32 矩阵 A 和 3×2 I32 矩阵 B
    auto ai32 = Tensor::empty({2, 3}, DType::I32);
    auto bi32 = Tensor::empty({3, 2}, DType::I32);
    ASSERT_OK(ai32);
    ASSERT_OK(bi32);

    auto* pa = static_cast<int32_t*>(ai32->data_ptr());
    pa[0]    = 1;
    pa[1]    = 2;
    pa[2]    = 3;
    pa[3]    = 4;
    pa[4]    = 5;
    pa[5]    = 6;

    auto* pb = static_cast<int32_t*>(bi32->data_ptr());
    pb[0]    = 7;
    pb[1]    = 8;
    pb[2]    = 9;
    pb[3]    = 10;
    pb[4]    = 11;
    pb[5]    = 12;

    // cast 到 F32
    auto af32 = cast(*ai32, DType::F32);
    auto bf32 = cast(*bi32, DType::F32);
    ASSERT_OK(af32);
    ASSERT_OK(bf32);

    // cast 后 matmul
    auto c_cast = matmul(*af32, *bf32);
    ASSERT_OK(c_cast);

    // 直接用 F32 做 matmul（与 cast 路径对比）
    auto af32_direct = Tensor::empty({2, 3}, DType::F32);
    auto bf32_direct = Tensor::empty({3, 2}, DType::F32);
    ASSERT_OK(af32_direct);
    ASSERT_OK(bf32_direct);

    auto* paf = static_cast<float*>(af32_direct->data_ptr());
    paf[0]    = 1.f;
    paf[1]    = 2.f;
    paf[2]    = 3.f;
    paf[3]    = 4.f;
    paf[4]    = 5.f;
    paf[5]    = 6.f;

    auto* pbf = static_cast<float*>(bf32_direct->data_ptr());
    pbf[0]    = 7.f;
    pbf[1]    = 8.f;
    pbf[2]    = 9.f;
    pbf[3]    = 10.f;
    pbf[4]    = 11.f;
    pbf[5]    = 12.f;

    auto c_direct = matmul(*af32_direct, *bf32_direct);
    ASSERT_OK(c_direct);

    EXPECT_EQ(c_cast->shape(), c_direct->shape());

    const auto* pc_cast   = static_cast<const float*>(c_cast->data_ptr());
    const auto* pc_direct = static_cast<const float*>(c_direct->data_ptr());
    for (int64_t i = 0; i < c_direct->numel(); ++i)
        EXPECT_FLOAT_EQ(pc_cast[i], pc_direct[i]) << "元素 " << i << " 不一致";
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. reduce + elementwise：sum(add(a, b)) == sum(a) + sum(b)（F32 小张量）
// ─────────────────────────────────────────────────────────────────────────────

TEST(IntegrationTests, SumAddEqualsSumPlusSum)
{
    // a = [1,2,3,4], b = [5,6,7,8]
    auto a = Tensor::empty({4}, DType::F32);
    auto b = Tensor::empty({4}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto* pa = static_cast<float*>(a->data_ptr());
    pa[0]    = 1.f;
    pa[1]    = 2.f;
    pa[2]    = 3.f;
    pa[3]    = 4.f;

    auto* pb = static_cast<float*>(b->data_ptr());
    pb[0]    = 5.f;
    pb[1]    = 6.f;
    pb[2]    = 7.f;
    pb[3]    = 8.f;

    // sum(add(a, b))
    auto ab = add(*a, *b);
    ASSERT_OK(ab);
    auto sum_ab = sum(*ab);
    ASSERT_OK(sum_ab);

    // sum(a) + sum(b)（标量相加用 add）
    auto sa = sum(*a);
    auto sb = sum(*b);
    ASSERT_OK(sa);
    ASSERT_OK(sb);
    auto sa_plus_sb = add(*sa, *sb);
    ASSERT_OK(sa_plus_sb);

    const float lhs = *static_cast<const float*>(sum_ab->data_ptr());
    const float rhs = *static_cast<const float*>(sa_plus_sb->data_ptr());
    EXPECT_FLOAT_EQ(lhs, rhs);
    // 预期值：(1+5)+(2+6)+(3+7)+(4+8) = 36
    EXPECT_FLOAT_EQ(lhs, 36.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. rand 不同 seed 给出不同值
// ─────────────────────────────────────────────────────────────────────────────

TEST(IntegrationTests, RandDifferentSeedsGiveDifferentValues)
{
    auto r1 = rand({16}, DType::F32, 1);
    auto r2 = rand({16}, DType::F32, 2);
    ASSERT_OK(r1);
    ASSERT_OK(r2);

    const auto* p1 = static_cast<const float*>(r1->data_ptr());
    const auto* p2 = static_cast<const float*>(r2->data_ptr());

    // 16 个元素中至少有一个不同
    bool any_different = false;
    for (int i = 0; i < 16; ++i) {
        if (p1[i] != p2[i]) {
            any_different = true;
            break;
        }
    }
    EXPECT_TRUE(any_different) << "seed=1 与 seed=2 产生了完全相同的序列";
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. 0-rank 标量 clone 保留数据
// ─────────────────────────────────────────────────────────────────────────────

TEST(IntegrationTests, ScalarClonePreservesData)
{
    // 创建 0-rank F32 标量并写入已知值
    auto s = Tensor::empty({}, DType::F32);
    ASSERT_OK(s);
    EXPECT_EQ(s->ndim(), 0);
    EXPECT_EQ(s->numel(), 1);

    *static_cast<float*>(s->data_ptr()) = 3.14f;

    auto c = s->clone();
    ASSERT_OK(c);
    EXPECT_EQ(c->ndim(), 0);
    EXPECT_EQ(c->numel(), 1);
    // storage 独立
    EXPECT_NE(s->data_ptr(), c->data_ptr());
    // 数值一致
    EXPECT_FLOAT_EQ(*static_cast<const float*>(c->data_ptr()), 3.14f);
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. I64 手算 matmul 数值验证（非单位矩阵）
// ─────────────────────────────────────────────────────────────────────────────

TEST(IntegrationTests, I64MatmulNumerical)
{
    // [[1,2],[3,4]] × [[2,0],[1,2]] = [[4,4],[10,8]]
    auto a = Tensor::empty({2, 2}, DType::I64);
    auto b = Tensor::empty({2, 2}, DType::I64);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto* pa = static_cast<int64_t*>(a->data_ptr());
    pa[0]    = 1LL;
    pa[1]    = 2LL;
    pa[2]    = 3LL;
    pa[3]    = 4LL;

    auto* pb = static_cast<int64_t*>(b->data_ptr());
    pb[0]    = 2LL;
    pb[1]    = 0LL;
    pb[2]    = 1LL;
    pb[3]    = 2LL;

    auto c = matmul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{2, 2}));

    const auto* pc = static_cast<const int64_t*>(c->data_ptr());
    EXPECT_EQ(pc[0], 4LL);  // 1*2+2*1=4
    EXPECT_EQ(pc[1], 4LL);  // 1*0+2*2=4
    EXPECT_EQ(pc[2], 10LL); // 3*2+4*1=10
    EXPECT_EQ(pc[3], 8LL);  // 3*0+4*2=8
}
