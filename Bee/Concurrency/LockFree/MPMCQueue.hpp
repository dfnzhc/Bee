/**
 * @File MPMCQueue.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#pragma once

#include <limits>
#include <semaphore>

#include "Base/Bit.hpp"
#include "Base/Defines.hpp"
#include "Concurrency/Threading.hpp"

namespace bee
{

/**
 * @brief 无锁、有界的多生产者多消费者队列 - MPMCQueue
 * 
 * 1) 采用“全局票据 + 槽位序号（sequence）”协议：
 *    - 生产者通过 enqueue_pos_ 竞争一个 ticket（位置）。
 *    - 消费者通过 dequeue_pos_ 竞争一个 ticket（位置）。
 *    - 每个槽位 Cell::sequence 表示该槽当前可执行的阶段。
 *
 * 2) sequence 语义（容量记为 N，当前位置记为 pos）：
 *    - 入队可写条件：sequence == pos
 *    - 入队完成发布：sequence = pos + 1
 *    - 出队可读条件：sequence == pos + 1
 *    - 出队完成发布：sequence = pos + N
 *
 * 3) 并发安全与内存序：
 *    - sequence 读取使用 acquire，发布使用 release，保证对象构造/析构可见性。
 *    - 票据 CAS 使用 relaxed，减少热路径栅栏成本。
 *
 * 4) 阻塞接口实现策略：
 *    - push/pop 并非内核阻塞，而是“自旋 + 退避 + yield”的用户态等待。
 *    - 这有利于低延迟和高吞吐，但在超高争用时会消耗更多 CPU。
 *
 * 5) 关键不变量：
 *    - enqueue_pos_ / dequeue_pos_ 单调递增（按 ticket 语义推进）。
 *    - 生产者完成构造后，必须先发布 sequence，再允许消费者读取。
 *    - 消费者完成读取/析构后，必须发布“可复用”sequence，生产者才可覆盖该槽位。
 *
 * 6) 线程职责：
 *    - 任意线程都可生产、任意线程都可消费（MPMC）。
 *    - try_* 接口不保证成功，但保证“当下不可达时快速返回”。
 *    - blocking 接口保证最终完成，但可能在高争用下让出时间片。
 *
 * 7) 生命周期契约：
 *    - 析构前，拥有者必须先停止所有仍可能访问该队列的线程。
 *    - 队列析构与并发 push/pop 同时发生属于未定义行为。
 */
template <typename T,
          typename Allocator = std::allocator<T>,
          typename SpinPolicy = DefaultSpinPolicy,
          bool ForceRoundUpPowerOfTwo = true>
class MPMCQueue
{
    // 类型是 trivial 类型，可以直接拷贝避免构造
    static constexpr bool kTrivialFastPath = std::is_trivially_copyable_v<T> &&
                                             std::is_trivially_destructible_v<T> &&
                                             std::is_default_constructible_v<T>;

public:
    using value_type      = T;
    using size_type       = size_t;
    using reference       = T&;
    using const_reference = const T&;
    using allocator_type  = Allocator;

    // =========================================================================
    // 构造与生命周期管理
    // =========================================================================

    explicit MPMCQueue(size_type capacity, const Allocator& allocator = Allocator())
        : _capacity(normalize_capacity(capacity))
        , _allocator(allocator)
        , _cell_allocator(_allocator)
        , _capacity_mask(_capacity - 1)
    {
        // 预分配固定容量槽位；该队列是有界队列，不在运行时扩容。
        // capacity_=1 仍合法，此时队列可容纳 1 个元素（不会额外扩容）。
        _cells = std::allocator_traits<cell_allocator_type>::allocate(_cell_allocator, _capacity);

        size_type constructed_cells = 0;
        try {
            for (size_type index = 0; index < _capacity; ++index) {
                // 初始 sequence = index，表示第 index 个 ticket 可立即写入对应槽位。
                std::allocator_traits<cell_allocator_type>::construct(_cell_allocator, _cells + index, index);
                ++constructed_cells;
                if constexpr (kTrivialFastPath) {
                    std::construct_at(_cells[index].ptr(), T{});
                }
            }
        } catch (...) {
            for (size_type index = 0; index < constructed_cells; ++index) {
                std::allocator_traits<cell_allocator_type>::destroy(_cell_allocator, _cells + index);
            }
            std::allocator_traits<cell_allocator_type>::deallocate(_cell_allocator, _cells, _capacity);
            _cells = nullptr;
            throw;
        }
    }

    ~MPMCQueue() noexcept
    {
        if (_cells == nullptr) {
            return;
        }

        if constexpr (!kTrivialFastPath) {
            // 非 trivial 路径需显式析构仍在“可读状态”的对象，避免资源泄漏。
            const auto end = _enqueue_pos.load(std::memory_order_relaxed);
            for (auto ticket = _dequeue_pos.load(std::memory_order_relaxed); ticket < end; ++ticket) {
                auto& cell = cell_ref(ticket);
                if (cell.sequence.load(std::memory_order_acquire) == (ticket + 1)) {
                    std::destroy_at(cell.ptr());
                }
            }
        }

        for (size_type index = 0; index < _capacity; ++index) {
            std::allocator_traits<cell_allocator_type>::destroy(_cell_allocator, _cells + index);
        }
        std::allocator_traits<cell_allocator_type>::deallocate(_cell_allocator, _cells, _capacity);
    }

    MPMCQueue(const MPMCQueue&)            = delete;
    MPMCQueue(MPMCQueue&&)                 = delete;
    MPMCQueue& operator=(const MPMCQueue&) = delete;
    MPMCQueue& operator=(MPMCQueue&&)      = delete;

    // =========================================================================
    // 生产者
    // =========================================================================

    template <typename... Args>
    [[nodiscard]] bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        static_assert(std::is_nothrow_constructible_v<T, Args&&...>, "T must be nothrow constructible with Args&&...");

        auto position            = _enqueue_pos.load(std::memory_order_relaxed);
        std::uint32_t contention = 0;
        // try_* 语义：只尝试一次“可达成功路径”；若当前不可写，快速返回 false。
        if (!try_reserve_enqueue(position, contention)) {
            return false;
        }

        auto& cell = cell_ref(position);
        // 成功保留位置后，只有当前线程会写该槽位。
        write_value(cell, std::forward<Args>(args)...);
        // 发布可读状态。
        // release 确保对象写入对随后 acquire 的消费者可见。
        cell.sequence.store(position + 1, std::memory_order_release);
        return true;
    }

    template <typename... Args>
    [[nodiscard]] bool emplace_if_not_full(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        return try_emplace(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool try_push(const_reference value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        static_assert(std::is_nothrow_copy_constructible_v<T>, "T must be nothrow copy constructible");
        return try_emplace(value);
    }

    [[nodiscard]] bool try_push(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        static_assert(std::is_nothrow_move_constructible_v<T>, "T must be nothrow move constructible");
        return try_emplace(std::move(value));
    }

    template <typename P>
        requires std::constructible_from<T, P&&>
    [[nodiscard]] bool try_push(P&& value) noexcept(std::is_nothrow_constructible_v<T, P&&>)
    {
        return try_emplace(std::forward<P>(value));
    }

    [[nodiscard]] bool push_if_not_full(const_reference value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        return try_push(value);
    }

    [[nodiscard]] bool push_if_not_full(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        return try_push(std::move(value));
    }

    template <typename P>
        requires std::constructible_from<T, P&&>
    [[nodiscard]] bool push_if_not_full(P&& value) noexcept(std::is_nothrow_constructible_v<T, P&&>)
    {
        return try_push(std::forward<P>(value));
    }

    template <typename... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        static_assert(std::is_nothrow_constructible_v<T, Args&&...>, "T must be nothrow constructible with Args&&...");

        auto position = _enqueue_pos.load(std::memory_order_relaxed);
        // blocking 路径：持续等待直到抢到可写 ticket。
        reserve_enqueue_blocking(position);

        auto& cell = cell_ref(position);
        write_value(cell, std::forward<Args>(args)...);
        cell.sequence.store(position + 1, std::memory_order_release);
    }

    void push(const_reference value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        emplace(value);
    }

    void push(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        emplace(std::move(value));
    }

    template <typename P>
        requires std::constructible_from<T, P&&>
    void push(P&& value) noexcept(std::is_nothrow_constructible_v<T, P&&>)
    {
        emplace(std::forward<P>(value));
    }

    // =========================================================================
    // 消费者
    // =========================================================================

    [[nodiscard]] bool try_pop(reference out_value) noexcept
    {
        static_assert(std::is_nothrow_move_assignable_v<T>, "T must be nothrow move assignable");
        static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");

        auto position            = _dequeue_pos.load(std::memory_order_relaxed);
        std::uint32_t contention = 0;
        // try_* 语义：若当前不可读，立即返回 false，不做长等待。
        if (!try_reserve_dequeue(position, contention)) {
            return false;
        }

        auto& cell = cell_ref(position);
        read_value(cell, out_value);
        // 释放槽位给下一轮写入。
        // position + capacity_ 是本算法中的“下一轮可写序号”。
        cell.sequence.store(position + _capacity, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop_if_not_empty(reference out_value) noexcept
    {
        return try_pop(out_value);
    }

    void pop(reference out_value) noexcept
    {
        static_assert(std::is_nothrow_move_assignable_v<T>, "T must be nothrow move assignable");
        static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");

        auto position = _dequeue_pos.load(std::memory_order_relaxed);
        // blocking 路径：持续等待直到抢到可读 ticket。
        reserve_dequeue_blocking(position);

        auto& cell = cell_ref(position);
        read_value(cell, out_value);
        cell.sequence.store(position + _capacity, std::memory_order_release);
    }

    // =========================================================================
    // 状态访问与查询
    // =========================================================================

    [[nodiscard]] size_type capacity() const noexcept
    {
        return _capacity;
    }

    [[nodiscard]] std::ptrdiff_t size_approx() const noexcept
    {
        const auto head = _enqueue_pos.load(std::memory_order_relaxed);
        const auto tail = _dequeue_pos.load(std::memory_order_relaxed);
        return static_cast<std::ptrdiff_t>(head - tail);
    }

    [[nodiscard]] bool is_empty() const noexcept
    {
        return size_approx() <= 0;
    }

    [[nodiscard]] bool is_full() const noexcept
    {
        return size_approx() >= static_cast<std::ptrdiff_t>(_capacity);
    }

private:
    struct alignas(BEE_CACHE_LINE_SIZE) Cell
    {
        explicit Cell(size_type initial_sequence) noexcept
            : sequence(initial_sequence)
        {
        }

        ~Cell() = default;

        T* ptr() noexcept
        {
            // storage 是原始字节区，ptr() 返回当前构造对象地址。
            return std::launder(reinterpret_cast<T*>(&storage));
        }

        // sequence 是该算法的核心状态机。
        // 约定：
        // - 入队可写：sequence == ticket
        // - 入队完成：sequence = ticket + 1
        // - 出队完成：sequence = ticket + capacity
        std::atomic<size_type> sequence;
        // std::aligned_storage_t<sizeof(T), alignof(T)> storage;
        alignas(T) std::byte storage[sizeof(T)];
    };

    using cell_allocator_type = std::allocator_traits<allocator_type>::template rebind_alloc<Cell>;

    static constexpr size_type normalize_capacity(size_type requested_capacity) noexcept
    {
        auto normalized = requested_capacity == 0 ? 1 : requested_capacity;
        if constexpr (ForceRoundUpPowerOfTwo) {
            auto rounded = RoundUpPowerOfTwo(normalized);
            if (rounded == 0) {
                rounded = HighestPowerOfTwoLEQ(std::numeric_limits<size_type>::max());
            }
            if (rounded >= 1) {
                normalized = rounded;
            }
        }
        return normalized;
    }

    [[nodiscard]] size_type cell_index(size_type ticket) const noexcept
    {
        // 与 SPSC 一致：将容量策略提升到模板参数，
        // 让热路径在编译期确定，避免运行时分支。
        if constexpr (ForceRoundUpPowerOfTwo) {
            return ticket & _capacity_mask;
        } else {
            return ticket % _capacity;
        }
    }

    [[nodiscard]] Cell& cell_ref(size_type ticket) noexcept
    {
        return _cells[cell_index(ticket)];
    }

    template <typename... Args>
    void write_value(Cell& cell, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        if constexpr (kTrivialFastPath) {
            *cell.ptr() = T(std::forward<Args>(args)...);
        } else {
            std::construct_at(cell.ptr(), std::forward<Args>(args)...);
        }
    }

    void read_value(Cell& cell, reference out_value) noexcept
    {
        out_value = std::move(*cell.ptr());
        if constexpr (!kTrivialFastPath) {
            std::destroy_at(cell.ptr());
        }
    }

    [[nodiscard]] bool try_reserve_enqueue(size_type& position, std::uint32_t& contention) noexcept
    {
        // 仅做“可立即成功”的保留尝试。
        for (;;) {
            auto& cell          = cell_ref(position);
            const auto sequence = cell.sequence.load(std::memory_order_acquire);
            const auto delta    = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position);
            // delta 语义：
            // - delta == 0：当前 ticket 对应槽位可写
            // - delta < 0：槽位尚未被消费释放（队列逻辑上满）
            // - delta > 0：本线程 position 落后，可前跳追平

            if (delta == 0) {
                if (_enqueue_pos.compare_exchange_weak(position, position + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    return true;
                }
                // CAS 失败通常代表竞争冲突（有其他线程先一步成功）。
                try_contention<SpinPolicy>(contention);
            } else if (delta < 0) {
                // 槽位还没被消费者释放，队列当前逻辑上“满”。
                return false;
            } else {
                // 该槽位已被推进到更高轮次，追赶到更可能成功的位置。
                position = sequence;
                try_contention<SpinPolicy>(contention);
            }
        }
    }

    [[nodiscard]] bool try_reserve_dequeue(size_type& position, std::uint32_t& contention) noexcept
    {
        // 仅做“可立即成功”的保留尝试。
        for (;;) {
            auto& cell          = cell_ref(position);
            const auto sequence = cell.sequence.load(std::memory_order_acquire);
            const auto delta    = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position + 1);
            // delta 语义：
            // - delta == 0：当前 ticket 对应槽位可读
            // - delta < 0：写入尚未完成（队列逻辑上空）
            // - delta > 0：本线程 position 落后，可前跳追平

            if (delta == 0) {
                if (_dequeue_pos.compare_exchange_weak(position, position + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    return true;
                }
                try_contention<SpinPolicy>(contention);
            } else if (delta < 0) {
                // 对应写入尚未完成，队列当前逻辑上“空”。
                return false;
            } else {
                position = sequence - 1;
                try_contention<SpinPolicy>(contention);
            }
        }
    }

    void reserve_enqueue_blocking(size_type& position) noexcept
    {
        // 与 try_reserve_enqueue 的核心判断相同，但不会返回 false，而是退避等待。
        std::uint32_t spin_count = 0;
        while (true) {
            auto& cell          = cell_ref(position);
            const auto sequence = cell.sequence.load(std::memory_order_acquire);
            const auto delta    = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position);
            if (delta == 0) {
                if (_enqueue_pos.compare_exchange_weak(position, position + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    return;
                }
            } else if (delta > 0) {
                position = sequence;
            }

            adaptive_backoff<SpinPolicy>(spin_count, _producer_spin_limit, _producer_yield_limit);
        }
    }

    void reserve_dequeue_blocking(size_type& position) noexcept
    {
        // 与 try_reserve_dequeue 的核心判断相同，但不会返回 false，而是退避等待。
        std::uint32_t spin_count = 0;
        while (true) {
            auto& cell          = cell_ref(position);
            const auto sequence = cell.sequence.load(std::memory_order_acquire);
            const auto delta    = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position + 1);
            if (delta == 0) {
                if (_dequeue_pos.compare_exchange_weak(position, position + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    return;
                }
            } else if (delta > 0) {
                position = sequence - 1;
            }

            adaptive_backoff<SpinPolicy>(spin_count, _consumer_spin_limit, _consumer_yield_limit);
        }
    }

private:
    // 固定容量（有界）
    size_type _capacity = 0;
    // 槽位数组，按 ticket 映射访问。
    Cell* _cells = nullptr;
    BEE_NO_UNIQUE_ADDRESS allocator_type _allocator;
    BEE_NO_UNIQUE_ADDRESS cell_allocator_type _cell_allocator;
    size_type _capacity_mask = 0;

    // 固定退避参数：工程化版本以稳定、可读为优先。
    // 可按平台经验调整，但建议保持“先小改、后观测”的节奏。
    std::uint32_t _producer_spin_limit  = 128u;
    std::uint32_t _producer_yield_limit = 1024u;
    std::uint32_t _consumer_spin_limit  = 128u;
    std::uint32_t _consumer_yield_limit = 1024u;

    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _enqueue_pos = {0};
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _dequeue_pos = {0};
};

template <typename T, typename Allocator = std::allocator<T>, typename SpinPolicy = DefaultSpinPolicy>
using MPMCQueueExact = MPMCQueue<T, Allocator, SpinPolicy, false>;

template <typename T, typename Allocator = std::allocator<T>>
using MPMCQueueThroughput = MPMCQueue<T, Allocator, ThroughputSpinPolicy, true>;

template <typename T, typename Allocator = std::allocator<T>>
using MPMCQueueExactThroughput = MPMCQueue<T, Allocator, ThroughputSpinPolicy, false>;

} // namespace bee
