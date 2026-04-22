/**
 * @File Core/Event.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief cudaEvent_t 的 RAII 封装（host only）。
 *
 * 用于 stream 间同步与计时。默认创建 DisableTiming + BlockingSync 较轻量的 event；
 * 要做 kernel 级计时需显式传 cudaEventDefault。
 */

#pragma once

#include <cuda_runtime.h>

#include <utility>

#include "Base/Diagnostics/Error.hpp"
#include "CUDA/Core/Check.hpp"
#include "CUDA/Core/Stream.hpp"

namespace bee::cuda
{

class Event
{
public:
    Event() noexcept = default;

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&& other) noexcept
        : event_(std::exchange(other.event_, nullptr))
    {}

    Event& operator=(Event&& other) noexcept
    {
        if (this != &other) {
            reset();
            event_ = std::exchange(other.event_, nullptr);
        }
        return *this;
    }

    ~Event() { reset(); }

    // 默认 flags 适合轻量同步（不启用 timing）；要测时请传 cudaEventDefault。
    [[nodiscard]] static auto create(unsigned flags = cudaEventDisableTiming | cudaEventBlockingSync) -> Result<Event>
    {
        cudaEvent_t e = nullptr;
        BEE_CUDA_CHECK(cudaEventCreateWithFlags(&e, flags));
        Event ev;
        ev.event_ = e;
        return ev;
    }

    [[nodiscard]] auto record(StreamView stream = StreamView::per_thread()) const -> Result<void>
    {
        BEE_CUDA_CHECK(cudaEventRecord(event_, stream.native_handle()));
        return {};
    }

    [[nodiscard]] auto synchronize() const -> Result<void>
    {
        BEE_CUDA_CHECK(cudaEventSynchronize(event_));
        return {};
    }

    // 返回 true 代表事件已完成。
    [[nodiscard]] auto query() const -> Result<bool>
    {
        const cudaError_t err = cudaEventQuery(event_);
        if (err == cudaSuccess) return true;
        if (err == cudaErrorNotReady) return false;
        (void)cudaGetLastError();
        return std::unexpected(detail::to_error(err, "cudaEventQuery"));
    }

    // 两事件间的毫秒时间差（start 先 record、this 后 record）。
    // 要求两事件均以 cudaEventDefault 标志创建（启用 timing）。
    [[nodiscard]] auto elapsed_ms_since(const Event& start) const -> Result<float>
    {
        float ms = 0.f;
        BEE_CUDA_CHECK(cudaEventElapsedTime(&ms, start.event_, event_));
        return ms;
    }

    [[nodiscard]] cudaEvent_t native_handle() const noexcept { return event_; }

    [[nodiscard]] explicit operator bool() const noexcept { return event_ != nullptr; }

    void reset() noexcept
    {
        if (event_) {
            (void)cudaEventDestroy(event_);
            event_ = nullptr;
        }
    }

private:
    cudaEvent_t event_ = nullptr;
};

} // namespace bee::cuda
