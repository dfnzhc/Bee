#include <gtest/gtest.h>

#include "BlockDeviceTestHelpers.hpp"

TEST(CUDAKernelBlockDeviceTests, ExclusiveScanInt32)
{
    EXPECT_TRUE(run_block_exclusive_scan_i32());
}

TEST(CUDAKernelBlockDeviceTests, InclusiveScanInt32)
{
    EXPECT_TRUE(run_block_inclusive_scan_i32());
}

TEST(CUDAKernelBlockDeviceTests, CompactHelpersInt32)
{
    EXPECT_TRUE(run_block_compact_i32());
}

TEST(CUDAKernelWarpDeviceTests, ReduceSumInt32)
{
    EXPECT_TRUE(run_warp_reduce_sum_i32());
}

TEST(CUDAKernelWarpDeviceTests, PrefixSumInt32)
{
    EXPECT_TRUE(run_warp_prefix_sum_i32());
}

TEST(CUDAKernelWarpDeviceTests, VoteAndCompact)
{
    EXPECT_TRUE(run_warp_vote_and_compact());
}

TEST(CUDAKernelWgmmaTests, DescriptorSmoke)
{
    EXPECT_TRUE(run_wgmma_descriptor_smoke());
}

TEST(CUDAKernelWgmmaTests, DescriptorEncodeOnDevice)
{
    EXPECT_TRUE(run_wgmma_descriptor_encode_device());
}

class CUDAKernelWgmmaShapeF32Tests : public ::testing::TestWithParam<int>
{
};

TEST_P(CUDAKernelWgmmaShapeF32Tests, CoversEachNShape)
{
    EXPECT_TRUE(run_wgmma_shape_f32(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(ByNShape, CUDAKernelWgmmaShapeF32Tests, ::testing::Values(8, 16, 24, 32, 40, 48, 56, 64));

class CUDAKernelWgmmaShapeF16Tests : public ::testing::TestWithParam<int>
{
};

TEST_P(CUDAKernelWgmmaShapeF16Tests, CoversEachNShape)
{
    EXPECT_TRUE(run_wgmma_shape_f16(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(ByNShape, CUDAKernelWgmmaShapeF16Tests, ::testing::Values(8, 16, 24, 32, 40, 48, 56, 64));
