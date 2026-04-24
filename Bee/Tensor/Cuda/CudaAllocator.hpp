#pragma once

#include "Base/Memory/Allocator.hpp"
#include "Base/Memory/Device.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

// CUDA 分配类型
enum class CudaAllocKind : std::uint8_t
{
    Device,    // 常规 CUDA 设备内存
    Workspace, // 临时工作区（当前阶段直接用 cudaMalloc，后续可池化）
};

// CUDA 内存分配器：实现 IAllocator 接口，并转发到 tensor::cuda 后端桥接层。
// 当 Tensor 打开 CUDA 支持时，该桥接层会继续转发到 Bee::CUDA；
// 未打开时，allocate 返回可恢复错误，deallocate 保持 no-op。
class CudaAllocator final : public IAllocator
{
public:
    // 线程安全单例（Meyer 模式）
    [[nodiscard]] static auto instance() noexcept -> CudaAllocator&;

    [[nodiscard]] auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*> override;
    auto               deallocate(void* p, std::size_t nbytes, std::size_t alignment) noexcept -> void override;

    // 特定分配类型入口（供需要显式控制 workspace 等语义的代码调用）
    [[nodiscard]] auto allocate_for(CudaAllocKind kind, std::size_t nbytes, std::size_t alignment) -> Result<void*>;

    [[nodiscard]] auto device() const noexcept -> Device override;

    // 统计峰值字节数（当前骨架阶段返回 0）
    [[nodiscard]] auto peak_bytes() const noexcept -> std::size_t;

private:
    CudaAllocator() = default;
};

// ── 内联实现 ─────────────────────────────────────────────────────────────────

inline auto CudaAllocator::instance() noexcept -> CudaAllocator&
{
    static CudaAllocator inst;
    return inst;
}

inline auto CudaAllocator::allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>
{
    // 默认走常规 Device 分配
    return allocate_for(CudaAllocKind::Device, nbytes, alignment);
}

inline auto CudaAllocator::allocate_for(CudaAllocKind kind, std::size_t nbytes, std::size_t alignment) -> Result<void*>
{
    // 转发至 Tensor/CUDA 桥接层，再由其决定进入真实 CUDA 后端或返回可恢复错误。
    // 当前 workspace 与 device 统一走 tensor::cuda::allocate；后续可分流。
    (void)kind; // suppress unused warning
    return tensor::cuda::allocate(nbytes, alignment);
}

inline auto CudaAllocator::deallocate(void* p, std::size_t nbytes, std::size_t alignment) noexcept -> void
{
    // 转发至 Tensor/CUDA 桥接层；在无 CUDA 后端时保持 no-op。
    tensor::cuda::deallocate(p, nbytes, alignment);
}

inline auto CudaAllocator::device() const noexcept -> Device
{
    return Device::CUDA;
}

inline auto CudaAllocator::peak_bytes() const noexcept -> std::size_t
{
    // 骨架阶段，尚未实现统计
    return 0;
}

} // namespace bee
