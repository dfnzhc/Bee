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

TEST(CudaRuntimeTests, StorageTracksMemoryKind)
{
    // CPU storage 应该是 Host
    auto cpu = bee::Tensor::zeros({4}, bee::DType::F32, bee::Device::CPU).value();
    EXPECT_EQ(cpu.storage()->memory_kind(), bee::MemoryKind::Host);
    EXPECT_FALSE(cpu.storage()->is_pinned());

    // 无 CUDA 时显式跳过 HostPinned 相关断言
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA 不可用，跳过 HostPinned 测试";

    // 有 CUDA 时 HostPinned 分配必须成功，并正确标记 memory_kind / is_pinned
    auto& cpu_allocator = bee::CpuAllocator::instance();
    auto pinned = bee::Storage::allocate(256, cpu_allocator, bee::MemoryKind::HostPinned);
    ASSERT_TRUE(pinned.has_value()) << "HostPinned 分配应成功";
    EXPECT_EQ(pinned.value()->memory_kind(), bee::MemoryKind::HostPinned);
    EXPECT_TRUE(pinned.value()->is_pinned());
}

// 验证 Storage::allocate 对 Workspace 类型应显式拒绝
TEST(CudaRuntimeTests, StorageAllocateWorkspaceIsRejected)
{
    // Workspace 属于 runtime-owned 语义，不允许通过 Storage::allocate 创建；
    // 调用方应直接使用 tensor::cuda::request_workspace()。
    auto& allocator = bee::CpuAllocator::instance();
    auto result     = bee::Storage::allocate(256, allocator, bee::MemoryKind::Workspace);
    EXPECT_FALSE(result.has_value()) << "Storage::allocate(Workspace) 应返回错误";
}

TEST(CudaRuntimeTests, WorkspaceCanBeRequestedFromBackend)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    // 第一次请求：应返回非空指针
    void* ws1 = bee::tensor::cuda::request_workspace(4096, nullptr).value();
    EXPECT_NE(ws1, nullptr);

    // 较小请求应复用同一块（同一 device，capacity 足够时直接复用）
    void* ws2 = bee::tensor::cuda::request_workspace(2048, nullptr).value();
    EXPECT_NE(ws2, nullptr);
    EXPECT_EQ(ws1, ws2) << "容量足够时，较小请求应复用已有 workspace 指针";

    // 请求更大尺寸：runtime 重新分配更大块，ws1/ws2 旧指针随即失效
    // 契约：不可在增长边界之后继续使用旧指针
    void* ws3 = bee::tensor::cuda::request_workspace(8192, nullptr).value();
    EXPECT_NE(ws3, nullptr);

    // runtime-owned workspace 契约：
    // - 调用方无需也不应 free
    // - 同一 device 的后续请求容量足够时复用旧块；触发扩容时旧指针失效
}
