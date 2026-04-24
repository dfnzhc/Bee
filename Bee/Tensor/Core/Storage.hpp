#pragma once

#include <cstddef>
#include <memory>

#include "Base/Diagnostics/Error.hpp"
#include "Base/Memory/Device.hpp"

namespace bee
{

class IAllocator;

// Storage 内存类型归属
enum class MemoryKind : std::uint8_t
{
    Host,        // CPU 堆内存
    HostPinned,  // CUDA pinned host memory（页锁定内存）
    Device,      // CUDA 设备内存
    Workspace,   // 临时工作区（通常为设备内存，生命周期受运行时管理）
};

// Storage 持有一块裸内存及其分配器，通过 shared_ptr 共享所有权
// 禁止拷贝与移动，生命周期由 shared_ptr<Storage> 管控
class Storage
{
public:
    Storage(const Storage&)            = delete;
    Storage& operator=(const Storage&) = delete;
    Storage(Storage&&)                 = delete;
    Storage& operator=(Storage&&)      = delete;

    ~Storage();

    // 静态工厂：委托 allocator 分配内存，返回 shared_ptr<Storage>
    [[nodiscard]] static auto allocate(std::size_t nbytes, IAllocator& allocator) -> Result<std::shared_ptr<Storage>>;

    [[nodiscard]] auto data() noexcept -> void*;
    [[nodiscard]] auto data() const noexcept -> const void*;
    [[nodiscard]] auto nbytes() const noexcept -> std::size_t;
    [[nodiscard]] auto device() const noexcept -> Device;
    [[nodiscard]] auto allocator() const noexcept -> IAllocator&;
    [[nodiscard]] auto memory_kind() const noexcept -> MemoryKind;
    [[nodiscard]] auto is_pinned() const noexcept -> bool;

private:
    explicit Storage(void* data, std::size_t nbytes, std::size_t alignment, Device device, MemoryKind memory_kind, IAllocator* allocator) noexcept;

    void*       data_        = nullptr;
    std::size_t nbytes_      = 0;
    std::size_t alignment_   = 64; // 分配时实际使用的对齐值，析构时传回 deallocate
    Device      device_      = Device::CPU;
    MemoryKind  memory_kind_ = MemoryKind::Host;
    IAllocator* allocator_   = nullptr;
};

} // namespace bee
