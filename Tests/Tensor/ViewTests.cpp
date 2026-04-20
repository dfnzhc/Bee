#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

using namespace bee;

// ═══════════════════════════════════════════════════════════════
// view
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, ViewBasicShape)
{
    // {2,3,4} → {6,4}：共享 storage，strides 为 contiguous
    auto r = Tensor::empty({2, 3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto vr = r->view({6, 4});
    ASSERT_TRUE(vr.has_value());

    EXPECT_EQ(vr->shape(), (Shape{6, 4}));
    EXPECT_EQ(vr->strides(), (Strides{4, 1}));
    EXPECT_EQ(vr->storage().get(), r->storage().get()); // 零拷贝
    EXPECT_EQ(vr->storage_offset(), 0);
}

TEST(ViewTests, ViewFlatten)
{
    // {2,3,4} → {24}
    auto r = Tensor::empty({2, 3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto vr = r->view({24});
    ASSERT_TRUE(vr.has_value());
    EXPECT_EQ(vr->shape(), (Shape{24}));
    EXPECT_EQ(vr->strides(), (Strides{1}));
}

TEST(ViewTests, ViewInferNeg1Single)
{
    // {24} → {-1} 推断为 {24}
    auto r = Tensor::empty({24}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto vr = r->view({-1});
    ASSERT_TRUE(vr.has_value());
    EXPECT_EQ(vr->shape(), (Shape{24}));
}

TEST(ViewTests, ViewInferNeg1WithOther)
{
    // {24} → {2,-1} 推断为 {2,12}
    auto r = Tensor::empty({24}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto vr = r->view({2, -1});
    ASSERT_TRUE(vr.has_value());
    EXPECT_EQ(vr->shape(), (Shape{2, 12}));
}

TEST(ViewTests, ViewNumelMismatchError)
{
    // 元素数不匹配 → Err
    auto r = Tensor::empty({2, 3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto vr = r->view({5, 5});
    EXPECT_FALSE(vr.has_value());
}

TEST(ViewTests, ViewNonContiguousError)
{
    // 非 contiguous（transpose 后）→ view 应返回 Err
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto tr = r->transpose(0, 1);
    ASSERT_TRUE(tr.has_value());
    EXPECT_FALSE(tr->is_contiguous());

    auto vr = tr->view({12});
    EXPECT_FALSE(vr.has_value());
}

// ═══════════════════════════════════════════════════════════════
// reshape
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, ReshapeContiguousSharesStorage)
{
    // contiguous 输入：与 view 等价，共享 storage
    auto r = Tensor::empty({2, 3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto rr = r->reshape({6, 4});
    ASSERT_TRUE(rr.has_value());

    EXPECT_EQ(rr->shape(), (Shape{6, 4}));
    EXPECT_EQ(rr->storage().get(), r->storage().get());
}

TEST(ViewTests, ReshapeNonContiguousIndependentStorage)
{
    // 非 contiguous：先 clone，storage 独立
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto tr = r->transpose(0, 1); // shape {4,3}，非 contiguous
    ASSERT_TRUE(tr.has_value());
    EXPECT_FALSE(tr->is_contiguous());

    auto rr = tr->reshape({12});
    ASSERT_TRUE(rr.has_value());

    EXPECT_EQ(rr->shape(), (Shape{12}));
    EXPECT_TRUE(rr->is_contiguous());
    EXPECT_NE(rr->storage().get(), r->storage().get()); // storage 独立
}

// ═══════════════════════════════════════════════════════════════
// contiguous
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, ContiguousAlreadyContiguousSharesStorage)
{
    // 已连续：返回共享 storage 的拷贝
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto cr = r->contiguous();
    ASSERT_TRUE(cr.has_value());

    EXPECT_EQ(cr->storage().get(), r->storage().get());
    EXPECT_TRUE(cr->is_contiguous());
}

TEST(ViewTests, ContiguousNonContiguousIsContiguous)
{
    // 非 contiguous → 结果 is_contiguous，shape 相同
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto tr = r->transpose(0, 1); // shape {4,3}
    ASSERT_TRUE(tr.has_value());

    auto cr = tr->contiguous();
    ASSERT_TRUE(cr.has_value());

    EXPECT_TRUE(cr->is_contiguous());
    EXPECT_EQ(cr->shape(), (Shape{4, 3}));
}

TEST(ViewTests, ContiguousDataOrder)
{
    // 验证 transpose + contiguous 后数据按行优先正确排列
    // 原始 2×3 矩阵：[[0,1,2],[3,4,5]]
    auto r = Tensor::empty({2, 3}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto* ptr = static_cast<float*>(r->data_ptr());
    for (int i = 0; i < 6; ++i)
        ptr[i] = static_cast<float>(i);

    // transpose(0,1) → shape {3,2}，strides {1,3}
    auto tr = r->transpose(0, 1);
    ASSERT_TRUE(tr.has_value());

    // contiguous → shape {3,2}，strides {2,1}
    auto cr = tr->contiguous();
    ASSERT_TRUE(cr.has_value());

    EXPECT_TRUE(cr->is_contiguous());
    EXPECT_EQ(cr->shape(), (Shape{3, 2}));

    // 期望：原矩阵转置后按行优先 = [[0,3],[1,4],[2,5]] = {0,3,1,4,2,5}
    const auto* cptr = static_cast<const float*>(cr->data_ptr());
    EXPECT_FLOAT_EQ(cptr[0], 0.f);
    EXPECT_FLOAT_EQ(cptr[1], 3.f);
    EXPECT_FLOAT_EQ(cptr[2], 1.f);
    EXPECT_FLOAT_EQ(cptr[3], 4.f);
    EXPECT_FLOAT_EQ(cptr[4], 2.f);
    EXPECT_FLOAT_EQ(cptr[5], 5.f);
}

// ═══════════════════════════════════════════════════════════════
// permute
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, PermuteShapeAndStrides)
{
    // {2,3,4} permute({2,0,1}) → {4,2,3}
    auto r = Tensor::empty({2, 3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());
    // 原始 strides：{12, 4, 1}

    auto pr = r->permute({2, 0, 1});
    ASSERT_TRUE(pr.has_value());

    EXPECT_EQ(pr->shape(), (Shape{4, 2, 3}));
    EXPECT_EQ(pr->strides(), (Strides{1, 12, 4}));
    EXPECT_EQ(pr->storage().get(), r->storage().get()); // 零拷贝
    EXPECT_FALSE(pr->is_contiguous());
}

TEST(ViewTests, PermuteInvalidRepeated)
{
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    // 重复维度
    auto pr = r->permute({0, 0});
    EXPECT_FALSE(pr.has_value());
}

TEST(ViewTests, PermuteInvalidOutOfRange)
{
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    // 越界
    auto pr = r->permute({0, 2});
    EXPECT_FALSE(pr.has_value());
}

// ═══════════════════════════════════════════════════════════════
// transpose
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, TransposeBasic)
{
    // transpose(0,1) 等价于 permute({1,0})
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto tr = r->transpose(0, 1);
    ASSERT_TRUE(tr.has_value());

    auto pr = r->permute({1, 0});
    ASSERT_TRUE(pr.has_value());

    EXPECT_EQ(tr->shape(), pr->shape());
    EXPECT_EQ(tr->strides(), pr->strides());
    EXPECT_EQ(tr->storage().get(), r->storage().get());
}

TEST(ViewTests, TransposeNegativeIndex)
{
    // transpose(-1, -2) 等价于 transpose(1, 0)
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto tr1 = r->transpose(-1, -2);
    auto tr2 = r->transpose(1, 0);
    ASSERT_TRUE(tr1.has_value());
    ASSERT_TRUE(tr2.has_value());

    EXPECT_EQ(tr1->shape(), tr2->shape());
    EXPECT_EQ(tr1->strides(), tr2->strides());
}

TEST(ViewTests, TransposeOutOfRangeError)
{
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    EXPECT_FALSE(r->transpose(0, 5).has_value());
    EXPECT_FALSE(r->transpose(-3, 0).has_value());
}

// ═══════════════════════════════════════════════════════════════
// squeeze
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, SqueezeRemovesDimOne)
{
    // {1,3,1}.squeeze(0) → {3,1}
    auto r = Tensor::empty({1, 3, 1}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto s0 = r->squeeze(0);
    ASSERT_TRUE(s0.has_value());
    EXPECT_EQ(s0->shape(), (Shape{3, 1}));
    EXPECT_EQ(s0->storage().get(), r->storage().get());

    // {3,1}.squeeze(1) → {3}（原 dim 2，现 dim 1）
    auto s1 = s0->squeeze(1);
    ASSERT_TRUE(s1.has_value());
    EXPECT_EQ(s1->shape(), (Shape{3}));
}

TEST(ViewTests, SqueezeNonOneDimNoOp)
{
    // size != 1 的维度：返回浅拷贝，shape 不变，storage 共享
    auto r = Tensor::empty({3, 4}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto sr = r->squeeze(0);
    ASSERT_TRUE(sr.has_value());
    EXPECT_EQ(sr->shape(), (Shape{3, 4})); // 未变
    EXPECT_EQ(sr->storage().get(), r->storage().get());
}

TEST(ViewTests, SqueezeNegativeIndex)
{
    // squeeze(-1) 等价于 squeeze(ndim-1)
    auto r = Tensor::empty({1, 3, 1}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto sr = r->squeeze(-1);
    ASSERT_TRUE(sr.has_value());
    EXPECT_EQ(sr->shape(), (Shape{1, 3}));
}

// ═══════════════════════════════════════════════════════════════
// unsqueeze
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, UnsqueezeAtFront)
{
    // {3}.unsqueeze(0) → {1,3}
    auto r = Tensor::empty({3}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto ur = r->unsqueeze(0);
    ASSERT_TRUE(ur.has_value());
    EXPECT_EQ(ur->shape(), (Shape{1, 3}));
    EXPECT_EQ(ur->storage().get(), r->storage().get());
}

TEST(ViewTests, UnsqueezeAtEnd)
{
    // {3}.unsqueeze(-1) → {3,1}
    auto r = Tensor::empty({3}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto ur = r->unsqueeze(-1);
    ASSERT_TRUE(ur.has_value());
    EXPECT_EQ(ur->shape(), (Shape{3, 1}));
}

TEST(ViewTests, UnsqueezeAtNdim)
{
    // {3}.unsqueeze(1) (== ndim) → {3,1}
    auto r = Tensor::empty({3}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto ur = r->unsqueeze(1);
    ASSERT_TRUE(ur.has_value());
    EXPECT_EQ(ur->shape(), (Shape{3, 1}));
}

TEST(ViewTests, UnsqueezeOutOfRangeError)
{
    auto r = Tensor::empty({3}, DType::F32);
    ASSERT_TRUE(r.has_value());

    // {3}: ndim=1，有效范围 [-2, 1]；2 越界
    EXPECT_FALSE(r->unsqueeze(2).has_value());
    EXPECT_FALSE(r->unsqueeze(-3).has_value());
}

// ═══════════════════════════════════════════════════════════════
// slice
// ═══════════════════════════════════════════════════════════════

TEST(ViewTests, SliceBasic)
{
    // {10}.slice(0, 2, 8) → shape {6}, stride {1}, offset 2
    auto r = Tensor::empty({10}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto sr = r->slice(0, 2, 8);
    ASSERT_TRUE(sr.has_value());

    EXPECT_EQ(sr->shape(), (Shape{6}));
    EXPECT_EQ(sr->strides(), (Strides{1}));
    EXPECT_EQ(sr->storage_offset(), 2);

    // data_ptr 偏移应为 2 * sizeof(float)
    const auto diff =
        static_cast<const uint8_t*>(sr->data_ptr()) -
        static_cast<const uint8_t*>(r->data_ptr());
    EXPECT_EQ(diff, static_cast<ptrdiff_t>(2 * sizeof(float)));
}

TEST(ViewTests, SliceWithStep)
{
    // {10}.slice(0, 0, 10, 2) → shape {5}, stride {2}
    auto r = Tensor::empty({10}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto sr = r->slice(0, 0, 10, 2);
    ASSERT_TRUE(sr.has_value());

    EXPECT_EQ(sr->shape(), (Shape{5}));
    EXPECT_EQ(sr->strides(), (Strides{2}));
    EXPECT_EQ(sr->storage().get(), r->storage().get()); // 零拷贝
}

TEST(ViewTests, SliceOutOfBoundsError)
{
    auto r = Tensor::empty({10}, DType::F32);
    ASSERT_TRUE(r.has_value());

    // end > dim_size
    EXPECT_FALSE(r->slice(0, 0, 11).has_value());
    // start > end
    EXPECT_FALSE(r->slice(0, 5, 3).has_value());
}

TEST(ViewTests, SliceNegativeStepError)
{
    auto r = Tensor::empty({10}, DType::F32);
    ASSERT_TRUE(r.has_value());

    EXPECT_FALSE(r->slice(0, 0, 5, 0).has_value());
    EXPECT_FALSE(r->slice(0, 0, 5, -1).has_value());
}

TEST(ViewTests, SliceNegativeDimIndex)
{
    // dim=-1 等价于 dim=0（1D 张量）
    auto r = Tensor::empty({10}, DType::F32);
    ASSERT_TRUE(r.has_value());

    auto sr = r->slice(-1, 2, 8);
    ASSERT_TRUE(sr.has_value());
    EXPECT_EQ(sr->shape(), (Shape{6}));
}
