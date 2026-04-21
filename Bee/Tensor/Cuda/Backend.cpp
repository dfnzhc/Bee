#include "Tensor/Cuda/Backend.hpp"

namespace bee::tensor::cuda
{

// CUDA 后端 stub 实现
// 所有函数均返回 NotImplemented 错误。
// 将来由 Bee::CUDA 组件提供真正的 CUDA 实现。

auto allocate(std::size_t /*nbytes*/, std::size_t /*alignment*/) -> Result<void*>
{
    return std::unexpected(make_error("CUDA 后端不可用：allocate", Severity::Recoverable));
}

auto deallocate(void* /*p*/, std::size_t /*nbytes*/, std::size_t /*alignment*/) -> void
{
    // Stub 实现：不做任何操作
}

auto memcpy_h2d(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/)
    -> Result<void>
{
    return std::unexpected(
        make_error("CUDA 后端不可用：memcpy_h2d", Severity::Recoverable));
}

auto memcpy_d2h(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/)
    -> Result<void>
{
    return std::unexpected(
        make_error("CUDA 后端不可用：memcpy_d2h", Severity::Recoverable));
}

auto memcpy_d2d(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/)
    -> Result<void>
{
    return std::unexpected(
        make_error("CUDA 后端不可用：memcpy_d2d", Severity::Recoverable));
}

auto synchronize() -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：synchronize", Severity::Recoverable));
}

} // namespace bee::tensor::cuda
