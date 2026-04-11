/**
 * @File MoveOnlyFunction.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Core/Defines.hpp"
#include "Base/Diagnostics/Check.hpp"

#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace bee
{

// =============================================================================
// MoveOnlyFunction<R(Args...)> — 类型擦除、仅移动的可调用包装器，支持小缓冲区优化（SBO）
// =============================================================================

template<typename>
class MoveOnlyFunction; // 主模板，未定义

template<typename R, typename... Args>
class MoveOnlyFunction<R(Args...)>
{
public:
    MoveOnlyFunction() = default;

    template<typename Fn>
        requires(!std::is_same_v<std::decay_t<Fn>, MoveOnlyFunction>)
    MoveOnlyFunction(Fn&& fn)
    {
        emplace<std::decay_t<Fn>>(std::forward<Fn>(fn));
    }

    MoveOnlyFunction(MoveOnlyFunction&& other) noexcept
    {
        move_from(std::move(other));
    }

    auto operator=(MoveOnlyFunction&& other) noexcept -> MoveOnlyFunction&
    {
        if (this != &other) {
            reset();
            move_from(std::move(other));
        }
        return *this;
    }

    MoveOnlyFunction(const MoveOnlyFunction&)                    = delete;
    auto operator=(const MoveOnlyFunction&) -> MoveOnlyFunction& = delete;

    ~MoveOnlyFunction()
    {
        reset();
    }

    auto operator()(Args... args) -> R
    {
        BEE_ASSERT(vtable_ != nullptr);
        return vtable_->call(storage_ptr(), std::forward<Args>(args)...);
    }

    explicit operator bool() const noexcept
    {
        return vtable_ != nullptr;
    }

private:
    static constexpr std::size_t kInlineStorageSize  = 128;
    static constexpr std::size_t kInlineStorageAlign = alignof(std::max_align_t);

    struct VTable
    {
        R (*call)(void*, Args...);
        void (*move_construct)(void* dst, void* src);
        void (*destroy)(void*);
    };

    template<typename Fn>
    static auto inline_vtable() -> const VTable&
    {
        static const VTable vt{
            [](void* p, Args... args) -> R {
                return (*static_cast<Fn*>(p))(std::forward<Args>(args)...);
            },
            [](void* dst, void* src) {
                new (dst) Fn(std::move(*static_cast<Fn*>(src)));
                static_cast<Fn*>(src)->~Fn();
            },
            [](void* p) { static_cast<Fn*>(p)->~Fn(); }};
        return vt;
    }

    template<typename Fn>
    static auto heap_vtable() -> const VTable&
    {
        static const VTable vt{
            [](void* p, Args... args) -> R {
                return (*(*static_cast<Fn**>(p)))(std::forward<Args>(args)...);
            },
            [](void* dst, void* src) {
                *static_cast<Fn**>(dst) = *static_cast<Fn**>(src);
                *static_cast<Fn**>(src) = nullptr;
            },
            [](void* p) {
                delete *static_cast<Fn**>(p);
                *static_cast<Fn**>(p) = nullptr;
            }};
        return vt;
    }

    template<typename Fn>
    static constexpr auto fits_inline() -> bool
    {
        return sizeof(Fn) <= kInlineStorageSize && alignof(Fn) <= kInlineStorageAlign
            && std::is_nothrow_move_constructible_v<Fn>;
    }

    template<typename Fn, typename... CtorArgs>
    void emplace(CtorArgs&&... args)
    {
        if constexpr (fits_inline<Fn>()) {
            try {
                new (&inline_storage_) Fn(std::forward<CtorArgs>(args)...);
            } catch (...) {
                vtable_ = nullptr;
                throw;
            }
            vtable_ = &inline_vtable<Fn>();
        } else {
            try {
                *reinterpret_cast<Fn**>(&inline_storage_) = new Fn(std::forward<CtorArgs>(args)...);
            } catch (...) {
                vtable_ = nullptr;
                throw;
            }
            vtable_ = &heap_vtable<Fn>();
        }
    }

    void reset() noexcept
    {
        if (vtable_ != nullptr) {
            vtable_->destroy(storage_ptr());
            vtable_ = nullptr;
        }
    }

    void move_from(MoveOnlyFunction&& other) noexcept
    {
        if (other.vtable_ == nullptr) {
            return;
        }
        vtable_ = other.vtable_;
        vtable_->move_construct(storage_ptr(), other.storage_ptr());
        other.vtable_ = nullptr;
    }

    auto storage_ptr() noexcept -> void*
    {
        return static_cast<void*>(&inline_storage_);
    }

    alignas(kInlineStorageAlign) std::byte inline_storage_[kInlineStorageSize]{};
    const VTable* vtable_{nullptr};
};

} // namespace bee
