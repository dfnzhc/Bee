/**
 * @File Tests/CUDA/MatmulTests.cu
 * @Brief CUDA matmul 算子的正确性测试，覆盖基本矩阵乘法与非 4 对齐 fallback 路径。
 */

#include <gtest/gtest.h>

#include <cuda_runtime.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "CUDA/Ops/OpsBridge.hpp"

namespace
{

constexpr int kDtF32 = 4;

auto is_kernel_image_missing(int err) -> bool
{
    return err == static_cast<int>(cudaErrorNoKernelImageForDevice)
        || err == static_cast<int>(cudaErrorInvalidDeviceFunction);
}

// matmul 测试结果封装
template <typename T>
struct MatmulResult
{
    bool             skipped = false; // kernel image 缺失时跳过
    bool             ok      = false; // 计算成功
    std::vector<T>   value{};         // 输出数据
};

// 集中资源管理的 matmul 执行辅助函数
template <typename T>
auto run_matmul_f32(std::size_t m, std::size_t k, std::size_t n,
                    const std::vector<T>& a, const std::vector<T>& b) -> MatmulResult<T>
{
    T* da = nullptr;
    T* db = nullptr;
    T* dc = nullptr;
    MatmulResult<T> result{};

    // 清理资源的 lambda，确保所有返回路径都释放
    auto cleanup = [&]() {
        if (dc) (void)cudaFree(dc);
        if (db) (void)cudaFree(db);
        if (da) (void)cudaFree(da);
    };

    // 分配 A
    EXPECT_EQ(cudaMalloc(&da, a.size() * sizeof(T)), cudaSuccess);
    if (!da) {
        cleanup();
        return result;
    }

    // 分配 B
    EXPECT_EQ(cudaMalloc(&db, b.size() * sizeof(T)), cudaSuccess);
    if (!db) {
        cleanup();
        return result;
    }

    // 分配 C
    const std::size_t c_size = m * n;
    EXPECT_EQ(cudaMalloc(&dc, c_size * sizeof(T)), cudaSuccess);
    if (!dc) {
        cleanup();
        return result;
    }

    // 拷贝输入
    EXPECT_EQ(cudaMemcpy(da, a.data(), a.size() * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);
    EXPECT_EQ(cudaMemcpy(db, b.data(), b.size() * sizeof(T), cudaMemcpyHostToDevice), cudaSuccess);

    // 调用 matmul
    const int err = bee::cuda::detail::ops_matmul(kDtF32, da, db, dc, m, k, n);
    if (is_kernel_image_missing(err)) {
        cleanup();
        result.skipped = true;
        return result;
    }

    EXPECT_EQ(err, 0) << "ops_matmul err=" << err;
    if (err != 0) {
        cleanup();
        return result;
    }

    // 拷贝结果
    result.value.resize(c_size);
    EXPECT_EQ(cudaMemcpy(result.value.data(), dc, c_size * sizeof(T), cudaMemcpyDeviceToHost), cudaSuccess);

    cleanup();
    result.ok = true;
    return result;
}

} // namespace

// 小矩阵乘法：A[2,3] x B[3,2] = C[2,2]
TEST(CudaMatmulTests, F32SmallMatrix)
{
    // A = [1 2 3; 4 5 6]，行主序
    std::vector<float> a{1, 2, 3, 4, 5, 6};
    // B = [7 8; 9 10; 11 12]，行主序
    std::vector<float> b{7, 8, 9, 10, 11, 12};
    // C = [1*7+2*9+3*11, 1*8+2*10+3*12; 4*7+5*9+6*11, 4*8+5*10+6*12]
    //   = [58, 64; 139, 154]

    // matmul: C[M,N] = A[M,K] * B[K,N]，M=2, K=3, N=2
    const auto result = run_matmul_f32<float>(2, 3, 2, a, b);
    if (result.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";

    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.value.size(), 4u);
    EXPECT_FLOAT_EQ(result.value[0], 58.0f);
    EXPECT_FLOAT_EQ(result.value[1], 64.0f);
    EXPECT_FLOAT_EQ(result.value[2], 139.0f);
    EXPECT_FLOAT_EQ(result.value[3], 154.0f);
}

// 非 4 对齐矩阵乘法：M=5, K=3, N=7，期望触发 baseline/CUTLASS fallback 并正确计算
TEST(CudaMatmulTests, F32NonMultipleOfFourFallsBackAndComputes)
{
    constexpr std::size_t m = 5, k = 3, n = 7;
    std::vector<float> a(m * k, 1.0f); // A 全 1
    std::vector<float> b(k * n, 2.0f); // B 全 2

    // 每个输出元素 = sum(1 * 2 for 3 次) = 6
    const auto result = run_matmul_f32<float>(m, k, n, a, b);
    if (result.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";

    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.value.size(), m * n);
    for (std::size_t i = 0; i < result.value.size(); ++i)
        EXPECT_FLOAT_EQ(result.value[i], 6.0f) << "index=" << i;
}
