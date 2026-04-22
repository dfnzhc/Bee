#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cmath>
#include <cstdint>

using namespace bee;

// ── 测试辅助宏 ────────────────────────────────────────────────────────────────

#define ASSERT_OK(expr)  ASSERT_TRUE((expr).has_value())
#define ASSERT_ERR(expr) ASSERT_FALSE((expr).has_value())

// 从 0-rank 或 1-rank 张量读取第一个元素
template <typename T>
static auto scalar_val(const Tensor& t) -> T
{
    return *static_cast<const T*>(t.data_ptr());
}

// ─────────────────────────────────────────────────────────────────────────────
// 全局 reduce 测试
// ─────────────────────────────────────────────────────────────────────────────

TEST(ReduceTests, GlobalSumF32)
{
    // 2×3 全 2.0f，sum == 12.0f
    auto a = Tensor::full({2, 3}, DType::F32, 2.0);
    ASSERT_OK(a);

    auto r = sum(*a);
    ASSERT_OK(r);
    EXPECT_EQ(r->ndim(), 0);
    EXPECT_EQ(r->numel(), 1);
    EXPECT_FLOAT_EQ(scalar_val<float>(*r), 12.0f);
}

TEST(ReduceTests, GlobalSumF64)
{
    auto a = Tensor::full({5}, DType::F64, 3.0);
    ASSERT_OK(a);

    auto r = sum(*a);
    ASSERT_OK(r);
    EXPECT_DOUBLE_EQ(scalar_val<double>(*r), 15.0);
}

TEST(ReduceTests, GlobalSumI32)
{
    auto a = Tensor::full({4}, DType::I32, 5.0);
    ASSERT_OK(a);

    auto r = sum(*a);
    ASSERT_OK(r);
    EXPECT_EQ(scalar_val<int32_t>(*r), 20);
}

TEST(ReduceTests, GlobalSumI64)
{
    auto a = Tensor::full({3}, DType::I64, 7.0);
    ASSERT_OK(a);

    auto r = sum(*a);
    ASSERT_OK(r);
    EXPECT_EQ(scalar_val<int64_t>(*r), 21LL);
}

TEST(ReduceTests, GlobalSumBoolErr)
{
    // Bool 不支持 sum
    auto a = Tensor::full({4}, DType::Bool, 1.0);
    ASSERT_OK(a);
    ASSERT_ERR(sum(*a));
}

TEST(ReduceTests, GlobalSumU8Err)
{
    // U8 不支持 sum
    auto a = Tensor::full({4}, DType::U8, 1.0);
    ASSERT_OK(a);
    ASSERT_ERR(sum(*a));
}

TEST(ReduceTests, GlobalMeanApprox)
{
    // 1000 个 1.0f，mean ≈ 1.0f
    auto a = Tensor::full({1000}, DType::F32, 1.0);
    ASSERT_OK(a);

    auto r = mean(*a);
    ASSERT_OK(r);
    EXPECT_EQ(r->dtype(), DType::F32);
    EXPECT_NEAR(scalar_val<float>(*r), 1.0f, 1e-5f);
}

TEST(ReduceTests, GlobalMeanF64)
{
    auto a = Tensor::full({4}, DType::F64, 3.0);
    ASSERT_OK(a);

    auto r = mean(*a);
    ASSERT_OK(r);
    EXPECT_EQ(r->dtype(), DType::F64);
    EXPECT_DOUBLE_EQ(scalar_val<double>(*r), 3.0);
}

TEST(ReduceTests, GlobalMeanI32PromotesToF64)
{
    // I32 输入 → F64 输出
    auto a = Tensor::full({4}, DType::I32, 8.0);
    ASSERT_OK(a);

    auto r = mean(*a);
    ASSERT_OK(r);
    EXPECT_EQ(r->dtype(), DType::F64);
    EXPECT_DOUBLE_EQ(scalar_val<double>(*r), 8.0);
}

TEST(ReduceTests, GlobalMeanU8Err)
{
    auto a = Tensor::full({4}, DType::U8, 1.0);
    ASSERT_OK(a);
    ASSERT_ERR(mean(*a));
}

TEST(ReduceTests, GlobalMinI32)
{
    // {1, 2, 3, 4} → min = 1
    auto a = Tensor::empty({4}, DType::I32);
    ASSERT_OK(a);
    auto* p = static_cast<int32_t*>(a->data_ptr());
    p[0]    = 1;
    p[1]    = 2;
    p[2]    = 3;
    p[3]    = 4;

    auto r = min(*a);
    ASSERT_OK(r);
    EXPECT_EQ(scalar_val<int32_t>(*r), 1);
}

TEST(ReduceTests, GlobalMaxI32)
{
    // {1, 2, 3, 4} → max = 4
    auto a = Tensor::empty({4}, DType::I32);
    ASSERT_OK(a);
    auto* p = static_cast<int32_t*>(a->data_ptr());
    p[0]    = 1;
    p[1]    = 2;
    p[2]    = 3;
    p[3]    = 4;

    auto r = max(*a);
    ASSERT_OK(r);
    EXPECT_EQ(scalar_val<int32_t>(*r), 4);
}

TEST(ReduceTests, GlobalMinF32)
{
    auto a = Tensor::full({8}, DType::F32, -3.0);
    ASSERT_OK(a);
    auto* p = static_cast<float*>(a->data_ptr());
    p[4]    = -10.0f; // 插入最小值

    auto r = min(*a);
    ASSERT_OK(r);
    EXPECT_FLOAT_EQ(scalar_val<float>(*r), -10.0f);
}

TEST(ReduceTests, GlobalMaxF32)
{
    auto a = Tensor::full({8}, DType::F32, 1.0);
    ASSERT_OK(a);
    auto* p = static_cast<float*>(a->data_ptr());
    p[7]    = 99.0f;

    auto r = max(*a);
    ASSERT_OK(r);
    EXPECT_FLOAT_EQ(scalar_val<float>(*r), 99.0f);
}

TEST(ReduceTests, GlobalMinU8)
{
    // U8 min/max 支持
    auto a = Tensor::empty({4}, DType::U8);
    ASSERT_OK(a);
    auto* p = static_cast<uint8_t*>(a->data_ptr());
    p[0]    = 10;
    p[1]    = 3;
    p[2]    = 7;
    p[3]    = 200;

    auto rmin = min(*a);
    ASSERT_OK(rmin);
    EXPECT_EQ(scalar_val<uint8_t>(*rmin), 3u);

    auto rmax = max(*a);
    ASSERT_OK(rmax);
    EXPECT_EQ(scalar_val<uint8_t>(*rmax), 200u);
}

TEST(ReduceTests, GlobalMinBoolErr)
{
    auto a = Tensor::full({4}, DType::Bool, 1.0);
    ASSERT_OK(a);
    ASSERT_ERR(min(*a));
    ASSERT_ERR(max(*a));
}

TEST(ReduceTests, GlobalProdI32)
{
    // 2×3 全 2，prod == 2^6 == 64
    auto a = Tensor::full({2, 3}, DType::I32, 2.0);
    ASSERT_OK(a);

    auto r = prod(*a);
    ASSERT_OK(r);
    EXPECT_EQ(scalar_val<int32_t>(*r), 64);
}

TEST(ReduceTests, GlobalProdF32)
{
    auto a = Tensor::full({3}, DType::F32, 2.0);
    ASSERT_OK(a);

    auto r = prod(*a);
    ASSERT_OK(r);
    EXPECT_FLOAT_EQ(scalar_val<float>(*r), 8.0f);
}

TEST(ReduceTests, GlobalProdBoolErr)
{
    auto a = Tensor::full({4}, DType::Bool, 1.0);
    ASSERT_OK(a);
    ASSERT_ERR(prod(*a));
}

TEST(ReduceTests, GlobalSumNonContiguous)
{
    // 构造 2×3 张量，转置后 sum 与直接 sum 相同（同一组元素）
    auto a = Tensor::full({2, 3}, DType::F32, 1.0);
    ASSERT_OK(a);
    // 手动设置已知数据
    auto* p = static_cast<float*>(a->data_ptr());
    for (int i = 0; i < 6; ++i)
        p[i] = static_cast<float>(i + 1); // {1,2,3,4,5,6}

    auto r_direct = sum(*a);
    ASSERT_OK(r_direct);

    auto t = a->transpose(0, 1);
    ASSERT_OK(t);
    EXPECT_FALSE(t->is_contiguous()); // 转置后非连续

    auto r_transposed = sum(*t);
    ASSERT_OK(r_transposed);
    // 元素集合相同，sum 应相等
    EXPECT_FLOAT_EQ(scalar_val<float>(*r_direct), scalar_val<float>(*r_transposed));
}

TEST(ReduceTests, GlobalSum0Rank)
{
    // 0-rank 标量张量：sum 应返回同值
    auto a = Tensor::full({}, DType::F32, 7.0);
    ASSERT_OK(a);
    EXPECT_EQ(a->ndim(), 0);

    auto r = sum(*a);
    ASSERT_OK(r);
    EXPECT_FLOAT_EQ(scalar_val<float>(*r), 7.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// 按轴 reduce 测试
// ─────────────────────────────────────────────────────────────────────────────

TEST(ReduceTests, AxisSumDim1_3x4)
{
    // arange(0,12).reshape({3,4})
    //   [[0,1,2,3],[4,5,6,7],[8,9,10,11]]
    // sum(dim=1) → {6, 22, 38}，shape {3}
    auto a = Tensor::arange(0, 12, 1, DType::I64);
    ASSERT_OK(a);
    auto b = a->reshape({3, 4});
    ASSERT_OK(b);

    auto r = sum(*b, 1);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{3}));
    const auto* p = static_cast<const int64_t*>(r->data_ptr());
    EXPECT_EQ(p[0], 6LL);
    EXPECT_EQ(p[1], 22LL);
    EXPECT_EQ(p[2], 38LL);
}

TEST(ReduceTests, AxisSumDim1_Keepdim)
{
    // sum(dim=1, keepdim=true) → shape {3,1}
    auto a = Tensor::arange(0, 12, 1, DType::I64);
    ASSERT_OK(a);
    auto b = a->reshape({3, 4});
    ASSERT_OK(b);

    auto r = sum(*b, 1, true);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{3, 1}));
    const auto* p = static_cast<const int64_t*>(r->data_ptr());
    EXPECT_EQ(p[0], 6LL);
    EXPECT_EQ(p[1], 22LL);
    EXPECT_EQ(p[2], 38LL);
}

TEST(ReduceTests, AxisSumNegDim)
{
    // dim=-1 与 dim=1 等价（对 3×4 张量）
    auto a = Tensor::arange(0, 12, 1, DType::I64);
    ASSERT_OK(a);
    auto b = a->reshape({3, 4});
    ASSERT_OK(b);

    auto r1 = sum(*b, 1);
    ASSERT_OK(r1);
    auto r2 = sum(*b, -1);
    ASSERT_OK(r2);

    const auto* p1 = static_cast<const int64_t*>(r1->data_ptr());
    const auto* p2 = static_cast<const int64_t*>(r2->data_ptr());
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ(p1[i], p2[i]);
}

TEST(ReduceTests, AxisSumDim0_3x4)
{
    // sum(dim=0) → {12, 15, 18, 21}，shape {4}
    auto a = Tensor::arange(0, 12, 1, DType::I64);
    ASSERT_OK(a);
    auto b = a->reshape({3, 4});
    ASSERT_OK(b);

    auto r = sum(*b, 0);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{4}));
    const auto* p = static_cast<const int64_t*>(r->data_ptr());
    EXPECT_EQ(p[0], 12LL);
    EXPECT_EQ(p[1], 15LL);
    EXPECT_EQ(p[2], 18LL);
    EXPECT_EQ(p[3], 21LL);
}

TEST(ReduceTests, AxisMinDim0_3x4)
{
    // min(dim=0) 对 3×4 矩阵，结果应为每列最小值
    auto a = Tensor::arange(0, 12, 1, DType::I64);
    ASSERT_OK(a);
    auto b = a->reshape({3, 4});
    ASSERT_OK(b);

    auto r = min(*b, 0);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{4}));
    const auto* p = static_cast<const int64_t*>(r->data_ptr());
    // 每列第一行（行0）即最小：0,1,2,3
    EXPECT_EQ(p[0], 0LL);
    EXPECT_EQ(p[1], 1LL);
    EXPECT_EQ(p[2], 2LL);
    EXPECT_EQ(p[3], 3LL);
}

TEST(ReduceTests, AxisMaxDim1_3x4)
{
    auto a = Tensor::arange(0, 12, 1, DType::I64);
    ASSERT_OK(a);
    auto b = a->reshape({3, 4});
    ASSERT_OK(b);

    auto r = max(*b, 1);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{3}));
    const auto* p = static_cast<const int64_t*>(r->data_ptr());
    // 每行最大值：3, 7, 11
    EXPECT_EQ(p[0], 3LL);
    EXPECT_EQ(p[1], 7LL);
    EXPECT_EQ(p[2], 11LL);
}

TEST(ReduceTests, AxisMeanI32ProducesF64)
{
    // I32 输入 → F64 输出
    auto a = Tensor::full({3, 4}, DType::I32, 6.0);
    ASSERT_OK(a);

    auto r = mean(*a, 1);
    ASSERT_OK(r);
    EXPECT_EQ(r->dtype(), DType::F64);
    EXPECT_EQ(r->shape(), (Shape{3}));
    const auto* p = static_cast<const double*>(r->data_ptr());
    for (int i = 0; i < 3; ++i)
        EXPECT_DOUBLE_EQ(p[i], 6.0);
}

TEST(ReduceTests, AxisProdDim1)
{
    // 2×3 全 2 的 I32，prod(dim=1) → {8, 8}
    auto a = Tensor::full({2, 3}, DType::I32, 2.0);
    ASSERT_OK(a);

    auto r = prod(*a, 1);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{2}));
    const auto* p = static_cast<const int32_t*>(r->data_ptr());
    EXPECT_EQ(p[0], 8);
    EXPECT_EQ(p[1], 8);
}

TEST(ReduceTests, AxisDimOutOfBoundsErr)
{
    // dim=2 在 shape {3,4} 上越界 → Err
    auto a = Tensor::full({3, 4}, DType::F32, 1.0);
    ASSERT_OK(a);
    ASSERT_ERR(sum(*a, 2));
}

TEST(ReduceTests, AxisDimOnZeroRankErr)
{
    // 0-rank 张量按轴 reduce → Err
    auto a = Tensor::full({}, DType::F32, 3.0);
    ASSERT_OK(a);
    ASSERT_ERR(sum(*a, 0));
}

TEST(ReduceTests, AxisSumF32Large)
{
    // 较大张量，验证 SIMD tail 处理正确
    // shape {3, 17}，全 1.0f，sum(dim=1) 每行 == 17.0f
    auto a = Tensor::full({3, 17}, DType::F32, 1.0);
    ASSERT_OK(a);

    auto r = sum(*a, 1);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{3}));
    const auto* p = static_cast<const float*>(r->data_ptr());
    for (int i = 0; i < 3; ++i)
        EXPECT_FLOAT_EQ(p[i], 17.0f);
}

TEST(ReduceTests, AxisMeanF32KeepdimShape)
{
    // mean(dim=0, keepdim=true) 在 {4,5} 上 → shape {1,5}
    auto a = Tensor::full({4, 5}, DType::F32, 2.0);
    ASSERT_OK(a);

    auto r = mean(*a, 0, true);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{1, 5}));
    const auto* p = static_cast<const float*>(r->data_ptr());
    for (int i = 0; i < 5; ++i)
        EXPECT_FLOAT_EQ(p[i], 2.0f);
}

TEST(ReduceTests, AxisSumNonContiguous)
{
    // 非连续张量（transpose）按轴 reduce 正确性
    // 构造 2×3 已知数据，转置为 3×2，再 sum(dim=1)
    auto a = Tensor::empty({2, 3}, DType::F32);
    ASSERT_OK(a);
    auto* p = static_cast<float*>(a->data_ptr());
    // 行0: [1,2,3], 行1: [4,5,6]
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;
    p[3] = 4;
    p[4] = 5;
    p[5] = 6;

    // 转置后 shape {3,2}，非连续
    auto t = a->transpose(0, 1);
    ASSERT_OK(t);
    EXPECT_FALSE(t->is_contiguous());

    // sum(dim=1) 对 3×2 矩阵：[[1,4],[2,5],[3,6]] → {5,7,9}
    auto r = sum(*t, 1);
    ASSERT_OK(r);
    EXPECT_EQ(r->shape(), (Shape{3}));
    const auto* rp = static_cast<const float*>(r->data_ptr());
    EXPECT_FLOAT_EQ(rp[0], 5.0f);
    EXPECT_FLOAT_EQ(rp[1], 7.0f);
    EXPECT_FLOAT_EQ(rp[2], 9.0f);
}

TEST(ReduceTests, GlobalMinEmptyErr)
{
    auto a = Tensor::empty({0}, DType::F32);
    ASSERT_OK(a);
    ASSERT_ERR(min(*a));
    ASSERT_ERR(max(*a));
}

TEST(ReduceTests, AxisMinMaxEmptyDimErr)
{
    auto a = Tensor::empty({2, 0, 3}, DType::F32);
    ASSERT_OK(a);
    ASSERT_ERR(min(*a, 1));
    ASSERT_ERR(max(*a, 1));
}
