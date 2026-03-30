#include <gtest/gtest.h>

#include "CUDAKernel/Launch.cuh"

TEST(CUDAKernelLaunchTests, CeilDivInt)
{
    EXPECT_EQ(bee::cuda::ceil_div(10, 3), 4);
    EXPECT_EQ(bee::cuda::ceil_div(12, 3), 4);
}

TEST(CUDAKernelLaunchTests, CeilDivInt64)
{
    EXPECT_EQ(bee::cuda::ceil_div(static_cast<std::int64_t>(33), static_cast<std::int64_t>(8)), 5);
    EXPECT_EQ(bee::cuda::ceil_div(static_cast<std::int64_t>(64), static_cast<std::int64_t>(8)), 8);
}

TEST(CUDAKernelLaunchTests, Make1DConfig)
{
    const bee::cuda::Config cfg = bee::cuda::make_1d(1025, 256, 128);
    EXPECT_EQ(cfg.block.x, 256u);
    EXPECT_EQ(cfg.grid.x, 5u);
    EXPECT_EQ(cfg.dynamic_shared_bytes, 128u);
}
