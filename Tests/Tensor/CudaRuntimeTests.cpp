#include "Tensor/Cuda/ExecContext.hpp"
#include "Tensor/Cuda/Backend.hpp"
#include "Tensor/Tensor.hpp"

#include <gtest/gtest.h>

TEST(CudaRuntimeTests, DefaultExecContextStartsEmpty)
{
    auto ctx = bee::tensor::cuda::make_default_exec_context();
    EXPECT_EQ(ctx.stream, nullptr);
    EXPECT_EQ(ctx.workspace, nullptr);
    EXPECT_EQ(ctx.workspace_bytes, std::size_t{0});
    EXPECT_FALSE(ctx.synchronize_on_exit);
}

TEST(CudaRuntimeTests, AsyncCopyApiExistsAndRoundTripsShape)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    auto src = bee::Tensor::ones({2, 3}, bee::DType::F32, bee::Device::CPU).value();
    auto ctx = bee::tensor::cuda::make_default_exec_context();
    auto dst = src.to(bee::Device::CUDA, &ctx).value();
    auto back = dst.to(bee::Device::CPU, &ctx).value();

    EXPECT_EQ(back.shape(), src.shape());
    EXPECT_EQ(back.dtype(), src.dtype());
}
