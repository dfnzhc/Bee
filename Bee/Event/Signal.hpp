/**
 * @File Signal.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Event/Connection.hpp"
#include "Base/Core/MoveOnlyFunction.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace bee
{

// =============================================================================
// Signal<void(Args...)> — 线程安全的多播委托，基于 COW 槽列表
// =============================================================================

template <typename>
class Signal; // 主模板，未定义

template <typename... Args>
class Signal<void(Args...)>
{
public:
    Signal() = default;

    ~Signal()
    {
        disconnect_all();
    }

    Signal(Signal&& other) noexcept
    {
        auto snapshot = other.slots_.load(std::memory_order_acquire);
        slots_.store(snapshot, std::memory_order_release);
        other.slots_.store(nullptr, std::memory_order_release);

        auto handler = other.error_handler_.load(std::memory_order_acquire);
        error_handler_.store(handler, std::memory_order_release);
        other.error_handler_.store(nullptr, std::memory_order_release);
    }

    auto operator=(Signal&& other) noexcept -> Signal&
    {
        if (this != &other) {
            disconnect_all();

            auto snapshot = other.slots_.load(std::memory_order_acquire);
            slots_.store(snapshot, std::memory_order_release);
            other.slots_.store(nullptr, std::memory_order_release);

            auto handler = other.error_handler_.load(std::memory_order_acquire);
            error_handler_.store(handler, std::memory_order_release);
            other.error_handler_.store(nullptr, std::memory_order_release);
        }
        return *this;
    }

    Signal(const Signal&)                    = delete;
    auto operator=(const Signal&) -> Signal& = delete;

    // -------------------------------------------------------------------------
    // 连接 / 断开
    // -------------------------------------------------------------------------

    /// 连接一个可调用槽。返回 Connection 句柄。
    /// 优先级：数值越小越先调用，默认为 0。
    [[nodiscard]]
    auto connect(MoveOnlyFunction<void(Args...)> slot, int priority = 0) -> Connection
    {
        auto state = std::make_shared<detail::SlotState>();
        auto cb    = std::make_shared<MoveOnlyFunction<void(Args...)>>(std::move(slot));

        while (true) {
            auto old_list = slots_.load(std::memory_order_acquire);
            auto new_list = old_list ? std::make_shared<SlotList>(*old_list) : std::make_shared<SlotList>();

            auto it = std::upper_bound(
                    new_list->begin(), new_list->end(), priority,
                    [](int p, const Slot& s) {
                        return p < s.priority;
                    });
            new_list->insert(it, Slot{state, cb, priority});

            if (slots_.compare_exchange_weak(old_list, new_list, std::memory_order_release, std::memory_order_acquire)) {
                return Connection(state);
            }
        }
    }

    /// 断开所有槽并清空列表。
    auto disconnect_all() -> void
    {
        auto old = slots_.exchange(nullptr, std::memory_order_acq_rel);
        if (old) {
            for (auto& slot : *old) {
                slot.state->active.store(false, std::memory_order_release);
            }
        }
    }

    // -------------------------------------------------------------------------
    // 发射
    // -------------------------------------------------------------------------

    /// 同步发射 — 按优先级顺序调用所有活跃槽。
    auto emit(Args... args) const -> void
    {
        auto snapshot = slots_.load(std::memory_order_acquire);
        if (!snapshot || snapshot->empty()) {
            return;
        }

        bool found_inactive = false;
        for (const auto& slot : *snapshot) {
            if (!slot.state->active.load(std::memory_order_acquire)) {
                found_inactive = true;
                continue;
            }
            try {
                (*slot.callback)(args...);
            } catch (...) {
                handle_error(std::current_exception());
            }
        }

        if (found_inactive) {
            try_compact();
        }
    }

    /// 异步发射 — 将参数拷贝到共享快照中，为每个活跃槽向线程池提交一个任务。
    /// Pool 须提供 post(callable) 方法（例如 bee::ThreadPool）。
    template <typename Pool>
    auto emit_async(Pool& pool, Args... args) const -> void
    {
        auto snapshot = slots_.load(std::memory_order_acquire);
        if (!snapshot || snapshot->empty()) {
            return;
        }

        auto args_tuple = std::make_shared<std::tuple<std::decay_t<Args>...>>(std::move(args)...);
        auto err        = error_handler_.load(std::memory_order_acquire);

        for (const auto& slot : *snapshot) {
            if (!slot.state->active.load(std::memory_order_acquire)) {
                continue;
            }

            auto cb = slot.callback;
            auto st = slot.state;
            auto at = args_tuple;
            auto eh = err;
            try {
                pool.post([cb = std::move(cb), st = std::move(st), at = std::move(at),
                            eh = std::move(eh)]() mutable {
                            if (!st->active.load(std::memory_order_acquire)) {
                                return;
                            }
                            try {
                                std::apply([&](auto&... a) {
                                    (*cb)(a...);
                                }, *at);
                            } catch (...) {
                                if (eh) {
                                    try {
                                        (*eh)(std::current_exception());
                                    } catch (...) {
                                    }
                                }
                            }
                        });
            } catch (...) {
                // 线程池拒绝了任务（如 FailFast 背压策略）— 转发给错误处理器
                if (err) {
                    try {
                        (*err)(std::current_exception());
                    } catch (...) {
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // 查询
    // -------------------------------------------------------------------------

    /// 当前活跃（未断开）的槽数量。
    [[nodiscard]] auto slot_count() const -> std::size_t
    {
        auto snapshot = slots_.load(std::memory_order_acquire);
        if (!snapshot) {
            return 0;
        }
        std::size_t count = 0;
        for (const auto& slot : *snapshot) {
            if (slot.state->active.load(std::memory_order_acquire)) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] auto empty() const -> bool
    {
        return slot_count() == 0;
    }

    // -------------------------------------------------------------------------
    // 错误处理
    // -------------------------------------------------------------------------

    /// 设置错误处理器，当槽在 emit() 中抛异常时被调用。
    /// 传入空的 MoveOnlyFunction 可清除处理器。
    auto set_error_handler(MoveOnlyFunction<void(std::exception_ptr)> handler) -> void
    {
        if (handler) {
            error_handler_.store(
                    std::make_shared<MoveOnlyFunction<void(std::exception_ptr)>>(std::move(handler)),
                    std::memory_order_release);
        } else {
            error_handler_.store(nullptr, std::memory_order_release);
        }
    }

private:
    struct Slot
    {
        std::shared_ptr<detail::SlotState> state;
        std::shared_ptr<MoveOnlyFunction<void(Args...)>> callback;
        int priority = 0;
    };

    using SlotList = std::vector<Slot>;

    auto handle_error(std::exception_ptr ep) const -> void
    {
        auto handler = error_handler_.load(std::memory_order_acquire);
        if (handler) {
            try {
                (*handler)(ep);
            } catch (...) {
                // 错误处理器自身抛异常 — 吞掉以防级联崩溃
            }
        }
    }

    /// 尽力而为的懒惰压缩 — 从列表中移除已失活的槽。
    auto try_compact() const -> void
    {
        auto old_list = slots_.load(std::memory_order_acquire);
        if (!old_list) {
            return;
        }

        auto new_list = std::make_shared<SlotList>();
        new_list->reserve(old_list->size());
        for (const auto& slot : *old_list) {
            if (slot.state->active.load(std::memory_order_acquire)) {
                new_list->push_back(slot);
            }
        }

        // 尽力 CAS — 若其他线程已修改列表，则跳过。
        auto expected = old_list;
        slots_.compare_exchange_strong(expected, new_list, std::memory_order_release, std::memory_order_relaxed);
    }

    mutable std::atomic<std::shared_ptr<SlotList>> slots_{nullptr};
    std::atomic<std::shared_ptr<MoveOnlyFunction<void(std::exception_ptr)>>> error_handler_{nullptr};
};

} // namespace bee
