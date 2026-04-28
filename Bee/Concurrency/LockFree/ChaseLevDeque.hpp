/**
 * @File ChaseLevDeque.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/26
 * @Brief This file is part of Bee.
 */

#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

#include "Base/Core/Defines.hpp"
#include "Base/Numeric/Bit.hpp"
#include "Concurrency/Threading.hpp"

namespace bee
{

/**
 * @brief Bounded Chase-Lev work-stealing deque.
 *
 * The owner thread pushes and pops from bottom. Stealer threads steal from top.
 * Top and bottom are monotonic tickets; physical slots are indexed by
 * ticket & (_capacity - 1).
 *
 * Each physical slot also has a sequence number. The sequence is the authority
 * for whether a slot may be overwritten, which prevents the owner from reusing
 * a slot while a stealer that already advanced top is still moving/destructing
 * the previous object.
 *
 * Lifetime contract: callers must stop all concurrent push/pop/steal access
 * before destroying the deque.
 */
template <typename T, typename Allocator = std::allocator<T>>
class ChaseLevDeque
{
public:
    using value_type      = T;
    using size_type       = size_t;
    using allocator_type  = Allocator;
    using reference       = T&;
    using const_reference = const T&;

    explicit ChaseLevDeque(size_type capacity, const Allocator& allocator = Allocator())
        : _capacity(normalize_capacity(capacity))
        , _capacity_mask(_capacity - 1)
        , _allocator(allocator)
        , _cell_allocator(_allocator)
    {
        _cells = std::allocator_traits<cell_allocator_type>::allocate(_cell_allocator, _capacity);

        size_type constructed = 0;
        try {
            for (size_type i = 0; i < _capacity; ++i) {
                std::allocator_traits<cell_allocator_type>::construct(_cell_allocator, _cells + i, i);
                ++constructed;
            }
        } catch (...) {
            for (size_type i = 0; i < constructed; ++i) {
                std::allocator_traits<cell_allocator_type>::destroy(_cell_allocator, _cells + i);
            }
            std::allocator_traits<cell_allocator_type>::deallocate(_cell_allocator, _cells, _capacity);
            _cells = nullptr;
            throw;
        }
    }

    ~ChaseLevDeque() noexcept
    {
        if (_cells == nullptr) {
            return;
        }

        const auto top = _top.load(std::memory_order_relaxed);
        const auto bot = _bottom.load(std::memory_order_relaxed);
        for (auto ticket = top; ticket < bot; ++ticket) {
            auto& cell = cell_ref(ticket);
            if (cell.sequence.load(std::memory_order_relaxed) == ticket + 1) {
                destroy_value(cell);
            }
        }

        for (size_type i = 0; i < _capacity; ++i) {
            std::allocator_traits<cell_allocator_type>::destroy(_cell_allocator, _cells + i);
        }
        std::allocator_traits<cell_allocator_type>::deallocate(_cell_allocator, _cells, _capacity);
    }

    ChaseLevDeque(const ChaseLevDeque&)            = delete;
    ChaseLevDeque(ChaseLevDeque&&)                 = delete;
    ChaseLevDeque& operator=(const ChaseLevDeque&) = delete;
    ChaseLevDeque& operator=(ChaseLevDeque&&)      = delete;

    template <typename... Args>
    [[nodiscard]] bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        static_assert(std::is_constructible_v<T, Args&&...>, "T must be constructible with Args&&...");

        const auto bottom = _bottom.load(std::memory_order_relaxed);
        const auto top    = _top.load(std::memory_order_acquire);
        if ((bottom - top) >= capacity()) {
            return false;
        }

        auto& cell = cell_ref(bottom);
        if (cell.sequence.load(std::memory_order_acquire) != bottom) {
            return false;
        }

        write_value(cell, std::forward<Args>(args)...);
        cell.sequence.store(bottom + 1, std::memory_order_release);
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

    [[nodiscard]] bool try_pop(T& out_value) noexcept
    {
        static_assert(std::is_nothrow_move_assignable_v<T>, "T must be nothrow move assignable");

        auto bottom = _bottom.load(std::memory_order_relaxed);
        if (bottom == 0) {
            return false;
        }

        bottom -= 1;
        _bottom.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        auto top = _top.load(std::memory_order_relaxed);
        if (top > bottom) {
            _bottom.store(bottom + 1, std::memory_order_relaxed);
            return false;
        }

        if (top == bottom) {
            if (!_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                _bottom.store(bottom + 1, std::memory_order_relaxed);
                return false;
            }
            _bottom.store(bottom + 1, std::memory_order_relaxed);
        }

        auto& cell = cell_ref(bottom);
        wait_until_readable(cell, bottom);
        read_value(cell, out_value);
        release_cell(cell, bottom);
        return true;
    }

    void pop(T& out_value) noexcept
    {
        while (!try_pop(out_value)) {
            std::this_thread::yield();
        }
    }

    [[nodiscard]] bool try_steal(T& out_value) noexcept
    {
        static_assert(std::is_nothrow_move_assignable_v<T>, "T must be nothrow move assignable");

        auto top = _top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        auto bottom = _bottom.load(std::memory_order_acquire);
        if (top >= bottom) {
            return false;
        }

        if (!_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
            return false;
        }

        auto& cell = cell_ref(top);
        wait_until_readable(cell, top);
        read_value(cell, out_value);
        release_cell(cell, top);
        return true;
    }

    void steal(T& out_value) noexcept
    {
        while (!try_steal(out_value)) {
            std::this_thread::yield();
        }
    }

    [[nodiscard]] size_type capacity() const noexcept
    {
        return _capacity - 1;
    }

    [[nodiscard]] size_type physical_capacity() const noexcept
    {
        return _capacity;
    }

    [[nodiscard]] std::ptrdiff_t size_approx() const noexcept
    {
        // Approximate observation only. Stealers and the owner can change the
        // tickets immediately after these relaxed loads.
        const auto top = _top.load(std::memory_order_relaxed);
        const auto bot = _bottom.load(std::memory_order_relaxed);
        if (bot >= top) {
            return static_cast<std::ptrdiff_t>(bot - top);
        }
        return -static_cast<std::ptrdiff_t>(top - bot);
    }

    [[nodiscard]] bool is_empty() const noexcept
    {
        return size_approx() <= 0;
    }

private:
    struct alignas(BEE_CACHE_LINE_SIZE) Cell
    {
        explicit Cell(size_type initial_sequence) noexcept
            : sequence(initial_sequence)
        {
        }

        T* ptr() noexcept
        {
            return std::launder(reinterpret_cast<T*>(&storage));
        }

        std::atomic<size_type> sequence;
        alignas(T) std::byte storage[sizeof(T)];
    };

    using cell_allocator_type = std::allocator_traits<allocator_type>::template rebind_alloc<Cell>;

    static constexpr size_type normalize_capacity(size_type requested_capacity) noexcept
    {
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

    [[nodiscard]] Cell& cell_ref(size_type ticket) noexcept
    {
        return _cells[slot_index(ticket)];
    }

    template <typename... Args>
    void write_value(Cell& cell, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        std::construct_at(cell.ptr(), std::forward<Args>(args)...);
    }

    void read_value(Cell& cell, T& out_value) noexcept
    {
        out_value = std::move(*cell.ptr());
        destroy_value(cell);
    }

    void destroy_value(Cell& cell) noexcept
    {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_at(cell.ptr());
        }
    }

    void release_cell(Cell& cell, size_type ticket) noexcept
    {
        cell.sequence.store(ticket + _capacity, std::memory_order_release);
    }

    void wait_until_readable(Cell& cell, size_type ticket) noexcept
    {
        const auto readable = ticket + 1;
        while (cell.sequence.load(std::memory_order_acquire) != readable) {
            std::this_thread::yield();
        }
    }

private:
    size_type                                 _capacity      = 0;
    size_type                                 _capacity_mask = 0;
    Cell*                                     _cells         = nullptr;
    BEE_NO_UNIQUE_ADDRESS allocator_type      _allocator;
    BEE_NO_UNIQUE_ADDRESS cell_allocator_type _cell_allocator;

    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _top    = {0};
    alignas(BEE_CACHE_LINE_SIZE) std::atomic<size_type> _bottom = {0};
};

} // namespace bee
