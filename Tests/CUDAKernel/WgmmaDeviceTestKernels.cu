#include "BlockDeviceTestHelpers.hpp"

#include <cuda_runtime.h>

#include "CUDAKernel/Intrinsics.cuh"

namespace
{

bool check_cuda(cudaError_t err)
{
    return err == cudaSuccess;
}

__global__ void wgmma_descriptor_smoke_kernel(uint64_t* out_desc, int* out_flag)
{
    __shared__ half smem[64];

    if (threadIdx.x == 0) {
        bee::cuda::wgmma_fence();
        const uint64_t desc = bee::cuda::make_smem_desc(smem, 128ull);
        bee::cuda::wgmma_commit_group();
        bee::cuda::wgmma_wait_group<0>();

        out_desc[0] = desc;
        out_flag[0] = bee::cuda::has_wgmma_support() ? 1 : 0;
    }
}

__global__ void wgmma_encode_kernel(uint64_t* out)
{
    if (threadIdx.x == 0) {
        out[0] = bee::cuda::matrix_descriptor_encode(0x12340ull);
    }
}

template <int N>
__global__ void wgmma_shape_f32_kernel(uint32_t* out_word)
{
    __shared__ half smem_a[64 * 16];
    __shared__ half smem_b[16 * 64];

    if (threadIdx.x == 0) {
        float d[N / 2] = {};
        bee::cuda::wgmma_fence();
        bee::cuda::wgmma_mma_m64_nk16_f32<N>(d, smem_a, smem_b, static_cast<uint64_t>(N) * sizeof(half));
        bee::cuda::wgmma_commit_group();
        bee::cuda::wgmma_wait_group<0>();

        out_word[0] = __float_as_uint(d[0]) ^ static_cast<uint32_t>(N << 24);
    }
}

template <int N>
__global__ void wgmma_shape_f16_kernel(uint32_t* out_word)
{
    __shared__ half smem_a[64 * 16];
    __shared__ half smem_b[16 * 64];

    if (threadIdx.x == 0) {
        uint32_t d[N / 4] = {};
        bee::cuda::wgmma_fence();
        bee::cuda::wgmma_mma_m64_nk16_f16<N>(d, smem_a, smem_b, static_cast<uint64_t>(N) * sizeof(half));
        bee::cuda::wgmma_commit_group();
        bee::cuda::wgmma_wait_group<0>();

        out_word[0] = d[0] ^ static_cast<uint32_t>(N << 24);
    }
}

template <int N>
bool run_wgmma_shape_f32_impl()
{
    uint32_t h_word  = 0;
    uint32_t* d_word = nullptr;
    if (!check_cuda(cudaMalloc(&d_word, sizeof(uint32_t)))) {
        return false;
    }

    wgmma_shape_f32_kernel<N><<<1, 32>>>(d_word);

    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(&h_word, d_word, sizeof(uint32_t), cudaMemcpyDeviceToHost));

    cudaFree(d_word);

    if (!ok) {
        return false;
    }

    return (h_word >> 24) == static_cast<uint32_t>(N);
}

template <int N>
bool run_wgmma_shape_f16_impl()
{
    uint32_t h_word  = 0;
    uint32_t* d_word = nullptr;
    if (!check_cuda(cudaMalloc(&d_word, sizeof(uint32_t)))) {
        return false;
    }

    wgmma_shape_f16_kernel<N><<<1, 32>>>(d_word);

    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(&h_word, d_word, sizeof(uint32_t), cudaMemcpyDeviceToHost));

    cudaFree(d_word);

    if (!ok) {
        return false;
    }

    return (h_word >> 24) == static_cast<uint32_t>(N);
}

} // namespace

bool run_wgmma_descriptor_smoke()
{
    uint64_t h_desc = 0;
    int h_flag      = 0;

    uint64_t* d_desc = nullptr;
    int* d_flag      = nullptr;
    if (!check_cuda(cudaMalloc(&d_desc, sizeof(uint64_t)))) {
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_flag, sizeof(int)))) {
        cudaFree(d_desc);
        return false;
    }

    wgmma_descriptor_smoke_kernel<<<1, 32>>>(d_desc, d_flag);

    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(&h_desc, d_desc, sizeof(uint64_t), cudaMemcpyDeviceToHost));
    ok      = ok && check_cuda(cudaMemcpy(&h_flag, d_flag, sizeof(int), cudaMemcpyDeviceToHost));

    cudaFree(d_desc);
    cudaFree(d_flag);

    if (!ok) {
        return false;
    }

    if ((h_desc & (1ull << 62)) == 0ull) {
        return false;
    }

    return h_flag == 0 || h_flag == 1;
}

bool run_wgmma_descriptor_encode_device()
{
    uint64_t h_encoded  = 0;
    uint64_t* d_encoded = nullptr;
    if (!check_cuda(cudaMalloc(&d_encoded, sizeof(uint64_t)))) {
        return false;
    }

    wgmma_encode_kernel<<<1, 1>>>(d_encoded);

    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(&h_encoded, d_encoded, sizeof(uint64_t), cudaMemcpyDeviceToHost));

    cudaFree(d_encoded);

    if (!ok) {
        return false;
    }

    return h_encoded == ((0x12340ull & 0x3FFFFull) >> 4);
}

bool run_wgmma_shape_f32(int n)
{
    switch (n) {
    case 8:
        return run_wgmma_shape_f32_impl<8>();
    case 16:
        return run_wgmma_shape_f32_impl<16>();
    case 24:
        return run_wgmma_shape_f32_impl<24>();
    case 32:
        return run_wgmma_shape_f32_impl<32>();
    case 40:
        return run_wgmma_shape_f32_impl<40>();
    case 48:
        return run_wgmma_shape_f32_impl<48>();
    case 56:
        return run_wgmma_shape_f32_impl<56>();
    case 64:
        return run_wgmma_shape_f32_impl<64>();
    default:
        return false;
    }
}

bool run_wgmma_shape_f16(int n)
{
    switch (n) {
    case 8:
        return run_wgmma_shape_f16_impl<8>();
    case 16:
        return run_wgmma_shape_f16_impl<16>();
    case 24:
        return run_wgmma_shape_f16_impl<24>();
    case 32:
        return run_wgmma_shape_f16_impl<32>();
    case 40:
        return run_wgmma_shape_f16_impl<40>();
    case 48:
        return run_wgmma_shape_f16_impl<48>();
    case 56:
        return run_wgmma_shape_f16_impl<56>();
    case 64:
        return run_wgmma_shape_f16_impl<64>();
    default:
        return false;
    }
}
