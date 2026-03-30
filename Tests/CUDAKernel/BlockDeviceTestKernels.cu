#include "BlockDeviceTestHelpers.hpp"

#include <cuda_runtime.h>

#include <array>
#include <vector>

#include "CUDAKernel/Block.cuh"
#include "CUDAKernel/Warp.cuh"

constexpr int kBlockSize   = 128;
constexpr int kWarpThreads = 32;

__global__ void exclusive_scan_kernel(const int* in, int* out)
{
    const int tid = static_cast<int>(threadIdx.x);
    out[tid]      = bee::cuda::exclusive_scan<int, kBlockSize>(in[tid]);
}

__global__ void inclusive_scan_kernel(const int* in, int* out)
{
    const int tid = static_cast<int>(threadIdx.x);
    out[tid]      = bee::cuda::inclusive_scan<int, kBlockSize>(in[tid]);
}

__global__ void compact_kernel(int* out_idx, int* out_count)
{
    const int tid   = static_cast<int>(threadIdx.x);
    const bool pred = (tid % 3) == 0;
    out_idx[tid]    = bee::cuda::compact_index<kBlockSize>(pred);
    out_count[tid]  = bee::cuda::compact_count<kBlockSize>(pred);
}

__global__ void warp_reduce_sum_kernel(int* out_leader, int* out_all)
{
    const int lane  = static_cast<int>(threadIdx.x);
    const int value = lane + 1;

    const int leader_sum = bee::cuda::reduce_sum<int>(value);
    const int all_sum    = bee::cuda::all_reduce_sum<int>(value);

    if (lane == 0) {
        out_leader[0] = leader_sum;
    }
    out_all[lane] = all_sum;
}

__global__ void warp_prefix_sum_kernel(int* out_exclusive, int* out_inclusive)
{
    const int lane  = static_cast<int>(threadIdx.x);
    const int value = (lane % 5) + 1;

    out_exclusive[lane] = bee::cuda::exclusive_prefix_sum<int>(value);
    out_inclusive[lane] = bee::cuda::inclusive_prefix_sum<int>(value);
}

__global__ void warp_vote_compact_kernel(int* out_idx, int* out_count, unsigned* out_ballot)
{
    const int lane  = static_cast<int>(threadIdx.x);
    const bool pred = (lane % 3) == 0;

    out_idx[lane]    = bee::cuda::compact_index(pred);
    out_count[lane]  = bee::cuda::count_if(pred);
    out_ballot[lane] = bee::cuda::ballot(pred);
}

bool check_cuda(cudaError_t err)
{
    return err == cudaSuccess;
}

bool run_block_exclusive_scan_i32()
{
    std::vector<int> h_in(kBlockSize);
    std::vector<int> h_out(kBlockSize, 0);
    std::vector<int> expected(kBlockSize, 0);

    for (int i = 0; i < kBlockSize; ++i) {
        h_in[i] = (i % 5) + 1;
    }

    int prefix = 0;
    for (int i = 0; i < kBlockSize; ++i) {
        expected[i]  = prefix;
        prefix      += h_in[i];
    }

    int* d_in  = nullptr;
    int* d_out = nullptr;
    if (!check_cuda(cudaMalloc(&d_in, sizeof(int) * kBlockSize))) {
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_out, sizeof(int) * kBlockSize))) {
        cudaFree(d_in);
        return false;
    }

    bool ok = true;
    ok      = ok && check_cuda(cudaMemcpy(d_in, h_in.data(), sizeof(int) * kBlockSize, cudaMemcpyHostToDevice));
    if (ok) {
        exclusive_scan_kernel<<<1, kBlockSize>>>(d_in, d_out);
        ok = ok && check_cuda(cudaGetLastError());
        ok = ok && check_cuda(cudaDeviceSynchronize());
        ok = ok && check_cuda(cudaMemcpy(h_out.data(), d_out, sizeof(int) * kBlockSize, cudaMemcpyDeviceToHost));
    }

    cudaFree(d_in);
    cudaFree(d_out);

    if (!ok) {
        return false;
    }

    return h_out == expected;
}

bool run_block_inclusive_scan_i32()
{
    std::vector<int> h_in(kBlockSize);
    std::vector<int> h_out(kBlockSize, 0);
    std::vector<int> expected(kBlockSize, 0);

    for (int i = 0; i < kBlockSize; ++i) {
        h_in[i] = (i % 7) + 1;
    }

    int prefix = 0;
    for (int i = 0; i < kBlockSize; ++i) {
        prefix      += h_in[i];
        expected[i]  = prefix;
    }

    int* d_in  = nullptr;
    int* d_out = nullptr;
    if (!check_cuda(cudaMalloc(&d_in, sizeof(int) * kBlockSize))) {
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_out, sizeof(int) * kBlockSize))) {
        cudaFree(d_in);
        return false;
    }

    bool ok = true;
    ok      = ok && check_cuda(cudaMemcpy(d_in, h_in.data(), sizeof(int) * kBlockSize, cudaMemcpyHostToDevice));
    if (ok) {
        inclusive_scan_kernel<<<1, kBlockSize>>>(d_in, d_out);
        ok = ok && check_cuda(cudaGetLastError());
        ok = ok && check_cuda(cudaDeviceSynchronize());
        ok = ok && check_cuda(cudaMemcpy(h_out.data(), d_out, sizeof(int) * kBlockSize, cudaMemcpyDeviceToHost));
    }

    cudaFree(d_in);
    cudaFree(d_out);

    if (!ok) {
        return false;
    }

    return h_out == expected;
}

bool run_block_compact_i32()
{
    std::vector<int> h_idx(kBlockSize, 0);
    std::vector<int> h_count(kBlockSize, 0);
    std::vector<int> expected_idx(kBlockSize, 0);

    int running_true = 0;
    for (int i = 0; i < kBlockSize; ++i) {
        expected_idx[i] = running_true;
        if ((i % 3) == 0) {
            ++running_true;
        }
    }
    const int expected_count = running_true;

    int* d_idx   = nullptr;
    int* d_count = nullptr;
    if (!check_cuda(cudaMalloc(&d_idx, sizeof(int) * kBlockSize))) {
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_count, sizeof(int) * kBlockSize))) {
        cudaFree(d_idx);
        return false;
    }

    compact_kernel<<<1, kBlockSize>>>(d_idx, d_count);
    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(h_idx.data(), d_idx, sizeof(int) * kBlockSize, cudaMemcpyDeviceToHost));
    ok      = ok && check_cuda(cudaMemcpy(h_count.data(), d_count, sizeof(int) * kBlockSize, cudaMemcpyDeviceToHost));

    cudaFree(d_idx);
    cudaFree(d_count);

    if (!ok) {
        return false;
    }

    if (h_idx != expected_idx) {
        return false;
    }

    for (int i = 0; i < kBlockSize; ++i) {
        if (h_count[i] != expected_count) {
            return false;
        }
    }

    return true;
}

bool run_warp_reduce_sum_i32()
{
    int h_leader = 0;
    std::array<int, kWarpThreads> h_all{};

    int* d_leader = nullptr;
    int* d_all    = nullptr;
    if (!check_cuda(cudaMalloc(&d_leader, sizeof(int)))) {
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_all, sizeof(int) * kWarpThreads))) {
        cudaFree(d_leader);
        return false;
    }

    warp_reduce_sum_kernel<<<1, kWarpThreads>>>(d_leader, d_all);

    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(&h_leader, d_leader, sizeof(int), cudaMemcpyDeviceToHost));
    ok      = ok && check_cuda(cudaMemcpy(h_all.data(), d_all, sizeof(int) * kWarpThreads, cudaMemcpyDeviceToHost));

    cudaFree(d_leader);
    cudaFree(d_all);

    if (!ok) {
        return false;
    }

    constexpr int expected_sum = (kWarpThreads * (kWarpThreads + 1)) / 2;
    if (h_leader != expected_sum) {
        return false;
    }

    for (int i = 0; i < kWarpThreads; ++i) {
        if (h_all[i] != expected_sum) {
            return false;
        }
    }

    return true;
}

bool run_warp_prefix_sum_i32()
{
    std::array<int, kWarpThreads> h_exclusive{};
    std::array<int, kWarpThreads> h_inclusive{};
    std::array<int, kWarpThreads> expected_exclusive{};
    std::array<int, kWarpThreads> expected_inclusive{};

    int prefix = 0;
    for (int lane = 0; lane < kWarpThreads; ++lane) {
        const int value           = (lane % 5) + 1;
        expected_exclusive[lane]  = prefix;
        prefix                   += value;
        expected_inclusive[lane]  = prefix;
    }

    int* d_exclusive = nullptr;
    int* d_inclusive = nullptr;
    if (!check_cuda(cudaMalloc(&d_exclusive, sizeof(int) * kWarpThreads))) {
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_inclusive, sizeof(int) * kWarpThreads))) {
        cudaFree(d_exclusive);
        return false;
    }

    warp_prefix_sum_kernel<<<1, kWarpThreads>>>(d_exclusive, d_inclusive);

    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(h_exclusive.data(), d_exclusive, sizeof(int) * kWarpThreads, cudaMemcpyDeviceToHost));
    ok      = ok && check_cuda(cudaMemcpy(h_inclusive.data(), d_inclusive, sizeof(int) * kWarpThreads, cudaMemcpyDeviceToHost));

    cudaFree(d_exclusive);
    cudaFree(d_inclusive);

    if (!ok) {
        return false;
    }

    return h_exclusive == expected_exclusive && h_inclusive == expected_inclusive;
}

bool run_warp_vote_and_compact()
{
    std::array<int, kWarpThreads> h_idx{};
    std::array<int, kWarpThreads> h_count{};
    std::array<unsigned, kWarpThreads> h_ballot{};

    std::array<int, kWarpThreads> expected_idx{};
    int running_true         = 0;
    unsigned expected_ballot = 0u;
    for (int lane = 0; lane < kWarpThreads; ++lane) {
        expected_idx[lane] = running_true;
        if ((lane % 3) == 0) {
            ++running_true;
            expected_ballot |= (1u << lane);
        }
    }
    const int expected_count = running_true;

    int* d_idx         = nullptr;
    int* d_count       = nullptr;
    unsigned* d_ballot = nullptr;
    if (!check_cuda(cudaMalloc(&d_idx, sizeof(int) * kWarpThreads))) {
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_count, sizeof(int) * kWarpThreads))) {
        cudaFree(d_idx);
        return false;
    }
    if (!check_cuda(cudaMalloc(&d_ballot, sizeof(unsigned) * kWarpThreads))) {
        cudaFree(d_idx);
        cudaFree(d_count);
        return false;
    }

    warp_vote_compact_kernel<<<1, kWarpThreads>>>(d_idx, d_count, d_ballot);

    bool ok = true;
    ok      = ok && check_cuda(cudaGetLastError());
    ok      = ok && check_cuda(cudaDeviceSynchronize());
    ok      = ok && check_cuda(cudaMemcpy(h_idx.data(), d_idx, sizeof(int) * kWarpThreads, cudaMemcpyDeviceToHost));
    ok      = ok && check_cuda(cudaMemcpy(h_count.data(), d_count, sizeof(int) * kWarpThreads, cudaMemcpyDeviceToHost));
    ok      = ok && check_cuda(cudaMemcpy(h_ballot.data(), d_ballot, sizeof(unsigned) * kWarpThreads, cudaMemcpyDeviceToHost));

    cudaFree(d_idx);
    cudaFree(d_count);
    cudaFree(d_ballot);

    if (!ok) {
        return false;
    }

    if (h_idx != expected_idx) {
        return false;
    }

    for (int lane = 0; lane < kWarpThreads; ++lane) {
        if (h_count[lane] != expected_count) {
            return false;
        }
        if (h_ballot[lane] != expected_ballot) {
            return false;
        }
    }

    return true;
}
