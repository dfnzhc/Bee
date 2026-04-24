#pragma once

#include "Base/Memory/Allocator.hpp"
#include "Base/Memory/Device.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

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

inline auto CudaAllocator::allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>
{
    // 转发至 Tensor/CUDA 桥接层，再由其决定进入真实 CUDA 后端或返回可恢复错误。
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

} // namespace bee
