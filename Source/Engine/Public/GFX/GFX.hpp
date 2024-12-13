/**
 * @File GFX.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/27
 * @Brief This file is part of Bee.
 */

#pragma once

#include "GFX_Context.hpp"
#include "GFX_DeviceDriver.hpp"

#include <Platform/Platform.hpp>

namespace bee {

enum class RenderDeviceType : u8
{
    Unknown = 0,
    Vulkan,
        
    // ...
};

struct RenderDeviceConfig
{
    RenderDeviceType deviceType = {};
    bool headless = false;
};

struct RenderHandle
{
    std::size_t handle               = 0;
    BEE_ALWAYS_INLINE RenderHandle() = default;

    BEE_ALWAYS_INLINE RenderHandle(std::size_t h) : handle(h) { }
};

// clang-format off
#define DEFINE_RENDER_HANDLE(Type)                                                                              \
	struct Type##Handle : public RenderHandle {                                                                 \
		_ALWAYS_INLINE_ explicit operator bool() const { return handle != 0; }                                  \
		_ALWAYS_INLINE_ Type##Handle &operator=(Type##Handle other) {                                           \
			handle = other.handle;                                                                              \
			return *this;                                                                                       \
		}                                                                                                       \
		_ALWAYS_INLINE_ bool operator<(const Type##Handle &other) const { return handle < other.handle; }       \
		_ALWAYS_INLINE_ bool operator==(const Type##Handle &other) const { return handle == other.handle; }     \
		_ALWAYS_INLINE_ bool operator!=(const Type##Handle &other) const { return handle != other.handle; }     \
		_ALWAYS_INLINE_ Type##Handle(const Type##Handle &other) : RenderHandle(other.handle) {}                 \
		_ALWAYS_INLINE_ explicit Type##Handle(u64 p_int) : RenderHandle(p_int) {}                               \
		_ALWAYS_INLINE_ explicit Type##Handle(void *p_ptr) : RenderHandle((size_t)p_ptr) {}                     \
		_ALWAYS_INLINE_ Type##Handle() = default;                                                               \
	};                                                                                                          \
	static_assert(sizeof(Type##Handle) == sizeof(void *));

// clang-format on

/// ================================================================================
/// 方法
/// ================================================================================

BEE_API UniquePtr<GFX_Context> CreateRenderContext(RenderDeviceType type);

} // namespace bee