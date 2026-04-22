/**
 * @File Core/Stream.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief cudaStream_t 的 RAII 封装（host only）。
 *
 * 设计要点（plan-cuda §5）：
 *  - 默认内部使用 cudaStreamPerThread（不创建独立 stream），减少隐式同步；
 *  - 提供 OwnedStream：创建独立流并在析构时销毁；
 *  - 同步接口返回 Result<void>；
 *  - 仅 host 使用，任何 .cu 内部想拿裸 handle 用 native_handle()。
 */

#pragma once

#include <cuda_runtime.h>

#include <utility>

#include "Base/Diagnostics/Error.hpp"
#include "CUDA/Core/Check.hpp"

namespace bee::cuda
{

// 非拥有型 stream 视图；可由 cudaStreamPerThread 或 OwnedStream 构造。
class StreamView
{
public:
    StreamView() noexcept = default;

    explicit StreamView(cudaStream_t s) noexcept : stream_(s) {}

    [[nodiscard]] cudaStream_t native_handle() const noexcept { return stream_; }

    [[nodiscard]] auto synchronize() const -> Result<void>
    {
        BEE_CUDA_CHECK(cudaStreamSynchronize(stream_));
        return {};
    }

    // true 表示所有已提交到该 stream 的工作均已完成。
    [[nodiscard]] auto query() const -> Result<bool>
    {
        const cudaError_t err = cudaStreamQuery(stream_);
        if (err == cudaSuccess) return true;
        if (err == cudaErrorNotReady) return false;
        (void)cudaGetLastError();
        return std::unexpected(detail::to_error(err, "cudaStreamQuery"));
    }

    [[nodiscard]] static StreamView per_thread() noexcept
    {
        return StreamView{cudaStreamPerThread};
    }

    [[nodiscard]] static StreamView legacy_default() noexcept
    {
        return StreamView{cudaStreamLegacy};
    }

private:
    cudaStream_t stream_ = cudaStreamPerThread;
};

// 拥有型 stream；析构时调用 cudaStreamDestroy。
class OwnedStream
{
public:
    OwnedStream() noexcept = default;

    OwnedStream(const OwnedStream&) = delete;
    OwnedStream& operator=(const OwnedStream&) = delete;

    OwnedStream(OwnedStream&& other) noexcept
        : stream_(std::exchange(other.stream_, nullptr))
    {}

    OwnedStream& operator=(OwnedStream&& other) noexcept
    {
        if (this != &other) {
            reset();
            stream_ = std::exchange(other.stream_, nullptr);
        }
        return *this;
    }

    ~OwnedStream() { reset(); }

    // flags: cudaStreamDefault / cudaStreamNonBlocking
    // priority: 0 或由 cudaDeviceGetStreamPriorityRange 获得
    [[nodiscard]] static auto create(unsigned flags = cudaStreamNonBlocking, int priority = 0) -> Result<OwnedStream>
    {
        cudaStream_t s = nullptr;
        BEE_CUDA_CHECK(cudaStreamCreateWithPriority(&s, flags, priority));
        OwnedStream o;
        o.stream_ = s;
        return o;
    }

    [[nodiscard]] cudaStream_t native_handle() const noexcept { return stream_; }

    [[nodiscard]] StreamView view() const noexcept { return StreamView{stream_}; }

    [[nodiscard]] auto synchronize() const -> Result<void>
    {
        BEE_CUDA_CHECK(cudaStreamSynchronize(stream_));
        return {};
    }

    [[nodiscard]] explicit operator bool() const noexcept { return stream_ != nullptr; }

    void reset() noexcept
    {
        if (stream_) {
            (void)cudaStreamDestroy(stream_);
            stream_ = nullptr;
        }
    }

private:
    cudaStream_t stream_ = nullptr;
};

} // namespace bee::cuda
