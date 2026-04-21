#pragma once

#include <cstddef>
#include <new>

#include "Base/Diagnostics/Error.hpp"
#include "Base/Memory/Device.hpp"

namespace bee
{

// 分配器抽象接口
class IAllocator
{
public:
    virtual ~IAllocator() = default;

    [[nodiscard]] virtual auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*> = 0;
    virtual auto deallocate(void* p, std::size_t nbytes, std::size_t alignment) noexcept -> void   = 0;

    [[nodiscard]] virtual auto device() const noexcept -> Device = 0;
};

// CPU 分配器：以 64 字节对齐分配内存（SIMD 友好），线程安全单例
class CpuAllocator final : public IAllocator
{
public:
    [[nodiscard]] static auto instance() noexcept -> CpuAllocator&;

    [[nodiscard]] auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*> override;
    auto deallocate(void* p, std::size_t nbytes, std::size_t alignment) noexcept -> void override;

    [[nodiscard]] auto device() const noexcept -> Device override;
};

// Meyer's singleton：首次调用时构造，线程安全（C++11 及以后）
inline auto CpuAllocator::instance() noexcept -> CpuAllocator&
{
    static CpuAllocator inst;
    return inst;
}

inline auto CpuAllocator::allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>
{
    // 统一使用至少 64 字节对齐，以保证 SIMD 友好
    const std::size_t actual = alignment < 64u ? 64u : alignment;
    try {
        void* p = ::operator new(nbytes, std::align_val_t{actual});
        return p;
    } catch (const std::bad_alloc&) {
        return std::unexpected(make_error("内存分配失败", Severity::Recoverable));
    }
}

inline auto CpuAllocator::deallocate(void* p, std::size_t /*nbytes*/, std::size_t alignment) noexcept -> void
{
    // 与 allocate 保持一致：至少 64 字节对齐
    const std::size_t actual = alignment < 64u ? std::size_t{64} : alignment;
    if (p != nullptr)
        ::operator delete(p, std::align_val_t{actual});
}

inline auto CpuAllocator::device() const noexcept -> Device
{
    return Device::CPU;
}

} // namespace bee
