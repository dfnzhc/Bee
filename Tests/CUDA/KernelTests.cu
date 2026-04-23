/**
 * @File KernelTests.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA kernel 级测试占位；将在 M1/M3 逐步填充。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include "CUDA/Core/Check.cuh"
#include "CUDA/Core/Launch.cuh"
#include "CUDA/Core/Ptx.cuh"
#include "CUDA/Core/ArchDispatch.cuh"
#include "CUDA/DeviceView/TensorView.cuh"
#include "CUDA/DeviceView/Vectorize.cuh"

namespace
{

__global__ void bee_m1_touch_kernel(int* out, int n)
{
    const unsigned idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (static_cast<int>(idx) < n) {
        out[idx] = static_cast<int>(idx);
    }
}

// host 封装：返回 int 错误码（见 Core/Check.cuh 约定）。
int bee_m1_touch_launch(int* d_out, int n)
{
    const unsigned int block = bee::cuda::kDefaultBlockSize;
    const unsigned int grid  = bee::cuda::compute_grid_1d(static_cast<unsigned long long>(n), block);
    BEE_CUDA_LAUNCH(bee_m1_touch_kernel<<<grid, block>>>(d_out, n));
    BEE_CUDA_RET_ON_ERR(cudaDeviceSynchronize());
    return BEE_CUDA_OK;
}

} // namespace

TEST(CudaM1Infra, LaunchKernelWritesExpectedValues)
{
    constexpr int N = 1024;
    int* d = nullptr;
    ASSERT_EQ(cudaMalloc(&d, N * sizeof(int)), cudaSuccess);

    const int launch_err = bee_m1_touch_launch(d, N);
    if (launch_err != BEE_CUDA_OK) {
        if (launch_err == static_cast<int>(cudaErrorNoKernelImageForDevice) || launch_err == static_cast<int>(cudaErrorInvalidDeviceFunction)) {
            (void)cudaFree(d);
            GTEST_SKIP() << "Kernel image is incompatible with current GPU architecture (err=" << launch_err << ")";
        }
        EXPECT_EQ(launch_err, BEE_CUDA_OK);
        (void)cudaFree(d);
        return;
    }

    int h[N] = {};
    ASSERT_EQ(cudaMemcpy(h, d, N * sizeof(int), cudaMemcpyDeviceToHost), cudaSuccess);
    (void)cudaFree(d);

    for (int i = 0; i < N; ++i) ASSERT_EQ(h[i], i);
}

TEST(CudaM1Infra, GridSizeHelpers)
{
    EXPECT_EQ(bee::cuda::compute_grid_1d(1, 256), 1u);
    EXPECT_EQ(bee::cuda::compute_grid_1d(256, 256), 1u);
    EXPECT_EQ(bee::cuda::compute_grid_1d(257, 256), 2u);
    EXPECT_EQ(bee::cuda::ceil_div_u(10, 3), 4u);
}

TEST(CudaM1Infra, TensorViewOffsetOnHost)
{
    bee::cuda::TensorView<float, 2> v{};
    v.shape[0] = 3; v.shape[1] = 4;
    v.stride[0] = 4; v.stride[1] = 1;
    // (row=2, col=1) in row-major: linear idx = 2*4 + 1 = 9, offset = 2*4 + 1*1 = 9
    EXPECT_EQ(v.offset_linear(9), 9);
    EXPECT_EQ(v.numel(), 12);
}

TEST(CudaM1Infra, VectorizeAlignmentHost)
{
    alignas(16) char buf[32] = {};
    EXPECT_TRUE(bee::cuda::is_aligned<16>(buf));
    EXPECT_TRUE(bee::cuda::is_aligned<16>(buf + 16));
    EXPECT_FALSE(bee::cuda::is_aligned<16>(buf + 1));
}
