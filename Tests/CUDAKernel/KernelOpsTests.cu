#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

#include "CUDAKernel/Elementwise.cuh"
#include "CUDAKernel/LayerNorm.cuh"
#include "CUDAKernel/Reduce.cuh"
#include "CUDAKernel/SoftMax.cuh"
#include "CUDAKernel/Transpose.cuh"

namespace
{
struct SquarePlusOne
{
    __host__ __device__ float operator()(float x) const
    {
        return x * x + 1.0f;
    }
};

struct AffineAdd
{
    __host__ __device__ float operator()(float a, float b) const
    {
        return a + 2.0f * b;
    }
};

struct Mix3
{
    __host__ __device__ float operator()(float a, float b, float c) const
    {
        return a + b - c;
    }
};

struct SumInt
{
    __host__ __device__ int operator()(int a, int b) const
    {
        return a + b;
    }
};

void expect_cuda_success(cudaError_t err)
{
    ASSERT_EQ(err, cudaSuccess) << cudaGetErrorString(err);
}
} // namespace

TEST(CUDAKernelOpsTests, ElementwiseUnaryBinaryTernaryFloat)
{
    constexpr int n = 37; // 覆盖尾部不足向量宽度的路径。

    std::vector<float> h_in0(n);
    std::vector<float> h_in1(n);
    std::vector<float> h_in2(n);
    for (int i = 0; i < n; ++i) {
        h_in0[i] = static_cast<float>(i) * 0.25f;
        h_in1[i] = static_cast<float>(i % 5) - 1.0f;
        h_in2[i] = static_cast<float>(i % 3) + 0.5f;
    }

    float* d_in0 = nullptr;
    float* d_in1 = nullptr;
    float* d_in2 = nullptr;
    float* d_out = nullptr;

    expect_cuda_success(cudaMalloc(&d_in0, n * sizeof(float)));
    expect_cuda_success(cudaMalloc(&d_in1, n * sizeof(float)));
    expect_cuda_success(cudaMalloc(&d_in2, n * sizeof(float)));
    expect_cuda_success(cudaMalloc(&d_out, n * sizeof(float)));

    expect_cuda_success(cudaMemcpy(d_in0, h_in0.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    expect_cuda_success(cudaMemcpy(d_in1, h_in1.data(), n * sizeof(float), cudaMemcpyHostToDevice));
    expect_cuda_success(cudaMemcpy(d_in2, h_in2.data(), n * sizeof(float), cudaMemcpyHostToDevice));

    expect_cuda_success(bee::cuda::launch_elementwise_unary(d_out, d_in0, n, SquarePlusOne{}));
    expect_cuda_success(cudaDeviceSynchronize());

    std::vector<float> h_out(n);
    expect_cuda_success(cudaMemcpy(h_out.data(), d_out, n * sizeof(float), cudaMemcpyDeviceToHost));
    for (int i = 0; i < n; ++i) {
        EXPECT_NEAR(h_out[i], h_in0[i] * h_in0[i] + 1.0f, 1e-6f);
    }

    expect_cuda_success(bee::cuda::launch_elementwise_binary(d_out, d_in0, d_in1, n, AffineAdd{}));
    expect_cuda_success(cudaDeviceSynchronize());
    expect_cuda_success(cudaMemcpy(h_out.data(), d_out, n * sizeof(float), cudaMemcpyDeviceToHost));
    for (int i = 0; i < n; ++i) {
        EXPECT_NEAR(h_out[i], h_in0[i] + 2.0f * h_in1[i], 1e-6f);
    }

    expect_cuda_success(bee::cuda::launch_elementwise_ternary(d_out, d_in0, d_in1, d_in2, n, Mix3{}));
    expect_cuda_success(cudaDeviceSynchronize());
    expect_cuda_success(cudaMemcpy(h_out.data(), d_out, n * sizeof(float), cudaMemcpyDeviceToHost));
    for (int i = 0; i < n; ++i) {
        EXPECT_NEAR(h_out[i], h_in0[i] + h_in1[i] - h_in2[i], 1e-6f);
    }

    cudaFree(d_in0);
    cudaFree(d_in1);
    cudaFree(d_in2);
    cudaFree(d_out);
}

TEST(CUDAKernelOpsTests, ReduceSumAndEmptyInput)
{
    constexpr int n = 1024;

    std::vector<int> h_in(n);
    for (int i = 0; i < n; ++i) {
        h_in[i] = (i % 7) - 3;
    }
    const int expected_sum = std::accumulate(h_in.begin(), h_in.end(), 0);

    int* d_in  = nullptr;
    int* d_out = nullptr;

    expect_cuda_success(cudaMalloc(&d_in, n * sizeof(int)));
    expect_cuda_success(cudaMalloc(&d_out, sizeof(int)));
    expect_cuda_success(cudaMemcpy(d_in, h_in.data(), n * sizeof(int), cudaMemcpyHostToDevice));

    expect_cuda_success(bee::cuda::launch_reduce(d_in, d_out, n, 0, SumInt{}));
    expect_cuda_success(cudaDeviceSynchronize());

    int h_out = 0;
    expect_cuda_success(cudaMemcpy(&h_out, d_out, sizeof(int), cudaMemcpyDeviceToHost));
    EXPECT_EQ(h_out, expected_sum);

    // n<=0 时应直接返回 identity。
    expect_cuda_success(bee::cuda::launch_reduce(d_in, d_out, 0, 123, SumInt{}));
    expect_cuda_success(cudaDeviceSynchronize());
    expect_cuda_success(cudaMemcpy(&h_out, d_out, sizeof(int), cudaMemcpyDeviceToHost));
    EXPECT_EQ(h_out, 123);

    cudaFree(d_in);
    cudaFree(d_out);
}

TEST(CUDAKernelOpsTests, SoftmaxRowSumsToOne)
{
    constexpr int rows = 3;
    constexpr int cols = 7;

    std::vector<float> h_x(rows * cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            h_x[r * cols + c] = static_cast<float>(r * 3 + c - 4);
        }
    }

    float* d_x = nullptr;
    float* d_y = nullptr;

    expect_cuda_success(cudaMalloc(&d_x, h_x.size() * sizeof(float)));
    expect_cuda_success(cudaMalloc(&d_y, h_x.size() * sizeof(float)));
    expect_cuda_success(cudaMemcpy(d_x, h_x.data(), h_x.size() * sizeof(float), cudaMemcpyHostToDevice));

    expect_cuda_success(bee::cuda::launch_softmax(d_x, d_y, rows, cols));
    expect_cuda_success(cudaDeviceSynchronize());

    std::vector<float> h_y(h_x.size());
    expect_cuda_success(cudaMemcpy(h_y.data(), d_y, h_y.size() * sizeof(float), cudaMemcpyDeviceToHost));

    for (int r = 0; r < rows; ++r) {
        float sum = 0.0f;
        for (int c = 0; c < cols; ++c) {
            const float v = h_y[r * cols + c];
            EXPECT_GT(v, 0.0f);
            sum += v;
        }
        EXPECT_NEAR(sum, 1.0f, 1e-5f);
    }

    cudaFree(d_x);
    cudaFree(d_y);
}

TEST(CUDAKernelOpsTests, LayerNormMatchesReference)
{
    constexpr int rows = 2;
    constexpr int cols = 8;
    constexpr float eps = 1e-5f;

    std::vector<float> h_x(rows * cols);
    std::vector<float> h_gamma(cols);
    std::vector<float> h_beta(cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            h_x[r * cols + c] = static_cast<float>((r + 1) * (c - 3));
        }
    }
    for (int c = 0; c < cols; ++c) {
        h_gamma[c] = 0.5f + 0.1f * static_cast<float>(c);
        h_beta[c] = -0.3f + 0.05f * static_cast<float>(c);
    }

    float* d_x = nullptr;
    float* d_gamma = nullptr;
    float* d_beta = nullptr;
    float* d_y = nullptr;

    expect_cuda_success(cudaMalloc(&d_x, h_x.size() * sizeof(float)));
    expect_cuda_success(cudaMalloc(&d_gamma, h_gamma.size() * sizeof(float)));
    expect_cuda_success(cudaMalloc(&d_beta, h_beta.size() * sizeof(float)));
    expect_cuda_success(cudaMalloc(&d_y, h_x.size() * sizeof(float)));

    expect_cuda_success(cudaMemcpy(d_x, h_x.data(), h_x.size() * sizeof(float), cudaMemcpyHostToDevice));
    expect_cuda_success(cudaMemcpy(d_gamma, h_gamma.data(), h_gamma.size() * sizeof(float), cudaMemcpyHostToDevice));
    expect_cuda_success(cudaMemcpy(d_beta, h_beta.data(), h_beta.size() * sizeof(float), cudaMemcpyHostToDevice));

    expect_cuda_success(bee::cuda::launch_layer_norm(d_x, d_gamma, d_beta, d_y, rows, cols, eps));
    expect_cuda_success(cudaDeviceSynchronize());

    std::vector<float> h_y(h_x.size());
    expect_cuda_success(cudaMemcpy(h_y.data(), d_y, h_y.size() * sizeof(float), cudaMemcpyDeviceToHost));

    for (int r = 0; r < rows; ++r) {
        float mean = 0.0f;
        for (int c = 0; c < cols; ++c) {
            mean += h_x[r * cols + c];
        }
        mean /= static_cast<float>(cols);

        float var = 0.0f;
        for (int c = 0; c < cols; ++c) {
            const float d = h_x[r * cols + c] - mean;
            var += d * d;
        }
        var /= static_cast<float>(cols);

        const float inv_std = 1.0f / std::sqrt(var + eps);
        for (int c = 0; c < cols; ++c) {
            const float expected = (h_x[r * cols + c] - mean) * inv_std * h_gamma[c] + h_beta[c];
            EXPECT_NEAR(h_y[r * cols + c], expected, 2e-4f);
        }
    }

    // 标量仿射参数路径。
    constexpr float gamma_s = 1.25f;
    constexpr float beta_s = -0.75f;
    expect_cuda_success(bee::cuda::launch_layer_norm_affine_scalar(d_x, gamma_s, beta_s, d_y, rows, cols, eps));
    expect_cuda_success(cudaDeviceSynchronize());
    expect_cuda_success(cudaMemcpy(h_y.data(), d_y, h_y.size() * sizeof(float), cudaMemcpyDeviceToHost));

    for (int r = 0; r < rows; ++r) {
        float mean = 0.0f;
        for (int c = 0; c < cols; ++c) {
            mean += h_x[r * cols + c];
        }
        mean /= static_cast<float>(cols);

        float var = 0.0f;
        for (int c = 0; c < cols; ++c) {
            const float d = h_x[r * cols + c] - mean;
            var += d * d;
        }
        var /= static_cast<float>(cols);

        const float inv_std = 1.0f / std::sqrt(var + eps);
        for (int c = 0; c < cols; ++c) {
            const float expected = (h_x[r * cols + c] - mean) * inv_std * gamma_s + beta_s;
            EXPECT_NEAR(h_y[r * cols + c], expected, 2e-4f);
        }
    }

    cudaFree(d_x);
    cudaFree(d_gamma);
    cudaFree(d_beta);
    cudaFree(d_y);
}

TEST(CUDAKernelOpsTests, TransposeProducesExpectedLayout)
{
    constexpr int rows = 5;
    constexpr int cols = 7;

    std::vector<int> h_in(rows * cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            h_in[r * cols + c] = r * 100 + c;
        }
    }

    int* d_in = nullptr;
    int* d_out = nullptr;

    expect_cuda_success(cudaMalloc(&d_in, h_in.size() * sizeof(int)));
    expect_cuda_success(cudaMalloc(&d_out, h_in.size() * sizeof(int)));
    expect_cuda_success(cudaMemcpy(d_in, h_in.data(), h_in.size() * sizeof(int), cudaMemcpyHostToDevice));

    expect_cuda_success(bee::cuda::launch_transpose(d_in, d_out, rows, cols));
    expect_cuda_success(cudaDeviceSynchronize());

    std::vector<int> h_out(h_in.size());
    expect_cuda_success(cudaMemcpy(h_out.data(), d_out, h_out.size() * sizeof(int), cudaMemcpyDeviceToHost));

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            EXPECT_EQ(h_out[c * rows + r], h_in[r * cols + c]);
        }
    }

    cudaFree(d_in);
    cudaFree(d_out);
}
