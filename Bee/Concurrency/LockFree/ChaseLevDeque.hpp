/**
 * @File ChaseLevDeque.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#pragma once

#include <limits>

#include "Base/Core/Defines.hpp"
#include "Base/Numeric/Bit.hpp"
#include "Concurrency/Threading.hpp"

namespace bee
{

/**
 * @brief Chase-Lev Work-Stealing Deque
 *
 * 容量始终向上对齐到 2 的次幂，以便用按位与替代取模运算。
 *
 * 1) 角色分工：
 *    - owner 线程：负责 push/pop（操作 bottom_）。
 *    - stealer 线程：负责 steal（操作 top_）。
 *
 * 2) 并发模型：
 *    - top_/bottom_ 使用单调递增 ticket，不回退。
 *    - 实际槽位索引通过 ticket % capacity_ 映射。
 *
 * 3) 关键判定：
 *    - 队列大小近似为 bottom - top。
 *    - owner pop 到最后一个元素时，与 stealer 在 top 上 CAS 竞争。
 *
 * 4) 生命周期契约：
 *    - 析构前，调用者必须确保没有线程仍在访问该 deque。
 *    - 析构与并发 push/pop/steal 同时发生属于未定义行为。
 */
template <typename T, typename Allocator = std::allocator<T>>
class ChaseLevDeque
{
    // 类型是 trivial 类型，可以直接拷贝避免构造
    static constexpr bool kTrivialFastPath =
        std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T> && std::is_default_constructible_v<T>;

public:
    using value_type      = T;
    using size_type       = size_t;
    using allocator_type  = Allocator;
    using reference       = T&;
    using const_reference = const T&;

    // =========================================================================
    // 构造与生命周期管理
    // =========================================================================

    explicit ChaseLevDeque(size_type capacity, const Allocator& allocator = Allocator())
        : _capacity(normalize_capacity(capacity))
        , _capacity_mask(_capacity - 1)
        , _allocator(allocator)
    {
        _slots = std::allocator_traits<allocator_type>::allocate(_allocator, _capacity);

        size_type constructed = 0;
        try {
            if constexpr (kTrivialFastPath) {
                for (size_type i = 0; i < _capacity; ++i) {
                    std::allocator_traits<allocator_type>::construct(_allocator, _slots + i, T{});
                    ++constructed;
                }
            }
        } catch (...) {
            for (size_type i = 0; i < constructed; ++i) {
                std::allocator_traits<allocator_type>::destroy(_allocator, _slots + i);
            }
            std::allocator_traits<allocator_type>::deallocate(_allocator, _slots, _capacity);
            _slots = nullptr;
            throw;
        }
    }

    ~ChaseLevDeque() noexcept
    {
        if (_slots == nullptr) {
            return;
        }

        // 非 trivial 路径只销毁仍在队列中的有效对象。
        if constexpr (!kTrivialFastPath) {
            const auto top = _top.load(std::memory_order_relaxed);
            const auto bot = _bottom.load(std::memory_order_relaxed);
            for (auto ticket = top; ticket < bot; ++ticket) {
                std::allocator_traits<allocator_type>::destroy(_allocator, _slots + slot_index(ticket));
            }
        } else {
            for (size_type i = 0; i < _capacity; ++i) {
                std::allocator_traits<allocator_type>::destroy(_allocator, _slots + i);
            }
        }

        std::allocator_traits<allocator_type>::deallocate(_allocator, _slots, _capacity);
    }

    ChaseLevDeque(const ChaseLevDeque&)            = delete;
    ChaseLevDeque(ChaseLevDeque&&)                 = delete;
    ChaseLevDeque& operator=(const ChaseLevDeque&) = delete;
    ChaseLevDeque& operator=(ChaseLevDeque&&)      = delete;

    // =========================================================================
    // owner-only 入队
    // =========================================================================

    template <typename... Args>
    [[nodiscard]] bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        static_assert(std::is_constructible_v<T, Args&&...>, "T must be constructible with Args&&...");

        const auto bottom = _bottom.load(std::memory_order_relaxed);
        const auto top    = _top.load(std::memory_order_acquire);

        // 有界版本：容量满则快速失败，不做扩容。
        // 注意：使用 _capacity - 1 而非 _capacity 作为上限。
        // 原因：stealer 通过 CAS top 后仍在 read/destroy 对应槽位，
        // 若 owner 允许 bottom-top == capacity，则 slot_index(bottom) 会
        // 与 slot_index(old_top) 重叠，导致 owner 的 placement new 与
        // stealer 的 destroy 发生数据竞争（UB）。
        // 保留 1 个槽位的间隔确保物理槽位不会被并发读写。
        if ((bottom - top) >= _capacity - 1) {
            return false;
        }

        write_slot(slot_index(bottom), std::forward<Args>(args)...);
        // 发布新元素：先写对象，再 release bottom。
        _bottom.store(bottom + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool try_push(const_reference value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        static_assert(std::is_copy_constructible_v<T>, "T must be copy constructible");
        return try_emplace(value);
    }

    [[nodiscard]] bool try_push(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        static_assert(std::is_move_constructible_v<T>, "T must be move constructible");
        return try_emplace(std::move(value));
    }

    template <typename P>
        requires std::constructible_from<T, P&&>
    [[nodiscard]] bool try_push(P&& value) noexcept(std::is_nothrow_constructible_v<T, P&&>)
    {
        return try_emplace(std::forward<P>(value));
    }

    template <typename... Args>
    void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        std::uint32_t spin = 0;
        while (!try_emplace(std::forward<Args>(args)...)) {
            // owner 阻塞版本采用轻量自旋+周期性让出时间片。
            if ((spin & 0xFFu) == 0xFFu) {
                std::this_thread::yield();
            }
            ++spin;
        }
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
    // owner-only 出队（LIFO）
    // =========================================================================

    [[nodiscard]] bool try_pop(T& out_value) noexcept
    {
        static_assert(std::is_nothrow_move_assignable_v<T>, "T must be nothrow move assignable");

        auto bottom = _bottom.load(std::memory_order_relaxed);
        if (bottom == 0) {
            return false;
        }

        bottom -= 1;
        // owner 先尝试“预扣减”bottom，表示准备拿走尾元素。
        // 这一步只在 owner 线程执行，不与其他 writer 冲突。
        _bottom.store(bottom, std::memory_order_relaxed);
        // 与 stealer 在最后一个元素场景下通过全序栅栏对齐观察顺序。
        std::atomic_thread_fence(std::memory_order_seq_cst);

        auto top = _top.load(std::memory_order_relaxed);
        if (top > bottom) {
            // 说明原队列为空，回滚 bottom。
            _bottom.store(bottom + 1, std::memory_order_relaxed);
            return false;
        }

        if (top == bottom) {
            // 最后一个元素：owner 与 stealer 竞争 top。
            // CAS 成功表示 owner 获得该元素；失败表示 stealer 先一步拿走。
            if (!_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                // 竞争失败，元素已被 steal，回滚并返回失败。
                _bottom.store(bottom + 1, std::memory_order_relaxed);
                return false;
            }
            _bottom.store(bottom + 1, std::memory_order_relaxed);
        }

        auto* ptr = _slots + slot_index(bottom);
        out_value = std::move(*ptr);
        destroy_slot(ptr);
        return true;
    }

    void pop(T& out_value) noexcept
    {
        while (!try_pop(out_value)) {
            std::this_thread::yield();
        }
    }

    // =========================================================================
    // stealer-only 出队（FIFO）
    // =========================================================================

    [[nodiscard]] bool try_steal(T& out_value) noexcept
    {
        static_assert(std::is_nothrow_move_assignable_v<T>, "T must be nothrow move assignable");

        auto top = _top.load(std::memory_order_acquire);
        // 与 owner 的 seq_cst 栅栏配合，保证对 top/bottom 的观察次序。
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto bottom = _bottom.load(std::memory_order_acquire);
        if (top >= bottom) {
            return false;
        }

        // 抢到 top 才算偷成功。
        if (!_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
            return false;
        }

        auto* ptr = _slots + slot_index(top);
        out_value = std::move(*ptr);
        destroy_slot(ptr);
        return true;
    }

    void steal(T& out_value) noexcept
    {
        while (!try_steal(out_value)) {
            std::this_thread::yield();
        }
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
        const auto top = _top.load(std::memory_order_relaxed);
        const auto bot = _bottom.load(std::memory_order_relaxed);
        return static_cast<std::ptrdiff_t>(bot - top);
    }

    [[nodiscard]] bool is_empty() const noexcept
    {
        return size_approx() <= 0;
    }

private:
    static constexpr size_type normalize_capacity(size_type requested_capacity) noexcept
    {
        // 最小物理容量为 2：有效容量 = 物理容量 - 1，
        // 保留 1 个间隔槽位以避免 owner/stealer 数据竞争。
        auto normalized = requested_capacity < 2 ? 2 : requested_capacity;
        auto rounded    = RoundUpPowerOfTwo(normalized);
        if (rounded == 0) {
            rounded = HighestPowerOfTwoLEQ(std::numeric_limits<size_type>::max());
        }
        return rounded;
    }

    [[nodiscard]] size_type slot_index(size_type ticket) const noexcept
    {
        return ticket & _capacity_mask;
    }

    template <typename... Args>
    void write_slot(size_type index, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        if constexpr (kTrivialFastPath) {
            _slots[index] = T(std::forward<Args>(args)...);
        } else {
            std::allocator_traits<allocator_type>::construct(_allocator, _slots + index, std::forward<Args>(args)...);
        }
    }

    void destroy_slot(T* ptr) noexcept
    {
        if constexpr (!kTrivialFastPath) {
            std::allocator_traits<allocator_type>::destroy(_allocator, ptr);
        }
    }

private:
    size_type                            _capacity      = 0;
    size_type                            _capacity_mask = 0;
    T*                                   _slots         = nullptr;
    BEE_NO_UNIQUE_ADDRESS allocator_type _allocator;

    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _top    = {0};
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _bottom = {0};
};

} // namespace bee
