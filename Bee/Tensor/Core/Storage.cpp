#include "Tensor/Core/Storage.hpp"
#include "Base/Memory/Allocator.hpp"

namespace bee
{

Storage::Storage(void* data, std::size_t nbytes, std::size_t alignment, Device device,
                 IAllocator* allocator) noexcept
    : data_(data), nbytes_(nbytes), alignment_(alignment), device_(device), allocator_(allocator)
{
}

Storage::~Storage()
{
    if (data_ != nullptr)
        allocator_->deallocate(data_, nbytes_, alignment_);
}

auto Storage::allocate(std::size_t nbytes, IAllocator& allocator) -> Result<std::shared_ptr<Storage>>
{
    auto result = allocator.allocate(nbytes, 64u);
    if (!result)
        return std::unexpected(std::move(result.error()));

    // Storage 构造函数为私有，无法使用 make_shared，直接用 new
    return std::shared_ptr<Storage>(
        new Storage(result.value(), nbytes, 64u, allocator.device(), &allocator));
}

auto Storage::data() noexcept -> void*
{
    return data_;
}

auto Storage::data() const noexcept -> const void*
{
    return data_;
}

auto Storage::nbytes() const noexcept -> std::size_t
{
    return nbytes_;
}

auto Storage::device() const noexcept -> Device
{
    return device_;
}

auto Storage::allocator() const noexcept -> IAllocator&
{
    return *allocator_;
}

} // namespace bee
