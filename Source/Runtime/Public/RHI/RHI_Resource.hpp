/**
 * @File RHI_Resource.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/13
 * @Brief This file is part of Bee.
 */

#pragma once

#include "RHI/RHI_Enum.hpp"

namespace bee {

// clang-format off
#define DEFINE_RENDER_HANDLE(Type)                                                                              \
    struct Type##Handle : public RenderHandle {                                                                 \
        BEE_ALWAYS_INLINE explicit operator bool() const { return handle != 0; }                                \
        BEE_ALWAYS_INLINE Type##Handle &operator=(Type##Handle other) {                                         \
            handle = other.handle;                                                                              \
            return *this;                                                                                       \
        }                                                                                                       \
        BEE_ALWAYS_INLINE bool operator<(const Type##Handle &other) const { return handle < other.handle; }     \
        BEE_ALWAYS_INLINE bool operator==(const Type##Handle &other) const { return handle == other.handle; }   \
        BEE_ALWAYS_INLINE bool operator!=(const Type##Handle &other) const { return handle != other.handle; }   \
        BEE_ALWAYS_INLINE Type##Handle(const Type##Handle &other) : RenderHandle(other.handle) {}               \
        BEE_ALWAYS_INLINE explicit Type##Handle(u64 p_int) : RenderHandle(p_int) {}                             \
        BEE_ALWAYS_INLINE explicit Type##Handle(void *p_ptr) : RenderHandle((size_t)p_ptr) {}                   \
        BEE_ALWAYS_INLINE Type##Handle() = default;                                                             \
    };                                                                                                          \
    static_assert(sizeof(Type##Handle) == sizeof(void *));

// clang-format on


} // namespace bee