#pragma once

#include <cstddef>
#include <memory>

#include "Base/Diagnostics/Error.hpp"
#include "Base/Memory/Device.hpp"

namespace bee
{

class IAllocator;

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
    [[nodiscard]] static auto allocate(std::size_t nbytes, IAllocator& allocator)
        -> Result<std::shared_ptr<Storage>>;

    [[nodiscard]] auto data() noexcept -> void*;
    [[nodiscard]] auto data() const noexcept -> const void*;
    [[nodiscard]] auto nbytes() const noexcept -> std::size_t;
    [[nodiscard]] auto device() const noexcept -> Device;
    [[nodiscard]] auto allocator() const noexcept -> IAllocator&;

private:
    explicit Storage(void* data, std::size_t nbytes, std::size_t alignment, Device device,
                     IAllocator* allocator) noexcept;

    void*       data_      = nullptr;
    std::size_t nbytes_    = 0;
    std::size_t alignment_ = 64;   // 分配时实际使用的对齐值，析构时传回 deallocate
    Device      device_    = Device::CPU;
    IAllocator* allocator_ = nullptr;
};

} // namespace bee
