/**
 * @File Defines.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/22
 * @Brief 通用的宏定义
 */

#pragma once

#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <limits>

#include "Portable.hpp"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#ifdef isnan
#undef isnan
#endif

#ifdef isinf
#undef isinf
#endif

#ifdef log2
#undef log2
#endif

namespace bee
{
// --------------------
// 类型别名

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

inline constexpr size_t kCacheLineSize = std::hardware_constructive_interference_size;

} // namespace bee
