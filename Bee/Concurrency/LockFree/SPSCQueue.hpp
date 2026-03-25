/**
 * @File SPSCQueue.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include <thread>
#include <atomic>
#include <cassert>

#include "Base/Defines.hpp"
#include "Base/Bit.hpp"
#include "Concurrency/Threading.hpp"

namespace bee
{

/**
 * @brief 无锁、有界的单生产者单消费者队列
 * 
 * 1) 单生产者 / 单消费者前提：
 *    - 写端只改 _writeIdx
 *    - 读端只改 _readIdx
 *    因而无需 CAS 即可实现无锁。
 *
 * 2) 容量策略：
 *    - 内部容量 = 逻辑容量 + 1（留一个空槽区分满/空）
 *    - ForceRoundUpPowerOfTwo=true 时优先使用按位与加速取模。
 *
 * 3) 缓存策略：
 *    - 生产者缓存 _readIdxCache，消费者缓存 _writeIdxCache，减少跨核读取对端原子索引的频率。
 *    
 * 4) 自旋策略：
 *    - 默认策略会调用 yield 换出自身
 *    - 吞吐量导向策略不会换出自身，在某些平台上更高效
 */
template <typename T,
          typename Allocator = std::allocator<T>,
          bool ForceRoundUpPowerOfTwo = true,
          typename SpinPolicy = DefaultSpinPolicy>
class BasicSPSCQueue
{
    // 类型是 trivial 类型，可以直接拷贝避免构造
    static constexpr bool kTrivialFastPath = std::is_trivially_copyable_v<T> &&
                                             std::is_trivially_destructible_v<T> &&
                                             std::is_default_constructible_v<T>;

public:
    using value_type      = T;
    using size_type       = size_t;
    using pointer         = T*;
    using reference       = T&;
    using const_reference = const T&;

    // =========================================================================
    // 构造与生命周期管理
    // =========================================================================
    explicit BasicSPSCQueue(size_type capacity, const Allocator& allocator = Allocator())
        : _capacity(normalize_internal_capacity(capacity))
        , _indexMask(initial_index_mask(_capacity))
        , _allocator(allocator)
    {
        // 只分配内存，不构造对象；对象在 push/emplace 时原位构造。
        _slots = std::allocator_traits<Allocator>::allocate(_allocator, _capacity);
        if constexpr (kTrivialFastPath) {
            for (size_type i = 0; i < _capacity; ++i) {
                std::allocator_traits<Allocator>::construct(_allocator, &_slots[i], T{});
            }
        }
    }

    ~BasicSPSCQueue()
    {
        // 析构时清理仍在队列中的对象。
        if constexpr (kTrivialFastPath) {
            for (size_type i = 0; i < _capacity; ++i) {
                destroy_slot(i);
            }
        } else {
            auto readIdx  = _readIdx.load(std::memory_order_relaxed);
            auto writeIdx = _writeIdx.load(std::memory_order_relaxed);
            while (readIdx != writeIdx) {
                destroy_slot(readIdx);
                readIdx = next_index(readIdx);
            }
        }
        std::allocator_traits<Allocator>::deallocate(_allocator, _slots, _capacity);
    }

    BasicSPSCQueue(const BasicSPSCQueue&)            = delete;
    BasicSPSCQueue(BasicSPSCQueue&&)                 = delete;
    BasicSPSCQueue& operator=(const BasicSPSCQueue&) = delete;
    BasicSPSCQueue& operator=(BasicSPSCQueue&&)      = delete;

    // =========================================================================
    // 生产者
    // =========================================================================

    // 非阻塞接口
    template <typename... Args>
    [[nodiscard]] bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        static_assert(std::is_constructible_v<T, Args&&...>, "T must be constructible with Args&&...");
        auto const writeIdx     = _writeIdx.load(std::memory_order_relaxed);
        auto const nextWriteIdx = next_index(writeIdx);
        // 只有在本地缓存判断“可能满”时才刷新对端读索引。
        if (!refresh_read_cache_if_full(nextWriteIdx)) {
            return false;
        }

        write_slot_emplace(writeIdx, std::forward<Args>(args)...);
        _writeIdx.store(nextWriteIdx, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool try_push(const_reference value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        static_assert(std::is_copy_constructible_v<T>, "T must be copy constructible");
        return try_push_value(value);
    }

    [[nodiscard]] bool try_push(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        return try_push_value(std::forward<T>(value));
    }

    template <typename P>
        requires std::constructible_from<T, P&&>
    [[nodiscard]] bool try_push(P&& v) noexcept(std::is_nothrow_constructible_v<T, P&&>)
    {
        return try_push_value(std::forward<P>(v));
    }

    // 阻塞接口
    template <typename... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        static_assert(std::is_constructible_v<T, Args&&...>, "T must be constructible with Args&&...");
        auto const writeIdx      = _writeIdx.load(std::memory_order_relaxed);
        auto const nextWriteIdx  = next_index(writeIdx);
        std::uint32_t spin_count = 0;
        // 阻塞语义：等待直到有空位。
        while (!refresh_read_cache_if_full(nextWriteIdx)) {
            SpinPolicy::wait(spin_count++);
        }

        write_slot_emplace(writeIdx, std::forward<Args>(args)...);
        _writeIdx.store(nextWriteIdx, std::memory_order_release);
    }

    void push(const_reference value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        static_assert(std::is_copy_constructible_v<T>, "T must be copy constructible");
        emplace(value);
    }

    void push(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        emplace(std::forward<T>(value));
    }

    template <typename P>
        requires std::constructible_from<T, P&&>
    void push(P&& v) noexcept(std::is_nothrow_constructible_v<T, P&&>)
    {
        emplace(std::forward<P>(v));
    }

    // =========================================================================
    // 消费者
    // =========================================================================
    [[nodiscard]] bool try_pop(reference out_value) noexcept
    {
        static_assert(std::is_nothrow_move_assignable_v<T>, "T must be nothrow move assignable");
        auto const readIdx = _readIdx.load(std::memory_order_relaxed);
        // 只有在本地缓存判断“可能空”时才刷新对端写索引。
        if (!refresh_write_cache_if_empty(readIdx)) {
            return false;
        }

        out_value = std::move(_slots[readIdx]);
        destroy_slot(readIdx);
        _readIdx.store(next_index(readIdx), std::memory_order_release);
        return true;
    }

    // 两阶段零拷贝出队
    [[nodiscard]] pointer try_front() noexcept
    {
        // 两阶段出队：front() 获取指针，pop() 真正提交出队。
        auto const readIdx = _readIdx.load(std::memory_order_relaxed);
        if (!refresh_write_cache_if_empty(readIdx)) {
            return nullptr;
        }

        return &_slots[readIdx];
    }

    [[nodiscard]] pointer front() noexcept
    {
        return try_front();
    }

    void pop() noexcept
    {
        static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");
        auto const readIdx = _readIdx.load(std::memory_order_relaxed);
        assert(readIdx != _writeIdxCache && "Can only call pop() after front() has returned a non-nullptr");
        destroy_slot(readIdx);
        _readIdx.store(next_index(readIdx), std::memory_order_release);
    }

    // =========================================================================
    // 状态访问与查询
    // =========================================================================

    [[nodiscard]] size_type capacity() const noexcept
    {
        // 外部可用容量 = 内部容量 - 1（保留 1 个哨兵空槽）。
        return _capacity - 1;
    }

    [[nodiscard]] size_type size_approx() const noexcept
    {
        auto const writeIdx = _writeIdx.load(std::memory_order_relaxed);
        auto const readIdx  = _readIdx.load(std::memory_order_relaxed);
        if (writeIdx >= readIdx) {
            return writeIdx - readIdx;
        }
        return _capacity - (readIdx - writeIdx);
    }

    [[nodiscard]] bool is_empty() const noexcept
    {
        return _writeIdx.load(std::memory_order_relaxed) == _readIdx.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool is_full() const noexcept
    {
        auto const nextWriteIdx = next_index(_writeIdx.load(std::memory_order_relaxed));
        return nextWriteIdx == _readIdx.load(std::memory_order_relaxed);
    }

private:
    static constexpr size_type normalize_internal_capacity(size_type requested_capacity) noexcept
    {
        // 统一容量归一化：
        // - 至少为 1
        // - 转为内部容量（+1）
        // - 可选 round-up 到 2 的幂
        auto logical_capacity = requested_capacity;
        if (logical_capacity < 1) {
            logical_capacity = 1;
        }

        constexpr auto kMaxLogicalCapacity = std::numeric_limits<size_type>::max() - 1;
        if (logical_capacity > kMaxLogicalCapacity) {
            logical_capacity = kMaxLogicalCapacity;
        }

        auto internal_capacity      = logical_capacity + 1;
        constexpr auto max_capacity = std::numeric_limits<size_type>::max();
        if (internal_capacity > max_capacity) {
            internal_capacity = max_capacity;
        }

        if constexpr (ForceRoundUpPowerOfTwo) {
            auto rounded = RoundUpPowerOfTwo(internal_capacity);
            if (rounded == 0 || rounded > max_capacity) {
                rounded = HighestPowerOfTwoLEQ(max_capacity);
            }
            if (rounded >= 2) {
                internal_capacity = rounded;
            }
        }

        return internal_capacity;
    }

    static constexpr size_type initial_index_mask(size_type capacity) noexcept
    {
        if constexpr (ForceRoundUpPowerOfTwo) {
            return capacity - 1;
        }
        return 0;
    }

    [[nodiscard]] size_type next_index(size_type index) const noexcept
    {
        // power-of-two 走位与，否则走通用分支。
        if constexpr (ForceRoundUpPowerOfTwo) {
            return (index + 1) & _indexMask;
        } else {
            auto next = index + 1;
            return (next == _capacity) ? 0 : next;
        }
    }

    template <typename U>
    [[nodiscard]] bool try_push_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>)
    {
        static_assert(std::is_constructible_v<T, U&&>, "T must be constructible with U&&");
        auto const writeIdx     = _writeIdx.load(std::memory_order_relaxed);
        auto const nextWriteIdx = next_index(writeIdx);
        if (!refresh_read_cache_if_full(nextWriteIdx)) {
            return false;
        }

        write_slot_value(writeIdx, std::forward<U>(value));

        _writeIdx.store(nextWriteIdx, std::memory_order_release);
        return true;
    }

    template <typename... Args>
    void write_slot_emplace(size_type index, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        if constexpr (kTrivialFastPath) {
            _slots[index] = T(std::forward<Args>(args)...);
        } else {
            std::allocator_traits<Allocator>::construct(_allocator, &_slots[index], std::forward<Args>(args)...);
        }
    }

    template <typename U>
    void write_slot_value(size_type index, U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>)
    {
        if constexpr (kTrivialFastPath) {
            _slots[index] = std::forward<U>(value);
        } else {
            std::allocator_traits<Allocator>::construct(_allocator, &_slots[index], std::forward<U>(value));
        }
    }

    void destroy_slot(size_type index) noexcept
    {
        if constexpr (!kTrivialFastPath) {
            std::allocator_traits<Allocator>::destroy(_allocator, &_slots[index]);
        }
    }

    [[nodiscard]] bool refresh_read_cache_if_full(size_type nextWriteIdx) noexcept
    {
        // 生产者局部快路径：没满时不读原子 _readIdx。
        if (nextWriteIdx != _readIdxCache) {
            return true;
        }

        _readIdxCache = _readIdx.load(std::memory_order_acquire);
        return nextWriteIdx != _readIdxCache;
    }

    [[nodiscard]] bool refresh_write_cache_if_empty(size_type readIdx) noexcept
    {
        // 消费者局部快路径：不空时不读原子 _writeIdx。
        if (readIdx != _writeIdxCache) {
            return true;
        }

        _writeIdxCache = _writeIdx.load(std::memory_order_acquire);
        return _writeIdxCache != readIdx;
    }

private:
    size_type _capacity  = 0;
    size_type _indexMask = 0;
    T* _slots            = nullptr;
    BEE_NO_UNIQUE_ADDRESS Allocator _allocator;

    // 对齐布局：尽量让生产者与消费者热点字段分离。
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _writeIdx = {0};
    alignas(BEE_CACHE_LINE_SIZE) size_type _readIdxCache          = {0};
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _readIdx  = {0};
    alignas(BEE_CACHE_LINE_SIZE) size_type _writeIdxCache         = {0};
};

template <typename T, typename Allocator = std::allocator<T>>
using SPSCQueue = BasicSPSCQueue<T, Allocator>;
template <typename T, typename Allocator = std::allocator<T>>
using SPSCQueueExact = BasicSPSCQueue<T, Allocator, false>;

template <typename T, typename Allocator = std::allocator<T>>
using SPSCQueueThroughput = BasicSPSCQueue<T, Allocator, true, ThroughputSpinPolicy>;
template <typename T, typename Allocator = std::allocator<T>>
using SPSCQueueExactThroughput = BasicSPSCQueue<T, Allocator, false, ThroughputSpinPolicy>;

} // namespace bee
