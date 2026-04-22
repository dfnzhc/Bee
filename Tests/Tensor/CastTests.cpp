#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

using namespace bee;

// ── F32 → I32 基本转换 ──────────────────────────────────────────────────────

TEST(CastTests, F32ToI32_Arange)
{
    // arange(0,5,1,F32) → cast I32 → 值 {0,1,2,3,4}
    auto src_r = Tensor::arange(0, 5, 1, DType::F32);
    ASSERT_TRUE(src_r.has_value());

    auto out_r = cast(*src_r, DType::I32);
    ASSERT_TRUE(out_r.has_value());

    const Tensor& out = *out_r;
    EXPECT_EQ(out.dtype(), DType::I32);
    EXPECT_EQ(out.numel(), 5);

    const auto* ptr = static_cast<const int32_t*>(out.data_ptr());
    for (int32_t i = 0; i < 5; ++i)
        EXPECT_EQ(ptr[i], i);
}

// ── 浮点截断：3.7f → I32 = 3 ────────────────────────────────────────────────

TEST(CastTests, F32ToI32_Truncation)
{
    auto src_r = Tensor::full({1}, DType::F32, 3.7);
    ASSERT_TRUE(src_r.has_value());

    auto out_r = cast(*src_r, DType::I32);
    ASSERT_TRUE(out_r.has_value());

    EXPECT_EQ(static_cast<const int32_t*>(out_r->data_ptr())[0], 3);
}

// ── I32 → F64 精度保留 ──────────────────────────────────────────────────────

TEST(CastTests, I32ToF64_Preserve)
{
    auto src_r = Tensor::arange(0, 6, 1, DType::I32);
    ASSERT_TRUE(src_r.has_value());

    auto out_r = cast(*src_r, DType::F64);
    ASSERT_TRUE(out_r.has_value());

    EXPECT_EQ(out_r->dtype(), DType::F64);
    const auto* ptr = static_cast<const double*>(out_r->data_ptr());
    for (int i = 0; i < 6; ++i)
        EXPECT_DOUBLE_EQ(ptr[i], static_cast<double>(i));
}

// ── I32 非零 → Bool = true；0 → Bool = false ────────────────────────────────

TEST(CastTests, I32ToBool)
{
    // 构造 {0, 1, -3, 42}
    auto src_r = Tensor::zeros({4}, DType::I32);
    ASSERT_TRUE(src_r.has_value());
    auto* p = static_cast<int32_t*>(src_r->data_ptr());
    p[0]    = 0;
    p[1]    = 1;
    p[2]    = -3;
    p[3]    = 42;

    auto out_r = cast(*src_r, DType::Bool);
    ASSERT_TRUE(out_r.has_value());

    const auto* bp = static_cast<const bool*>(out_r->data_ptr());
    EXPECT_EQ(bp[0], false);
    EXPECT_EQ(bp[1], true);
    EXPECT_EQ(bp[2], true);
    EXPECT_EQ(bp[3], true);
}

// ── Bool → I32：true=1，false=0 ─────────────────────────────────────────────

TEST(CastTests, BoolToI32)
{
    auto src_r = Tensor::zeros({2}, DType::Bool);
    ASSERT_TRUE(src_r.has_value());
    auto* bp = static_cast<bool*>(src_r->data_ptr());
    bp[0]    = true;
    bp[1]    = false;

    auto out_r = cast(*src_r, DType::I32);
    ASSERT_TRUE(out_r.has_value());

    const auto* ip = static_cast<const int32_t*>(out_r->data_ptr());
    EXPECT_EQ(ip[0], 1);
    EXPECT_EQ(ip[1], 0);
}

// ── 相同 dtype cast → 新 storage，数据相同 ──────────────────────────────────

TEST(CastTests, SameDtype_NewStorage)
{
    auto src_r = Tensor::arange(0, 4, 1, DType::F32);
    ASSERT_TRUE(src_r.has_value());

    auto out_r = cast(*src_r, DType::F32);
    ASSERT_TRUE(out_r.has_value());

    // storage 指针不同（独立副本）
    EXPECT_NE(src_r->data_ptr(), out_r->data_ptr());

    // 数据相同
    const auto* sp = static_cast<const float*>(src_r->data_ptr());
    const auto* dp = static_cast<const float*>(out_r->data_ptr());
    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ(dp[i], sp[i]);
}

// ── 非 contiguous 输入：transpose 后 cast，输出 contiguous 且数据正确 ─────────

TEST(CastTests, NonContiguous_TransposeThenCast)
{
    // arange(0,12) → reshape {3,4} → transpose(0,1) → shape {4,3}，非连续
    auto base_r = Tensor::arange(0, 12, 1, DType::I32);
    ASSERT_TRUE(base_r.has_value());

    auto rs_r = base_r->reshape({3, 4});
    ASSERT_TRUE(rs_r.has_value());

    auto tr_r = rs_r->transpose(0, 1);
    ASSERT_TRUE(tr_r.has_value());
    EXPECT_FALSE(tr_r->is_contiguous());
    EXPECT_EQ(tr_r->shape()[0], 4);
    EXPECT_EQ(tr_r->shape()[1], 3);

    // cast 到 F32
    auto out_r = cast(*tr_r, DType::F32);
    ASSERT_TRUE(out_r.has_value());

    EXPECT_TRUE(out_r->is_contiguous());
    EXPECT_EQ(out_r->dtype(), DType::F32);
    EXPECT_EQ(out_r->shape()[0], 4);
    EXPECT_EQ(out_r->shape()[1], 3);

    // 验证数据：transpose 后 (i,j) 对应原始 (j,i)，即 j*4+i
    const auto* fp  = static_cast<const float*>(out_r->data_ptr());
    int         idx = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            float expected = static_cast<float>(j * 4 + i);
            EXPECT_FLOAT_EQ(fp[idx++], expected) << "i=" << i << " j=" << j;
        }
    }
}

// ── shape 与 device 保持不变 ─────────────────────────────────────────────────

TEST(CastTests, ShapePreserved)
{
    auto src_r = Tensor::zeros({2, 3, 4}, DType::F32);
    ASSERT_TRUE(src_r.has_value());

    auto out_r = cast(*src_r, DType::I64);
    ASSERT_TRUE(out_r.has_value());

    EXPECT_EQ(out_r->shape()[0], 2);
    EXPECT_EQ(out_r->shape()[1], 3);
    EXPECT_EQ(out_r->shape()[2], 4);
    EXPECT_EQ(out_r->device(), Device::CPU);
}
