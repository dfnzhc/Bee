/**
 * @File EventBus.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Event/Signal.hpp"

#include <memory>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace bee
{

// =============================================================================
// EventBus — 基于类型索引的发布/订阅路由器，底层使用 Signal
// =============================================================================

class EventBus
{
public:
    EventBus()  = default;
    ~EventBus() = default;

    EventBus(const EventBus&)                    = delete;
    auto operator=(const EventBus&) -> EventBus& = delete;
    EventBus(EventBus&&)                         = delete;
    auto operator=(EventBus&&) -> EventBus&      = delete;

    // -------------------------------------------------------------------------
    // 订阅 / 发布
    // -------------------------------------------------------------------------

    /// 订阅类型为 E 的事件。返回 Connection 句柄。
    template <typename E>
    [[nodiscard]]
    auto subscribe(MoveOnlyFunction<void(const E&)> handler, int priority = 0) -> Connection
    {
        return get_or_create_channel<E>().signal.connect(std::move(handler), priority);
    }

    /// 发布事件 — 同步分发给所有订阅者。
    template <typename E>
    auto publish(const E& event) -> void
    {
        auto* ch = find_channel<E>();
        if (ch) {
            ch->signal.emit(event);
        }
    }

    /// 通过线程池异步分发事件。
    /// 注意：调用方在实例化此方法前须 #include "Concurrency/Thread/ThreadPool.hpp"。
    template <typename E, typename Pool>
    auto publish_async(Pool& pool, const E& event) -> void
    {
        auto* ch = find_channel<E>();
        if (ch) {
            ch->signal.emit_async(pool, event);
        }
    }

    // -------------------------------------------------------------------------
    // 清除
    // -------------------------------------------------------------------------

    /// 移除事件类型 E 的所有订阅者。
    template <typename E>
    auto clear() -> void
    {
        auto* ch = find_channel<E>();
        if (ch) {
            ch->signal.disconnect_all();
        }
    }

    /// 移除所有事件类型的全部订阅者。
    /// Channel 对象本身保留（开销极小）— 避免与正在执行 publish() 的裸指针产生 use-after-free。
    auto clear_all() -> void
    {
        std::unique_lock lock(mutex_);
        for (auto& [key, channel] : channels_) {
            channel->disconnect_all();
        }
    }

    // -------------------------------------------------------------------------
    // 错误处理
    // -------------------------------------------------------------------------

    /// 设置错误处理器，应用于所有已存在和未来创建的 channel。
    auto set_error_handler(MoveOnlyFunction<void(std::exception_ptr)> handler) -> void
    {
        std::unique_lock lock(mutex_);
        error_handler_ = std::make_shared<MoveOnlyFunction<void(std::exception_ptr)>>(std::move(handler));
        for (auto& [key, channel] : channels_) {
            channel->apply_error_handler(error_handler_);
        }
    }

private:
    // -------------------------------------------------------------------------
    // 类型擦除的 channel
    // -------------------------------------------------------------------------

    struct IChannel
    {
        virtual ~IChannel() = default;
        virtual auto disconnect_all() -> void = 0;
        virtual auto apply_error_handler(
                std::shared_ptr<MoveOnlyFunction<void(std::exception_ptr)>> handler) -> void = 0;
    };

    template <typename E>
    struct Channel : IChannel
    {
        Signal<void(const E&)> signal;

        auto disconnect_all() -> void override
        {
            signal.disconnect_all();
        }

        auto apply_error_handler(
                std::shared_ptr<MoveOnlyFunction<void(std::exception_ptr)>> handler) -> void override
        {
            if (handler) {
                // 将共享处理器包装一层，使 Signal 拥有自己的 MoveOnlyFunction 拷贝
                auto h = handler;
                signal.set_error_handler(
                        MoveOnlyFunction<void(std::exception_ptr)>(
                                [h = std::move(h)](std::exception_ptr ep) {
                                    (*h)(ep);
                                }));
            }
        }
    };

    // -------------------------------------------------------------------------
    // Channel 查找（双重检查锁定）
    // -------------------------------------------------------------------------

    template <typename E>
    auto get_or_create_channel() -> Channel<E>&
    {
        auto key = std::type_index(typeid(E));

        // 快速路径：读锁
        {
            std::shared_lock lock(mutex_);
            auto it = channels_.find(key);
            if (it != channels_.end()) {
                return static_cast<Channel<E>&>(*it->second);
            }
        }

        // 慢速路径：写锁
        std::unique_lock lock(mutex_);
        auto [it, inserted] = channels_.try_emplace(key, nullptr);
        if (inserted) {
            auto ch = std::make_unique<Channel<E>>();
            if (error_handler_) {
                ch->apply_error_handler(error_handler_);
            }
            it->second = std::move(ch);
        }
        return static_cast<Channel<E>&>(*it->second);
    }

    /// 返回已存在的 Channel 裸指针；若不存在则返回 nullptr。
    /// 安全性不变量：channels_ 中的条目仅在 get_or_create_channel() 中插入，从不被移除或替换。
    /// 因此即使在 shared_lock 释放后，返回的裸指针仍然有效（unique_ptr 生命周期 >= EventBus）。
    /// Signal 内部通过 COW atomic<shared_ptr> 保证线程安全，无需在调用方持锁。
    /// ⚠ 若将来需要删除 Channel，此处必须同步修改为在锁内完成操作。
    template <typename E>
    auto find_channel() -> Channel<E>*
    {
        auto key = std::type_index(typeid(E));
        std::shared_lock lock(mutex_);
        auto it = channels_.find(key);
        if (it == channels_.end()) {
            return nullptr;
        }
        return static_cast<Channel<E>*>(it->second.get());
    }

    // -------------------------------------------------------------------------
    // 数据成员
    // -------------------------------------------------------------------------

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::type_index, std::unique_ptr<IChannel>> channels_;
    std::shared_ptr<MoveOnlyFunction<void(std::exception_ptr)>> error_handler_;
};

} // namespace bee
