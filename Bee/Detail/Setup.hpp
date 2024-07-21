/**
 * @File Setup.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/27
 * @Brief 
 */

#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#  define BEE_IN_WINDOWS
#elif defined(__unix__) || defined(__unix) || defined(__linux__)
#  define BEE_IN_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
#  define BEE_IN_MAC
#else
#  error Unsupport Platform
#endif

#ifdef __has_builtin
#  define BEE_HAVE_BUILTIN(x) __has_builtin(x)
#else
#  define BEE_HAVE_BUILTIN(x) 0
#endif

// -------------------------
// 关于 Cuda、Optix 的宏定义

#if defined(__CUDA_ARCH__) || defined(__CUDACC__)
#  ifndef BEE_NOINLINE
#    define BEE_NOINLINE __attribute__((noinline))
#  endif
#  ifndef BEE_GPU_CODE
#    define BEE_GPU_CODE
#  endif
#  define BEE_GPU                __device__
#  define BEE_CPU                __host__
#  define BEE_INLINE             __forceinline__
#  define BEE_CONST              __device__ const
#  define CONST_STATIC_INIT(...) void(0) /* ignore */
#else
#  ifndef BEE_HOST_CODE
#    define BEE_HOST_CODE
#  endif
#  define BEE_GPU                /* ignore */
#  define BEE_CPU                /* ignore */
#  define BEE_CONST              const
#  define BEE_INLINE             inline
#  define CONST_STATIC_INIT(...) = __VA_ARGS__
#endif

#define BEE_CPU_GPU BEE_CPU BEE_GPU

#define BEE_FUNC      BEE_CPU_GPU BEE_INLINE
#define BEE_FUNC_DECL BEE_CPU_GPU

// Debug & Release
namespace bee {
#ifdef NDEBUG
constexpr auto kIsDebug   = false;
constexpr auto kIsRelease = true;
#  define BEE_RELEASE_BUILD
#else
constexpr auto kIsDebug   = true;
constexpr auto kIsRelease = false;
#  define BEE_DEBUG_BUILD
#endif
} // namespace bee

#include <cmath>
#ifdef max
#  undef Max
#endif
#ifdef min
#  undef Min
#endif
#ifdef isnan
#  undef IsNaN
#endif
#ifdef isinf
#  undef IsInf
#endif
#ifdef log2
#  undef Log2
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
#  define BEE_LIKELY(x)   __builtin_expect(x, 1)
#  define BEE_UNLIKELY(x) __builtin_expect(x, 0)
#else
#  define BEE_LIKELY(x)   (x)
#  define BEE_UNLIKELY(x) (x)
#endif

#include <cfloat>
#include <limits>
#include <cstdint>
#include <type_traits>

namespace bee {
// -------------------------
// 内置类型、常量别名

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8   = uint8_t;
using u16  = uint16_t;
using u32  = uint32_t;
using u64  = uint64_t;
using uint = unsigned int;

using f32 = float;
using f64 = double;

using size = std::size_t;

static constexpr i8 i8_min   = INT8_MIN;
static constexpr i16 i16_min = INT16_MIN;
static constexpr i32 i32_min = INT32_MIN;
static constexpr i64 i64_min = INT64_MIN;

static constexpr i8 i8_max   = INT8_MAX;
static constexpr i16 i16_max = INT16_MAX;
static constexpr i32 i32_max = INT32_MAX;
static constexpr i64 i64_max = INT64_MAX;

static constexpr u8 u8_max   = UINT8_MAX;
static constexpr u16 u16_max = UINT16_MAX;
static constexpr u32 u32_max = UINT32_MAX;
static constexpr u64 u64_max = UINT64_MAX;

#ifdef BEE_FLOAT_AS_DOUBLE
using Float     = f64;
using FloatBits = u64;
#else
using Float     = f32;
using FloatBits = u32;
#endif

static_assert(sizeof(Float) == sizeof(FloatBits));

// concepts
// clang-format off
template<typename T> concept BoolType = std::is_same_v<bool, T>;
template<typename T> concept U32Type  = std::is_same_v<u32, T>;
template<typename T> concept U64Type  = std::is_same_v<u64, T>;
template<typename T> concept F32Type  = std::is_same_v<f32, T>;
template<typename T> concept F64Type  = std::is_same_v<f64, T>;

template<typename T> concept SignedType     = std::is_signed_v<T>;
template<typename T> concept UnsignedType   = std::is_unsigned_v<T>;
template<typename T> concept IntegralType   = std::is_integral_v<T>;
template<typename T> concept FloatType      = std::is_floating_point_v<T>;
template<typename T> concept ArithmeticType = std::is_arithmetic_v<T>;
// clang-format on

template<typename T, typename U>
concept Convertible = (not std::is_same_v<T, U>)and std::is_convertible_v<T, U> and std::is_convertible_v<U, T>;

template<typename T, typename U>
concept BothIntegral = IntegralType<T> and IntegralType<U>;

template<ArithmeticType T>
BEE_FUNC constexpr T cast_to(ArithmeticType auto f) noexcept
    requires std::is_nothrow_convertible_v<decltype(f), T>
{
    return static_cast<T>(f);
}

} // namespace bee