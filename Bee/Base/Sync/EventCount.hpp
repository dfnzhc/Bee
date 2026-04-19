/**
 * @File EventCount.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/20
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace bee
{

// =========================================================================
// EventCount — 基于 epoch 的条件等待原语（folly::EventCount 的 Bee 简化版）
// =========================================================================
//
// 用法契约（消费者侧，"check-prepare-check-wait" 模式）：
//
//     while (!condition_predicate()) {
//         auto key = ec.prepare_wait();          // 记录 epoch，登记 waiter
//         if (condition_predicate()) {           // 预检：若已满足则取消
//             ec.cancel_wait();
//             break;
//         }
//         ec.wait(key);                          // epoch 未变则阻塞
//     }
//
// 或使用便捷封装：
//
//     ec.await([&] { return condition_predicate(); });
//
// 生产者侧（设置条件后唤醒等待者）：
//
//     set_condition();
//     ec.notify();      // 唤醒一个
//     // 或
//     ec.notify_all();  // 唤醒全部
//
// 设计要点：
// 1) 采用"epoch + waiter 计数"双原子分离：epoch 每次 notify 递增，waiter
//    计数用于 notify 时判断是否需要真正发起 syscall（常见无等待场景零代价）。
// 2) wait 基于 std::atomic<uint64_t>::wait（C++20）。其"经典 spurious
//    wakeup"由 while-loop 重新比对 epoch 吸收。
// 3) prepare_wait → (cancel_wait | wait) 必须严格配对，否则 waiter_
//    计数会泄漏。所有操作均为 noexcept。
// 4) 不保证 FIFO；不提供"超时等待"。
//
// 与 folly::EventCount 差异：
// - 不用 waiter 链表，全部依赖 std::atomic::wait 的内核级 futex/addr 映射；
// - 不做自旋降级；
// - notify_one 的实际唤醒数量由 std::atomic::notify_one 决定（实现相关）。

class EventCount
{
public:
    class Key
    {
    public:
        Key() noexcept = default;

    private:
        friend class EventCount;
        explicit Key(std::uint64_t epoch) noexcept : epoch_(epoch) {}
        std::uint64_t epoch_{0};
    };

    EventCount() noexcept                    = default;
    EventCount(const EventCount&)            = delete;
    EventCount& operator=(const EventCount&) = delete;
    EventCount(EventCount&&)                 = delete;
    EventCount& operator=(EventCount&&)      = delete;

    // 登记等待并记录当前 epoch；必须与 cancel_wait 或 wait 配对。
    auto prepare_wait() noexcept -> Key
    {
        waiters_.fetch_add(1, std::memory_order_acq_rel);
        const auto cur = epoch_.load(std::memory_order_acquire);
        return Key{cur};
    }

    // 放弃等待（预检后发现条件已满足）。
    auto cancel_wait() noexcept -> void
    {
        waiters_.fetch_sub(1, std::memory_order_release);
    }

    // 若 epoch 未变则阻塞到被 notify；支持 spurious wakeup。
    auto wait(Key key) noexcept -> void
    {
        while (epoch_.load(std::memory_order_acquire) == key.epoch_) {
            epoch_.wait(key.epoch_, std::memory_order_acquire);
        }
        waiters_.fetch_sub(1, std::memory_order_release);
    }

    // 唤醒一个等待者。若无等待者则跳过 syscall。
    auto notify() noexcept -> void
    {
        epoch_.fetch_add(1, std::memory_order_release);
        if (waiters_.load(std::memory_order_acquire) != 0) {
            epoch_.notify_one();
        }
    }

    // 唤醒全部等待者。
    auto notify_all() noexcept -> void
    {
        epoch_.fetch_add(1, std::memory_order_release);
        if (waiters_.load(std::memory_order_acquire) != 0) {
            epoch_.notify_all();
        }
    }

    // 便捷封装：await(pred)。pred 在本线程多次调用，务必无副作用或幂等。
    template <typename Predicate>
    auto await(Predicate&& pred) noexcept(std::is_nothrow_invocable_v<Predicate&>) -> void
    {
        while (!pred()) {
            auto key = prepare_wait();
            if (pred()) {
                cancel_wait();
                return;
            }
            wait(key);
        }
    }

private:
    // epoch 递增是 notify 的信号；wait 比对此值。
    std::atomic<std::uint64_t> epoch_{0};
    // waiter 计数；notify 侧用于跳过无意义 syscall。
    std::atomic<std::uint64_t> waiters_{0};
};

} // namespace bee
