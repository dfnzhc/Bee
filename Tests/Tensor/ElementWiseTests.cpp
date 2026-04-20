#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cmath>

using namespace bee;

#define ASSERT_OK(expr) ASSERT_TRUE((expr).has_value())
#define ASSERT_ERR(expr) ASSERT_FALSE((expr).has_value())

TEST(ElementWiseTests, AddF32Contiguous)
{
    auto a = Tensor::full({1000}, DType::F32, 1.5);
    auto b = Tensor::full({1000}, DType::F32, 2.5);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = add(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{1000}));

    const auto* ptr = static_cast<const float*>(c->data_ptr());
    for (int64_t i = 0; i < 1000; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 4.0f);
}

TEST(ElementWiseTests, AddF64WithSimdTail)
{
    auto a = Tensor::full({17}, DType::F64, 1.0);
    auto b = Tensor::full({17}, DType::F64, 2.0);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = add(*a, *b);
    ASSERT_OK(c);

    const auto* ptr = static_cast<const double*>(c->data_ptr());
    for (int64_t i = 0; i < 17; ++i)
        EXPECT_DOUBLE_EQ(ptr[i], 3.0);
}

TEST(ElementWiseTests, SubI32Contiguous)
{
    auto a = Tensor::full({8}, DType::I32, 10.0);
    auto b = Tensor::full({8}, DType::I32, 3.0);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = sub(*a, *b);
    ASSERT_OK(c);

    const auto* ptr = static_cast<const int32_t*>(c->data_ptr());
    for (int64_t i = 0; i < 8; ++i)
        EXPECT_EQ(ptr[i], 7);
}

TEST(ElementWiseTests, MulF32Contiguous)
{
    auto a = Tensor::full({16}, DType::F32, 3.0);
    auto b = Tensor::full({16}, DType::F32, 4.0);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = mul(*a, *b);
    ASSERT_OK(c);

    const auto* ptr = static_cast<const float*>(c->data_ptr());
    for (int64_t i = 0; i < 16; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 12.0f);
}

TEST(ElementWiseTests, DivF32Contiguous)
{
    auto a = Tensor::full({16}, DType::F32, 9.0);
    auto b = Tensor::full({16}, DType::F32, 3.0);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = div(*a, *b);
    ASSERT_OK(c);

    const auto* ptr = static_cast<const float*>(c->data_ptr());
    for (int64_t i = 0; i < 16; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 3.0f);
}

TEST(ElementWiseTests, BroadcastAdd2x3Plus3)
{
    auto a = Tensor::empty({2, 3}, DType::I32);
    ASSERT_OK(a);
    {
        auto* p = static_cast<int32_t*>(a->data_ptr());
        p[0] = 1; p[1] = 2; p[2] = 3;
        p[3] = 4; p[4] = 5; p[5] = 6;
    }

    auto b = Tensor::empty({3}, DType::I32);
    ASSERT_OK(b);
    {
        auto* p = static_cast<int32_t*>(b->data_ptr());
        p[0] = 10; p[1] = 20; p[2] = 30;
    }

    auto c = add(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{2, 3}));

    const auto* p = static_cast<const int32_t*>(c->data_ptr());
    EXPECT_EQ(p[0], 11); EXPECT_EQ(p[1], 22); EXPECT_EQ(p[2], 33);
    EXPECT_EQ(p[3], 14); EXPECT_EQ(p[4], 25); EXPECT_EQ(p[5], 36);
}

TEST(ElementWiseTests, BroadcastMul2x1Times1x3)
{
    auto a = Tensor::empty({2, 1}, DType::I32);
    ASSERT_OK(a);
    {
        auto* p = static_cast<int32_t*>(a->data_ptr());
        p[0] = 2; p[1] = 3;
    }

    auto b = Tensor::empty({1, 3}, DType::I32);
    ASSERT_OK(b);
    {
        auto* p = static_cast<int32_t*>(b->data_ptr());
        p[0] = 10; p[1] = 20; p[2] = 30;
    }

    auto c = mul(*a, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{2, 3}));

    const auto* p = static_cast<const int32_t*>(c->data_ptr());
    EXPECT_EQ(p[0], 20); EXPECT_EQ(p[1], 40); EXPECT_EQ(p[2], 60);
    EXPECT_EQ(p[3], 30); EXPECT_EQ(p[4], 60); EXPECT_EQ(p[5], 90);
}

TEST(ElementWiseTests, BroadcastIncompatible)
{
    auto a = Tensor::zeros({3, 4}, DType::F32);
    auto b = Tensor::zeros({3}, DType::F32);
    ASSERT_OK(a);
    ASSERT_OK(b);

    auto c = add(*a, *b);
    ASSERT_ERR(c);
}

TEST(ElementWiseTests, NonContiguousTransposeAdd)
{
    auto base = Tensor::arange(0, 12, 1, DType::I32);
    ASSERT_OK(base);
    auto a_3x4 = base->reshape({3, 4});
    ASSERT_OK(a_3x4);
    auto a_t = a_3x4->transpose(0, 1);
    ASSERT_OK(a_t);
    EXPECT_FALSE(a_t->is_contiguous());

    auto b = Tensor::zeros({4, 3}, DType::I32);
    ASSERT_OK(b);

    auto c = add(*a_t, *b);
    ASSERT_OK(c);

    auto a_cont = a_t->contiguous();
    ASSERT_OK(a_cont);
    auto c_ref = add(*a_cont, *b);
    ASSERT_OK(c_ref);

    EXPECT_EQ(c->shape(), c_ref->shape());
    const auto* p1 = static_cast<const int32_t*>(c->data_ptr());
    const auto* p2 = static_cast<const int32_t*>(c_ref->data_ptr());
    for (int64_t i = 0; i < c->numel(); ++i)
        EXPECT_EQ(p1[i], p2[i]) << "index=" << i;
}

TEST(ElementWiseTests, NonContiguousTransposeAdd_Values)
{
    auto base = Tensor::arange(0, 12, 1, DType::I32);
    ASSERT_OK(base);
    auto a_3x4 = base->reshape({3, 4});
    ASSERT_OK(a_3x4);
    auto a_t = a_3x4->transpose(0, 1);
    ASSERT_OK(a_t);

    auto b = Tensor::zeros({4, 3}, DType::I32);
    ASSERT_OK(b);

    auto c = add(*a_t, *b);
    ASSERT_OK(c);
    EXPECT_EQ(c->shape(), (Shape{4, 3}));

    const auto* p = static_cast<const int32_t*>(c->data_ptr());
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            const int expected = j * 4 + i;
            EXPECT_EQ(p[i * 3 + j], expected) << "i=" << i << " j=" << j;
        }
    }
}

TEST(ElementWiseTests, DtypeMismatch)
{
    auto a = Tensor::zeros({4}, DType::F32);
    auto b = Tensor::zeros({4}, DType::F64);
    ASSERT_OK(a);
    ASSERT_OK(b);
    ASSERT_ERR(add(*a, *b));
}

TEST(ElementWiseTests, BoolAddError)
{
    auto a = Tensor::zeros({4}, DType::Bool);
    auto b = Tensor::zeros({4}, DType::Bool);
    ASSERT_OK(a);
    ASSERT_OK(b);
    ASSERT_ERR(add(*a, *b));
}

TEST(ElementWiseTests, U8MulError)
{
    auto a = Tensor::zeros({4}, DType::U8);
    auto b = Tensor::zeros({4}, DType::U8);
    ASSERT_OK(a);
    ASSERT_OK(b);
    ASSERT_ERR(mul(*a, *b));
}

TEST(ElementWiseTests, I32SqrtError)
{
    auto a = Tensor::zeros({4}, DType::I32);
    ASSERT_OK(a);
    ASSERT_ERR(sqrt(*a));
}

TEST(ElementWiseTests, NegF32)
{
    auto a = Tensor::full({100}, DType::F32, 2.5);
    ASSERT_OK(a);

    auto b = neg(*a);
    ASSERT_OK(b);

    const auto* ptr = static_cast<const float*>(b->data_ptr());
    for (int64_t i = 0; i < 100; ++i)
        EXPECT_FLOAT_EQ(ptr[i], -2.5f);
}

TEST(ElementWiseTests, AbsF32)
{
    auto a = Tensor::full({16}, DType::F32, -3.0);
    ASSERT_OK(a);

    auto b = abs(*a);
    ASSERT_OK(b);

    const auto* ptr = static_cast<const float*>(b->data_ptr());
    for (int64_t i = 0; i < 16; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 3.0f);
}

TEST(ElementWiseTests, SqrtF32)
{
    auto a = Tensor::full({16}, DType::F32, 4.0);
    ASSERT_OK(a);

    auto b = sqrt(*a);
    ASSERT_OK(b);

    const auto* ptr = static_cast<const float*>(b->data_ptr());
    for (int64_t i = 0; i < 16; ++i)
        EXPECT_NEAR(ptr[i], 2.0f, 1e-5f);
}

TEST(ElementWiseTests, ExpF32)
{
    auto a = Tensor::full({100}, DType::F32, 1.0f);
    ASSERT_OK(a);

    auto b = exp(*a);
    ASSERT_OK(b);

    const auto* ptr = static_cast<const float*>(b->data_ptr());
    for (int64_t i = 0; i < 100; ++i)
        EXPECT_NEAR(ptr[i], std::exp(1.0f), 1e-4f);
}

TEST(ElementWiseTests, LogF32)
{
    const float e_val = static_cast<float>(std::exp(1.0));
    auto a = Tensor::full({100}, DType::F32, static_cast<double>(e_val));
    ASSERT_OK(a);

    auto b = log(*a);
    ASSERT_OK(b);

    const auto* ptr = static_cast<const float*>(b->data_ptr());
    for (int64_t i = 0; i < 100; ++i)
        EXPECT_NEAR(ptr[i], 1.0f, 1e-5f);
}

TEST(ElementWiseTests, NegI32)
{
    auto a = Tensor::full({8}, DType::I32, 7.0);
    ASSERT_OK(a);

    auto b = neg(*a);
    ASSERT_OK(b);

    const auto* ptr = static_cast<const int32_t*>(b->data_ptr());
    for (int64_t i = 0; i < 8; ++i)
        EXPECT_EQ(ptr[i], -7);
}

TEST(ElementWiseTests, AbsI32)
{
    auto a = Tensor::full({8}, DType::I32, -5.0);
    ASSERT_OK(a);

    auto b = abs(*a);
    ASSERT_OK(b);

    const auto* ptr = static_cast<const int32_t*>(b->data_ptr());
    for (int64_t i = 0; i < 8; ++i)
        EXPECT_EQ(ptr[i], 5);
}

TEST(ElementWiseTests, AddInplaceBasic)
{
    auto dst = Tensor::full({8}, DType::F32, 1.0);
    auto src = Tensor::full({8}, DType::F32, 2.0);
    ASSERT_OK(dst);
    ASSERT_OK(src);

    auto r = add_inplace(*dst, *src);
    ASSERT_OK(r);

    const auto* ptr = static_cast<const float*>(dst->data_ptr());
    for (int64_t i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ(ptr[i], 3.0f);
}

TEST(ElementWiseTests, AddInplaceBroadcast)
{
    auto dst = Tensor::zeros({2, 3}, DType::I32);
    ASSERT_OK(dst);
    {
        auto* p = static_cast<int32_t*>(dst->data_ptr());
        p[0]=1; p[1]=2; p[2]=3; p[3]=4; p[4]=5; p[5]=6;
    }

    auto src = Tensor::empty({3}, DType::I32);
    ASSERT_OK(src);
    {
        auto* p = static_cast<int32_t*>(src->data_ptr());
        p[0]=10; p[1]=20; p[2]=30;
    }

    auto r = add_inplace(*dst, *src);
    ASSERT_OK(r);

    const auto* p = static_cast<const int32_t*>(dst->data_ptr());
    EXPECT_EQ(p[0], 11); EXPECT_EQ(p[1], 22); EXPECT_EQ(p[2], 33);
    EXPECT_EQ(p[3], 14); EXPECT_EQ(p[4], 25); EXPECT_EQ(p[5], 36);
}

TEST(ElementWiseTests, AddInplaceNonContiguous)
{
    auto base = Tensor::arange(0, 12, 1, DType::I32);
    ASSERT_OK(base);
    auto m = base->reshape({3, 4});
    ASSERT_OK(m);
    auto dst = m->transpose(0, 1);
    ASSERT_OK(dst);
    EXPECT_FALSE(dst->is_contiguous());

    auto src = Tensor::zeros({4, 3}, DType::I32);
    ASSERT_OK(src);

    auto r = add_inplace(*dst, *src);
    ASSERT_OK(r);

    auto cont = dst->contiguous();
    ASSERT_OK(cont);
    const auto* p = static_cast<const int32_t*>(cont->data_ptr());
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 3; ++j)
            EXPECT_EQ(p[i * 3 + j], j * 4 + i) << "i=" << i << " j=" << j;
}

TEST(ElementWiseTests, AddInplaceBroadcastShapeError)
{
    auto dst = Tensor::zeros({3}, DType::I32);
    auto src = Tensor::zeros({2, 3}, DType::I32);
    ASSERT_OK(dst);
    ASSERT_OK(src);

    auto r = add_inplace(*dst, *src);
    ASSERT_ERR(r);
}

TEST(ElementWiseTests, CudaTensorCreationFails)
{
    auto a = Tensor::empty({4}, DType::F32, Device::CUDA);
    EXPECT_FALSE(a.has_value());
}

TEST(ElementWiseTests, U8NegError)
{
    auto a = Tensor::zeros({3}, DType::U8);
    ASSERT_OK(a);
    auto r = neg(*a);
    ASSERT_ERR(r);
}

TEST(ElementWiseTests, BoolNegError)
{
    auto a = Tensor::zeros({3}, DType::Bool);
    ASSERT_OK(a);
    auto r = neg(*a);
    ASSERT_ERR(r);
}

TEST(ElementWiseTests, U8AbsError)
{
    auto a = Tensor::zeros({3}, DType::U8);
    ASSERT_OK(a);
    auto r = abs(*a);
    ASSERT_ERR(r);
}

TEST(ElementWiseTests, BoolAbsError)
{
    auto a = Tensor::zeros({3}, DType::Bool);
    ASSERT_OK(a);
    auto r = abs(*a);
    ASSERT_ERR(r);
}
