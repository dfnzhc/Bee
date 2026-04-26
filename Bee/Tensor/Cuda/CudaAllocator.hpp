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
    // 根据 kind 实现真实分支语义：
    // - Device: 常规设备内存，经由池化分配器
    // - Workspace: runtime-owned 工作区，不适用于常规 deallocate 生命周期
    switch (kind) {
    case CudaAllocKind::Device: return tensor::cuda::allocate(nbytes, alignment);
    case CudaAllocKind::Workspace:
        // workspace 由 runtime 持有，调用方无需 free。
        // 注意：此路径返回的指针不应传递给 CudaAllocator::deallocate。
        return tensor::cuda::request_workspace(nbytes, nullptr);
    default: return tensor::cuda::allocate(nbytes, alignment);
    }
}

inline auto CudaAllocator::deallocate(void* p, std::size_t nbytes, std::size_t alignment) noexcept -> void
{
    // 注意：通过 allocate_for(CudaAllocKind::Workspace, ...) 获取的指针由 runtime 持有，
    // 不应传入此函数释放，否则会导致 double-free。
    // Storage::allocate 会对 MemoryKind::Workspace 显式拒绝，以阻断该误用路径。
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
