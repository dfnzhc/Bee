/**
 * @File MoveOnlyFunction.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/10
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Core/Defines.hpp"
#include "Base/Core/Traits.hpp"
#include "Base/Diagnostics/Check.hpp"

#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace bee
{

// =============================================================================
// MoveOnlyFunction 仅可移动（move‑only）的类型擦除函数包装器
// =============================================================================

template <typename T>
class MoveOnlyFunction
{
    static_assert(AlwaysFalse<T>, "MoveOnlyFunction only accepts function types as template arguments.");
};

template <typename R, typename... Args>
class MoveOnlyFunction<R(Args...)>
{
public:
    MoveOnlyFunction() = default;

    template <typename Fn>
        requires(!std::is_same_v<std::decay_t<Fn>, MoveOnlyFunction>) // 排除自身类型，避免递归
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

    // 类型擦除，将每个被包装的可调用类型 Fn 实例化一组静态的 VTable 常量
    // 所有相同 Fn 类型的 MoveOnlyFunction 实例共享同一份 vtable，节省内存。
    struct VTable
    {
        R (*call)(void*, Args...);
        void (*move_construct)(void* dst, void* src);
        void (*destroy)(void*);
    };

    template <typename Fn>
    static auto inline_vtable() -> const VTable&
    {
        static const VTable vt{
                [](void* p, Args... args) -> R {
                    return (*static_cast<Fn*>(p))(std::forward<Args>(args)...);
                },
                [](void* dst, void* src) {
                    // 原位移动构造并析构
                    new(dst) Fn(std::move(*static_cast<Fn*>(src)));
                    static_cast<Fn*>(src)->~Fn();
                },
                [](void* p) {
                    // 仅析构对象，不释放内存
                    static_cast<Fn*>(p)->~Fn();
                }};
        return vt;
    }

    template <typename Fn>
    static auto heap_vtable() -> const VTable&
    {
        static const VTable vt{
                [](void* p, Args... args) -> R {
                    return (*(*static_cast<Fn**>(p)))(std::forward<Args>(args)...);
                },
                [](void* dst, void* src) {
                    // 移动指针即可
                    *static_cast<Fn**>(dst) = *static_cast<Fn**>(src);
                    *static_cast<Fn**>(src) = nullptr;
                },
                [](void* p) {
                    // 释放堆对象，再将存储的指针置空
                    delete *static_cast<Fn**>(p);
                    *static_cast<Fn**>(p) = nullptr;
                }};
        return vt;
    }

    template <typename Fn>
    static constexpr auto fits_inline() -> bool
    {
        return sizeof(Fn) <= kInlineStorageSize && alignof(Fn) <= kInlineStorageAlign && std::is_nothrow_move_constructible_v<Fn>;
    }

    template <typename Fn, typename... CtorArgs>
    void emplace(CtorArgs&&... args)
    {
        if constexpr (fits_inline<Fn>()) {
            // 满足小对象优化，在 inline 空间创建对象并设置 vtable
            try {
                new(&inline_storage_) Fn(std::forward<CtorArgs>(args)...);
            } catch (...) {
                vtable_ = nullptr;
                throw;
            }
            vtable_ = &inline_vtable<Fn>();
        } else {
            // 在堆上创建对象并设置 vtable
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
