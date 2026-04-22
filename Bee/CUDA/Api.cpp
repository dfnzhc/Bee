/**
 * @File Api.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA 组件对外 API 的 host 端实现。
 *
 * 本 TU 由 MSVC 以 C++23 编译，可自由使用 Result<T>/std::format。
 * 所有 CUDA runtime 调用通过 detail::runtime_* 经由 Api.cu 转发。
 */

#include "CUDA/Api.hpp"
#include "CUDA/Runtime.hpp"
#include "CUDA/Core/Check.hpp"
#include "CUDA/Core/Stream.hpp"
#include "CUDA/Mem/MemoryPool.hpp"

#include <cuda_runtime.h>

#include <format>

namespace bee::cuda
{

// ── 组件元信息 ──────────────────────────────────────────────────────────────

auto component_name() noexcept -> std::string_view
{
    return "CUDA";
}

auto toolkit_version() noexcept -> std::string_view
{
    return BEE_CUDA_VERSION_STRING;
}

auto has_cutlass() noexcept -> bool
{
    return BEE_HAS_CUTLASS != 0;
}

auto compiled_with_nvcc() noexcept -> bool
{
    return detail::runtime_compiled_with_nvcc();
}

// ── 设备查询 ────────────────────────────────────────────────────────────────

auto device_count() noexcept -> int
{
    int n = 0;
    (void)detail::runtime_get_device_count(&n);
    return n;
}

namespace
{

[[nodiscard]] auto cuda_err_to_error(int err, std::string_view op) -> Error
{
    return make_error(
        std::format("{} 失败: {} ({})", op, detail::runtime_error_name(err), detail::runtime_error_string(err)),
        Severity::Recoverable,
        err
    );
}

} // namespace

auto set_device(int device_index) -> Result<void>
{
    const int err = detail::runtime_set_device(device_index);
    if (err != 0)
        return std::unexpected(cuda_err_to_error(err, std::format("cudaSetDevice({})", device_index)));
    return {};
}

auto device_synchronize() -> Result<void>
{
    const int err = detail::runtime_device_synchronize();
    if (err != 0)
        return std::unexpected(cuda_err_to_error(err, "cudaDeviceSynchronize"));
    return {};
}

// ── 内存通路（M2 实装） ─────────────────────────────────────────────────────

auto allocate(std::size_t nbytes, std::size_t /*alignment*/) -> Result<void*>
{
    if (nbytes == 0) return static_cast<void*>(nullptr);

    int dev = 0;
    BEE_CUDA_CHECK(cudaGetDevice(&dev));

    MemoryPool* pool = nullptr;
    BEE_TRY_ASSIGN(pool, MemoryPool::get_default(dev));

    const auto stream = StreamView::per_thread();
    void* ptr = nullptr;
    BEE_TRY_ASSIGN(ptr, pool->allocate_async(nbytes, stream));
    // 分配后同步：返回的指针立即可用（plan-cuda §5 同步 API 语义）。
    BEE_CUDA_CHECK(cudaStreamSynchronize(stream.native_handle()));
    return ptr;
}

void deallocate(void* ptr, std::size_t /*nbytes*/, std::size_t /*alignment*/) noexcept
{
    if (!ptr) return;
    int dev = 0;
    if (cudaGetDevice(&dev) != cudaSuccess) {
        (void)cudaGetLastError();
        return;
    }
    auto pool_r = MemoryPool::get_default(dev);
    if (!pool_r) return;
    const auto stream = StreamView::per_thread();
    pool_r.value()->deallocate_async(ptr, stream);
    // 同步，确保 free 生效后再继续（与 allocate 的同步语义对称）。
    (void)cudaStreamSynchronize(stream.native_handle());
}

namespace
{

[[nodiscard]] auto sync_memcpy(void* dst, const void* src, std::size_t nbytes, cudaMemcpyKind kind) -> Result<void>
{
    if (nbytes == 0) return {};
    const auto stream = StreamView::per_thread();
    BEE_CUDA_CHECK(cudaMemcpyAsync(dst, src, nbytes, kind, stream.native_handle()));
    BEE_CUDA_CHECK(cudaStreamSynchronize(stream.native_handle()));
    return {};
}

} // namespace

auto memcpy_h2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return sync_memcpy(dst, src, nbytes, cudaMemcpyHostToDevice);
}

auto memcpy_d2h(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return sync_memcpy(dst, src, nbytes, cudaMemcpyDeviceToHost);
}

auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return sync_memcpy(dst, src, nbytes, cudaMemcpyDeviceToDevice);
}

auto memset(void* ptr, int value, std::size_t nbytes) -> Result<void>
{
    if (nbytes == 0) return {};
    const auto stream = StreamView::per_thread();
    BEE_CUDA_CHECK(cudaMemsetAsync(ptr, value, nbytes, stream.native_handle()));
    BEE_CUDA_CHECK(cudaStreamSynchronize(stream.native_handle()));
    return {};
}

} // namespace bee::cuda
