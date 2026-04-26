#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

#include <cmath>

using namespace bee;

TEST(SoftmaxTests, ComputesRowsWithStableShift)
{
    // 计算稳定 softmax，验证结果正确
    auto x = Tensor::arange(0, 6, 1, DType::F32).value().reshape({2, 3}).value();
    auto y = softmax(x, 1).value();

    ASSERT_EQ(y.shape(), (Shape{2, 3}));
    ASSERT_EQ(y.dtype(), DType::F32);
    const auto* p = static_cast<const float*>(y.data_ptr());

    const float e0 = std::exp(0.0f - 2.0f);
    const float e1 = std::exp(1.0f - 2.0f);
    const float e2 = 1.0f;
    const float s  = e0 + e1 + e2;
    EXPECT_NEAR(p[0], e0 / s, 1e-6f);
    EXPECT_NEAR(p[1], e1 / s, 1e-6f);
    EXPECT_NEAR(p[2], e2 / s, 1e-6f);
    EXPECT_NEAR(p[3], e0 / s, 1e-6f);
    EXPECT_NEAR(p[4], e1 / s, 1e-6f);
    EXPECT_NEAR(p[5], e2 / s, 1e-6f);
}

TEST(SoftmaxTests, SupportsNegativeDim)
{
    // 支持负数维度索引
    auto x = Tensor::ones({2, 4}, DType::F64).value();
    auto y = softmax(x, -1).value();

    ASSERT_EQ(y.shape(), (Shape{2, 4}));
    const auto* p = static_cast<const double*>(y.data_ptr());
    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR(p[i], 0.25, 1e-12);
}

TEST(SoftmaxTests, RejectsIntegerInput)
{
    // 拒绝整数输入
    auto x = Tensor::ones({2, 3}, DType::I32).value();
    auto y = softmax(x, 1);
    EXPECT_FALSE(y.has_value());
}

TEST(SoftmaxTests, RejectsOutOfRangeDim)
{
    // 拒绝超出范围的维度
    auto x = Tensor::ones({2, 3}, DType::F32).value();
    auto y = softmax(x, 2);
    EXPECT_FALSE(y.has_value());
}

TEST(SoftmaxTests, F16InputPromotesToF32)
{
    // F16 输入提升为 F32
    auto f32 = Tensor::ones({2, 3}, DType::F32).value();
    auto f16 = cast(f32, DType::F16).value();
    auto y   = softmax(f16, 1).value();
    EXPECT_EQ(y.dtype(), DType::F32);
}

TEST(SoftmaxTests, CudaInputReturnsCudaWhenAvailable)
{
    // CUDA 输入返回 CUDA 张量
    if (!tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过";

    auto x_cpu = Tensor::ones({2, 4}, DType::F32).value();
    auto x_gpu = x_cpu.to(Device::CUDA).value();
    auto y_gpu = softmax(x_gpu, 1).value();
    EXPECT_EQ(y_gpu.device(), Device::CUDA);

    auto        y_cpu = y_gpu.to(Device::CPU).value();
    const auto* p     = static_cast<const float*>(y_cpu.data_ptr());
    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR(p[i], 0.25f, 1e-6f);
}
