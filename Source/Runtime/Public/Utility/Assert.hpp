/**
 * @File Assert.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/14
 * @Brief This file is part of Bee.
 */

#pragma once

#include <libassert/assert.hpp>

namespace bee {

// 做一个简单的包装

#define BEE_DEBUG_ASSERT(expr, ...) LIBASSERT_DEBUG_ASSERT(expr, __VA_ARGS__)

#define BEE_ASSERT(expr, ...) LIBASSERT_ASSERT(expr, __VA_ARGS__)

#define BEE_ASSUME(expr, ...) LIBASSERT_ASSUME(expr, __VA_ARGS__)

#define BEE_PANIC(expr, ...) LIBASSERT_PANIC(expr, __VA_ARGS__)

#define BEE_UNREACHABLE(...) LIBASSERT_UNREACHABLE(__VA_ARGS__)

#define BEE_DEBUG_ASSERT_VAL(expr, ...) LIBASSERT_DEBUG_ASSERT_VAL(expr, __VA_ARGS__)

#define BEE_ASSUME_VAL(expr, ...) LIBASSERT_ASSUME_VAL(expr, __VA_ARGS__)

#define BEE_ASSERT_VAL(expr, ...) LIBASSERT_ASSERT_VAL(expr, __VA_ARGS__)

// TODO: 添加一个断言错误处理类型，支持添加断言失败前调用

} // namespace bee