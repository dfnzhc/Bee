/**
 * @File Tests/CUDA/StridedCopyTests.cu
 * @Brief CUDA strided_copy 算子的正确性测试，覆盖转置视图与维度上限检查。
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

// strided_copy 测试结果封装
template <typename T>
struct StridedCopyResult
{
    bool             skipped = false; // kernel image 缺失时跳过
    bool             ok      = false; // 计算成功
    std::vector<T>   value{};         // 输出数据
    int              err     = 0;     // 错误码（用于检查维度限制等）
};

// 集中资源管理的 strided_copy 执行辅助函数
template <typename T>
auto run_strided_copy_f32(const std::vector<T>& src,
                          const std::int64_t* shape,
                          const std::int64_t* strides,
                          int ndim,
                          std::int64_t offset,
                          std::int64_t numel) -> StridedCopyResult<T>
{
    T* dsrc = nullptr;
    T* ddst = nullptr;
    StridedCopyResult<T> result{};

    // 清理资源的 lambda，确保所有返回路径都释放
    auto cleanup = [&]() {
        if (ddst) (void)cudaFree(ddst);
        if (dsrc) (void)cudaFree(dsrc);
    };

    // 分配源
    cudaError_t cu_err = cudaMalloc(&dsrc, src.size() * sizeof(T));
    EXPECT_EQ(cu_err, cudaSuccess);
    if (cu_err != cudaSuccess) {
        cleanup();
        return result;
    }

    // 分配目标
    cu_err = cudaMalloc(&ddst, numel * sizeof(T));
    EXPECT_EQ(cu_err, cudaSuccess);
    if (cu_err != cudaSuccess) {
        cleanup();
        return result;
    }

    // 拷贝输入
    cu_err = cudaMemcpy(dsrc, src.data(), src.size() * sizeof(T), cudaMemcpyHostToDevice);
    EXPECT_EQ(cu_err, cudaSuccess);
    if (cu_err != cudaSuccess) {
        cleanup();
        return result;
    }

    // 调用 strided_copy
    const int err = bee::cuda::detail::ops_strided_copy(kDtF32, dsrc, ddst, shape, strides, ndim, offset, numel);
    result.err = err;

    if (is_kernel_image_missing(err)) {
        cleanup();
        result.skipped = true;
        return result;
    }

    if (err != 0) {
        cleanup();
        return result;
    }

    // 拷贝结果
    result.value.resize(numel);
    cu_err = cudaMemcpy(result.value.data(), ddst, numel * sizeof(T), cudaMemcpyDeviceToHost);
    EXPECT_EQ(cu_err, cudaSuccess);
    if (cu_err != cudaSuccess) {
        cleanup();
        return result;
    }

    cleanup();
    result.ok = true;
    return result;
}

} // namespace

// 拷贝转置视图：src 为列主序，shape {3,2}, 拷贝为行主序
TEST(CudaStridedCopyTests, CopiesTransposedView)
{
    // src 数据：[1, 2, 3, 4, 5, 6]，视为列主序 3x2 矩阵（3 行 2 列）
    // 列主序 strides {1, 3}：shape {3, 2}，numel=6
    // 期望输出行主序：[1, 4, 2, 5, 3, 6]（转置后）
    std::vector<float> src{1, 2, 3, 4, 5, 6};

    const std::int64_t shape[2]   = {3, 2};
    const std::int64_t strides[2] = {1, 3}; // 列主序

    // strided_copy：从 src storage 按 shape/strides 拷贝到连续 dst
    const auto result = run_strided_copy_f32<float>(src, shape, strides, 2, 0, 6);
    if (result.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";

    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.value.size(), 6u);

    // 验证转置结果
    EXPECT_FLOAT_EQ(result.value[0], 1.0f);
    EXPECT_FLOAT_EQ(result.value[1], 4.0f);
    EXPECT_FLOAT_EQ(result.value[2], 2.0f);
    EXPECT_FLOAT_EQ(result.value[3], 5.0f);
    EXPECT_FLOAT_EQ(result.value[4], 3.0f);
    EXPECT_FLOAT_EQ(result.value[5], 6.0f);
}

// 拒绝超过 8 维的 strided_copy
TEST(CudaStridedCopyTests, RejectsMoreThanEightDimensions)
{
    std::vector<float> src(8, 1.0f);

    // 构造 9 维 shape/strides（虽然 numel 只有 8）
    const std::int64_t shape[9]   = {2, 1, 1, 1, 1, 1, 1, 1, 4};
    const std::int64_t strides[9] = {4, 4, 4, 4, 4, 4, 4, 4, 1};

    // ndim=9 应返回错误
    const auto result = run_strided_copy_f32<float>(src, shape, strides, 9, 0, 8);
    if (result.skipped)
        GTEST_SKIP() << "No kernel image for current GPU";

    EXPECT_NE(result.err, 0) << "strided_copy 应拒绝 ndim=9";
}
