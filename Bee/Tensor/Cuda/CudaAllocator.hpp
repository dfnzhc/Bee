#pragma once

#include "Tensor/Core/Allocator.hpp"
#include "Tensor/Core/Device.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

// CUDA 内存分配器：实现 IAllocator 接口，转发至 CUDA 后端。
// 当前实现通过 cuda::allocate/cuda::deallocate 转发，
// 这些函数返回 NotImplemented；将来 Bee::CUDA 组件实装时自动获得真实功能。
class CudaAllocator final : public IAllocator
{
public:
    // 线程安全单例（Meyer 模式）
    [[nodiscard]] static auto instance() noexcept -> CudaAllocator&;

    [[nodiscard]] auto allocate(std::size_t nbytes, std::size_t alignment)
        -> Result<void*> override;
    auto deallocate(void* p, std::size_t nbytes, std::size_t alignment) noexcept
        -> void override;

    [[nodiscard]] auto device() const noexcept -> Device override;

private:
    CudaAllocator() = default;
};

// ── 内联实现 ─────────────────────────────────────────────────────────────────

inline auto CudaAllocator::instance() noexcept -> CudaAllocator&
{
    static CudaAllocator inst;
    return inst;
}

inline auto CudaAllocator::allocate(std::size_t nbytes, std::size_t alignment)
    -> Result<void*>
{
    // 转发至 CUDA 后端接口（当前返回 NotImplemented）
    return tensor::cuda::allocate(nbytes, alignment);
}

inline auto CudaAllocator::deallocate(void* p, std::size_t nbytes, std::size_t alignment) noexcept
    -> void
{
    // 转发至 CUDA 后端接口（当前为 noop）
    tensor::cuda::deallocate(p, nbytes, alignment);
}

inline auto CudaAllocator::device() const noexcept -> Device
{
    return Device::CUDA;
}

} // namespace bee
