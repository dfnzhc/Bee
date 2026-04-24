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

    // 显式创建 HostPinned storage
    auto& cpu_allocator = bee::CpuAllocator::instance();
    auto pinned = bee::Storage::allocate(256, cpu_allocator, bee::MemoryKind::HostPinned);
    if (pinned) {
        EXPECT_EQ(pinned.value()->memory_kind(), bee::MemoryKind::HostPinned);
        EXPECT_TRUE(pinned.value()->is_pinned());
    }
}

TEST(CudaRuntimeTests, WorkspaceCanBeRequestedFromBackend)
{
    if (!bee::tensor::cuda::is_available())
        GTEST_SKIP() << "CUDA unavailable";

    // 第一次请求：应返回非空指针
    void* ws1 = bee::tensor::cuda::request_workspace(4096, nullptr).value();
    EXPECT_NE(ws1, nullptr);

    // 第二次请求相同或更小尺寸：应返回可复用的指针（可能是同一个）
    void* ws2 = bee::tensor::cuda::request_workspace(2048, nullptr).value();
    EXPECT_NE(ws2, nullptr);

    // 请求更大尺寸：runtime 应重新分配更大块
    void* ws3 = bee::tensor::cuda::request_workspace(8192, nullptr).value();
    EXPECT_NE(ws3, nullptr);

    // 所有返回的指针均由 runtime 持有，调用方无需 free
}
