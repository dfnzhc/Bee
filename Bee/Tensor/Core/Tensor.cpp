#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Core/Allocator.hpp"
#include "Tensor/Core/Storage.hpp"

#include <cstring>
#include <format>

namespace bee
{

Tensor::Tensor(std::shared_ptr<TensorImpl> impl)
    : impl_(std::move(impl))
{
}

auto Tensor::empty(Shape shape, DType dtype, Device device) -> Result<Tensor>
{
    if (device == Device::CUDA)
        return std::unexpected(make_error("CUDA backend not available", Severity::Recoverable));

    // 校验 shape：负维度会导致 size_t 溢出，属未定义行为
    for (auto d : shape) {
        if (d < 0) {
            return std::unexpected(make_error(
                std::format("Tensor::empty: 非法的负维度 {} in shape", d), Severity::Recoverable));
        }
    }

    const int64_t     n      = ::bee::numel(shape);
    const std::size_t nbytes = static_cast<std::size_t>(n) * dtype_size(dtype);

    auto storage_result = Storage::allocate(nbytes, CpuAllocator::instance());
    if (!storage_result)
        return std::unexpected(std::move(storage_result.error()));

    auto ti      = std::make_shared<TensorImpl>();
    ti->storage  = std::move(*storage_result);
    ti->dtype    = dtype;
    ti->strides  = compute_contiguous_strides(shape);
    ti->shape    = std::move(shape);
    ti->offset   = 0;

    return Tensor(std::move(ti));
}

auto Tensor::defined() const noexcept -> bool
{
    return impl_ != nullptr;
}

auto Tensor::ndim() const noexcept -> int64_t
{
    return static_cast<int64_t>(impl_->shape.size());
}

auto Tensor::numel() const noexcept -> int64_t
{
    return impl_->numel();
}

auto Tensor::shape() const noexcept -> const Shape&
{
    return impl_->shape;
}

auto Tensor::strides() const noexcept -> const Strides&
{
    return impl_->strides;
}

auto Tensor::dtype() const noexcept -> DType
{
    return impl_->dtype;
}

auto Tensor::device() const noexcept -> Device
{
    return impl_->storage->device();
}

auto Tensor::is_contiguous() const noexcept -> bool
{
    return ::bee::is_contiguous(impl_->shape, impl_->strides);
}

auto Tensor::storage_offset() const noexcept -> int64_t
{
    return impl_->offset;
}

auto Tensor::data_ptr() noexcept -> void*
{
    auto* base = static_cast<uint8_t*>(impl_->storage->data());
    return base + impl_->offset * static_cast<int64_t>(dtype_size(impl_->dtype));
}

auto Tensor::data_ptr() const noexcept -> const void*
{
    const auto* base = static_cast<const uint8_t*>(impl_->storage->data());
    return base + impl_->offset * static_cast<int64_t>(dtype_size(impl_->dtype));
}

auto Tensor::storage() const noexcept -> const std::shared_ptr<Storage>&
{
    return impl_->storage;
}

auto Tensor::impl() const noexcept -> const std::shared_ptr<TensorImpl>&
{
    return impl_;
}

auto Tensor::clone() const -> Result<Tensor>
{
    if (!defined())
        return std::unexpected(make_error("不能克隆未定义的 Tensor", Severity::Recoverable));

    if (!is_contiguous())
        return std::unexpected(make_error("非连续 Tensor 的 clone 尚未实现", Severity::Recoverable));

    if (device() == Device::CUDA)
        return std::unexpected(make_error("CUDA clone 尚未实现", Severity::Recoverable));

    // 防御性检查：impl 的 shape 不应含有负维度
    for (auto d : impl_->shape) {
        if (d < 0) {
            return std::unexpected(make_error(
                std::format("Tensor::clone: 非法的负维度 {} in shape", d), Severity::Recoverable));
        }
    }

    const auto nbytes =
        static_cast<std::size_t>(impl_->numel()) * dtype_size(impl_->dtype);

    auto storage_result = Storage::allocate(nbytes, CpuAllocator::instance());
    if (!storage_result)
        return std::unexpected(std::move(storage_result.error()));

    std::memcpy((*storage_result)->data(), data_ptr(), nbytes);

    auto ti     = std::make_shared<TensorImpl>();
    ti->storage = std::move(*storage_result);
    ti->dtype   = impl_->dtype;
    ti->shape   = impl_->shape;
    ti->strides = impl_->strides;
    ti->offset  = 0;

    return Tensor(std::move(ti));
}

} // namespace bee
