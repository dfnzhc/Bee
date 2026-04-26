#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cmath>

using namespace bee;

TEST(LossTests, CrossEntropyComputesMeanLossForI64Targets)
{
    auto  logits = Tensor::zeros({2, 3}, DType::F32).value();
    auto* l      = static_cast<float*>(logits.data_ptr());
    l[0]         = 2.0f;
    l[1]         = 1.0f;
    l[2]         = 0.0f;
    l[3]         = 0.0f;
    l[4]         = 1.0f;
    l[5]         = 2.0f;

    auto  target = Tensor::zeros({2}, DType::I64).value();
    auto* t      = static_cast<int64_t*>(target.data_ptr());
    t[0]         = 0;
    t[1]         = 2;

    auto loss = cross_entropy(logits, target).value();
    ASSERT_EQ(loss.shape(), (Shape{}));
    ASSERT_EQ(loss.dtype(), DType::F32);

    const float row_loss = std::log(std::exp(0.0f) + std::exp(-1.0f) + std::exp(-2.0f));
    EXPECT_NEAR(*static_cast<const float*>(loss.data_ptr()), row_loss, 1e-6f);
}

TEST(LossTests, CrossEntropySupportsI32Targets)
{
    auto  logits = Tensor::zeros({1, 2}, DType::F64).value();
    auto* l      = static_cast<double*>(logits.data_ptr());
    l[0]         = 0.0;
    l[1]         = 0.0;

    auto target = Tensor::zeros({1}, DType::I32).value();
    auto loss   = cross_entropy(logits, target).value();
    EXPECT_NEAR(*static_cast<const double*>(loss.data_ptr()), std::log(2.0), 1e-12);
}

TEST(LossTests, CrossEntropyRejectsOutOfRangeTarget)
{
    auto logits = Tensor::zeros({1, 2}, DType::F32).value();
    auto target = Tensor::full({1}, DType::I64, 3.0).value();
    auto loss   = cross_entropy(logits, target);
    EXPECT_FALSE(loss.has_value());
}

TEST(LossTests, CrossEntropyRejectsWrongShapes)
{
    auto logits = Tensor::zeros({2, 3, 4}, DType::F32).value();
    auto target = Tensor::zeros({2}, DType::I64).value();
    auto loss   = cross_entropy(logits, target);
    EXPECT_FALSE(loss.has_value());
}

TEST(LossTests, F16LogitsPromoteLossToF32)
{
    auto logits_f32 = Tensor::zeros({1, 2}, DType::F32).value();
    auto logits_f16 = cast(logits_f32, DType::F16).value();
    auto target     = Tensor::zeros({1}, DType::I64).value();
    auto loss       = cross_entropy(logits_f16, target).value();
    EXPECT_EQ(loss.dtype(), DType::F32);
}

TEST(LossTests, CudaInputsReturnCudaScalarWhenAvailable)
{
    if (!tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    auto logits = Tensor::zeros({1, 2}, DType::F32).value().to(Device::CUDA).value();
    auto target = Tensor::zeros({1}, DType::I64).value().to(Device::CUDA).value();
    auto loss   = cross_entropy(logits, target).value();
    EXPECT_EQ(loss.device(), Device::CUDA);

    auto cpu = loss.to(Device::CPU).value();
    EXPECT_NEAR(*static_cast<const float*>(cpu.data_ptr()), std::log(2.0f), 1e-6f);
}
