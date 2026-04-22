#include "Tensor/Cuda/Backend.hpp"

#if defined(BEE_TENSOR_WITH_CUDA)
#include "CUDA/Api.hpp"
#endif

namespace bee::tensor::cuda
{

#if defined(BEE_TENSOR_WITH_CUDA)

// BEE_TENSOR_WITH_CUDA=ON：转发到 Bee::CUDA 组件的 C++ API。
// 这里保持 host-only，绝不引入 cuda_runtime.h 或 *.cuh 头。

auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>
{
    return ::bee::cuda::allocate(nbytes, alignment);
}

auto deallocate(void* p, std::size_t nbytes, std::size_t alignment) -> void
{
    ::bee::cuda::deallocate(p, nbytes, alignment);
}

auto memcpy_h2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return ::bee::cuda::memcpy_h2d(dst, src, nbytes);
}

auto memcpy_d2h(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return ::bee::cuda::memcpy_d2h(dst, src, nbytes);
}

auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return ::bee::cuda::memcpy_d2d(dst, src, nbytes);
}

auto memset(void* ptr, int value, std::size_t nbytes) -> Result<void>
{
    return ::bee::cuda::memset(ptr, value, nbytes);
}

auto synchronize() -> Result<void>
{
    return ::bee::cuda::device_synchronize();
}

#else // BEE_TENSOR_WITH_CUDA

// CUDA 后端未启用时的 stub 实现。
// 所有函数均返回 Recoverable 错误；Tensor 层会把它原样传播给调用方。

auto allocate(std::size_t /*nbytes*/, std::size_t /*alignment*/) -> Result<void*>
{
    return std::unexpected(make_error("CUDA 后端不可用：allocate", Severity::Recoverable));
}

auto deallocate(void* /*p*/, std::size_t /*nbytes*/, std::size_t /*alignment*/) -> void
{
    // no-op
}

auto memcpy_h2d(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：memcpy_h2d", Severity::Recoverable));
}

auto memcpy_d2h(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：memcpy_d2h", Severity::Recoverable));
}

auto memcpy_d2d(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：memcpy_d2d", Severity::Recoverable));
}

auto memset(void* /*ptr*/, int /*value*/, std::size_t /*nbytes*/) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：memset", Severity::Recoverable));
}

auto synchronize() -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：synchronize", Severity::Recoverable));
}

#endif // BEE_TENSOR_WITH_CUDA

} // namespace bee::tensor::cuda
