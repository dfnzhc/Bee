#include "Tensor/Cuda/Backend.hpp"

#if defined(BEE_TENSOR_WITH_CUDA)
    #include "CUDA/Api.hpp"
#endif

#include <cstdint>

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

auto ew_binary(int op, int dt, const void* a, const void* b, void* out, std::size_t n) -> Result<void>
{
    return ::bee::cuda::ops::binary(static_cast<::bee::cuda::BinaryOp>(op), static_cast<::bee::cuda::ScalarType>(dt), a, b, out, n);
}

auto ew_unary(int op, int dt, const void* src, void* dst, std::size_t n) -> Result<void>
{
    return ::bee::cuda::ops::unary(static_cast<::bee::cuda::UnaryOp>(op), static_cast<::bee::cuda::ScalarType>(dt), src, dst, n);
}

auto ew_cast(int src_dt, const void* src, int dst_dt, void* dst, std::size_t n) -> Result<void>
{
    return ::bee::cuda::ops::cast(static_cast<::bee::cuda::ScalarType>(src_dt), src, static_cast<::bee::cuda::ScalarType>(dst_dt), dst, n);
}

auto reduce_global(int op, int dt, const void* src, void* dst, std::size_t n) -> Result<void>
{
    return ::bee::cuda::ops::reduce_global(static_cast<::bee::cuda::ReduceOp>(op), static_cast<::bee::cuda::ScalarType>(dt), src, dst, n);
}

auto reduce_axis(int op, int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) -> Result<void>
{
    return ::bee::cuda::ops::reduce_axis(
        static_cast<::bee::cuda::ReduceOp>(op), static_cast<::bee::cuda::ScalarType>(dt), src, dst, outer, axis, inner
    );
}

auto softmax(int dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) -> Result<void>
{
    return ::bee::cuda::ops::softmax(static_cast<::bee::cuda::ScalarType>(dt), src, dst, outer, axis, inner);
}

auto scale_fp(int dt, void* buf, double factor, std::size_t n) -> Result<void>
{
    return ::bee::cuda::ops::scale_fp(static_cast<::bee::cuda::ScalarType>(dt), buf, factor, n);
}

auto matmul(int dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) -> Result<void>
{
    return ::bee::cuda::ops::matmul(static_cast<::bee::cuda::ScalarType>(dt), A, B, C, M, K, N);
}

auto matmul_lowp(int dt, const void* A, const void* B, float* C, std::size_t M, std::size_t K, std::size_t N) -> Result<void>
{
    return ::bee::cuda::ops::matmul_lowp(static_cast<::bee::cuda::ScalarType>(dt), A, B, C, M, K, N);
}

auto transpose_2d(int dt, const void* src, void* dst, std::size_t rows, std::size_t cols) -> Result<void>
{
    return ::bee::cuda::ops::transpose_2d(static_cast<::bee::cuda::ScalarType>(dt), src, dst, rows, cols);
}

auto strided_copy(
    int            dt,
    const void*    src,
    void*          dst,
    const int64_t* shape,
    const int64_t* strides,
    int            ndim,
    int64_t        offset_elements,
    std::size_t    numel
) -> Result<void>
{
    return ::bee::cuda::ops::strided_copy(static_cast<::bee::cuda::ScalarType>(dt), src, dst, shape, strides, ndim, offset_elements, numel);
}

auto random_uniform(int dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>
{
    return ::bee::cuda::ops::random_uniform(static_cast<::bee::cuda::ScalarType>(dt), dst, n, seed);
}

auto random_normal(int dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>
{
    return ::bee::cuda::ops::random_normal(static_cast<::bee::cuda::ScalarType>(dt), dst, n, seed);
}

auto random_int(int dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) -> Result<void>
{
    return ::bee::cuda::ops::random_int(static_cast<::bee::cuda::ScalarType>(dt), dst, n, low, high, seed);
}

auto rms_norm(int dt, const void* x, const void* w, void* out,
              std::size_t rows, std::size_t dim, double eps) -> Result<void>
{
    return ::bee::cuda::ops::rms_norm(static_cast<::bee::cuda::ScalarType>(dt), x, w, out, rows, dim, eps);
}

auto rope(int dt, const void* x, void* out,
          std::size_t n_batch, std::size_t seq_len, std::size_t dim,
          double base, std::int64_t position_offset) -> Result<void>
{
    return ::bee::cuda::ops::rope(static_cast<::bee::cuda::ScalarType>(dt), x, out, n_batch, seq_len, dim, base, position_offset);
}

auto embedding(int weight_dt, int ids_dt,
               const void* weight, const void* ids, void* out,
               std::size_t n_ids, std::size_t hidden, std::size_t vocab) -> Result<void>
{
    return ::bee::cuda::ops::embedding(
        static_cast<::bee::cuda::ScalarType>(weight_dt), static_cast<::bee::cuda::ScalarType>(ids_dt),
        weight, ids, out, n_ids, hidden, vocab
    );
}

auto synchronize() -> Result<void>
{
    return ::bee::cuda::device_synchronize();
}

auto is_available() noexcept -> bool
{
    return true;
}

auto memcpy_h2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>
{
    return ::bee::cuda::memcpy_h2d_async(dst, src, nbytes, stream);
}

auto memcpy_d2h_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>
{
    return ::bee::cuda::memcpy_d2h_async(dst, src, nbytes, stream);
}

auto memcpy_d2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>
{
    return ::bee::cuda::memcpy_d2d_async(dst, src, nbytes, stream);
}

auto create_event() -> Result<void*>
{
    return ::bee::cuda::create_event();
}

auto record_event(void* event_handle, void* stream) -> Result<void>
{
    return ::bee::cuda::record_event(event_handle, stream);
}

auto wait_event(void* event_handle, void* stream) -> Result<void>
{
    return ::bee::cuda::wait_event(event_handle, stream);
}

auto request_workspace(std::size_t nbytes, void* stream) -> Result<void*>
{
    return ::bee::cuda::request_workspace(nbytes, stream);
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

auto ew_binary(int /*op*/, int /*dt*/, const void*, const void*, void*, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：ew_binary", Severity::Recoverable));
}

auto ew_unary(int /*op*/, int /*dt*/, const void*, void*, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：ew_unary", Severity::Recoverable));
}

auto ew_cast(int /*src_dt*/, const void*, int /*dst_dt*/, void*, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：ew_cast", Severity::Recoverable));
}

auto reduce_global(int, int, const void*, void*, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：reduce_global", Severity::Recoverable));
}

auto reduce_axis(int, int, const void*, void*, std::size_t, std::size_t, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：reduce_axis", Severity::Recoverable));
}

auto softmax(int, const void*, void*, std::size_t, std::size_t, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：softmax", Severity::Recoverable));
}

auto scale_fp(int, void*, double, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：scale_fp", Severity::Recoverable));
}

auto matmul(int, const void*, const void*, void*, std::size_t, std::size_t, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：matmul", Severity::Recoverable));
}

auto matmul_lowp(int, const void*, const void*, float*, std::size_t, std::size_t, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：matmul_lowp", Severity::Recoverable));
}

auto transpose_2d(int, const void*, void*, std::size_t, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：transpose_2d", Severity::Recoverable));
}

auto strided_copy(int, const void*, void*, const int64_t*, const int64_t*, int, int64_t, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：strided_copy", Severity::Recoverable));
}

auto random_uniform(int, void*, std::size_t, std::uint64_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：random_uniform", Severity::Recoverable));
}

auto random_normal(int, void*, std::size_t, std::uint64_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：random_normal", Severity::Recoverable));
}

auto random_int(int, void*, std::size_t, std::int64_t, std::int64_t, std::uint64_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：random_int", Severity::Recoverable));
}

auto rms_norm(int, const void*, const void*, void*, std::size_t, std::size_t, double) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：rms_norm", Severity::Recoverable));
}

auto rope(int, const void*, void*, std::size_t, std::size_t, std::size_t, double, std::int64_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：rope", Severity::Recoverable));
}

auto embedding(int, int, const void*, const void*, void*, std::size_t, std::size_t, std::size_t) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：embedding", Severity::Recoverable));
}

auto synchronize() -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：synchronize", Severity::Recoverable));
}

auto is_available() noexcept -> bool
{
    return false;
}

auto memcpy_h2d_async(void*, const void*, std::size_t, void*) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：memcpy_h2d_async", Severity::Recoverable));
}

auto memcpy_d2h_async(void*, const void*, std::size_t, void*) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：memcpy_d2h_async", Severity::Recoverable));
}

auto memcpy_d2d_async(void*, const void*, std::size_t, void*) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：memcpy_d2d_async", Severity::Recoverable));
}

auto create_event() -> Result<void*>
{
    return std::unexpected(make_error("CUDA 后端不可用：create_event", Severity::Recoverable));
}

auto record_event(void*, void*) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：record_event", Severity::Recoverable));
}

auto wait_event(void*, void*) -> Result<void>
{
    return std::unexpected(make_error("CUDA 后端不可用：wait_event", Severity::Recoverable));
}

auto request_workspace(std::size_t, void*) -> Result<void*>
{
    return std::unexpected(make_error("CUDA 后端不可用：request_workspace", Severity::Recoverable));
}

#endif // BEE_TENSOR_WITH_CUDA

} // namespace bee::tensor::cuda
