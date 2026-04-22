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

// ── 内存（M2 实装） ─────────────────────────────────────────────────────────

auto allocate(std::size_t /*nbytes*/, std::size_t /*alignment*/) -> Result<void*>
{
    return std::unexpected(make_error("bee::cuda::allocate 尚未实装（计划于 M2）", Severity::Recoverable));
}

void deallocate(void* /*ptr*/, std::size_t /*nbytes*/, std::size_t /*alignment*/) noexcept
{
    // 尚未实装（M2）
}

auto memcpy_h2d(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/) -> Result<void>
{
    return std::unexpected(make_error("bee::cuda::memcpy_h2d 尚未实装（计划于 M2）", Severity::Recoverable));
}

auto memcpy_d2h(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/) -> Result<void>
{
    return std::unexpected(make_error("bee::cuda::memcpy_d2h 尚未实装（计划于 M2）", Severity::Recoverable));
}

auto memcpy_d2d(void* /*dst*/, const void* /*src*/, std::size_t /*nbytes*/) -> Result<void>
{
    return std::unexpected(make_error("bee::cuda::memcpy_d2d 尚未实装（计划于 M2）", Severity::Recoverable));
}

} // namespace bee::cuda
