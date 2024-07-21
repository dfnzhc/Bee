/**
 * @File check.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/27
 * @Brief 
 */

#pragma once

#include "./Setup.hpp"

namespace bee {

#ifdef BEE_DEBUG_BUILD
#  include <cassert>
#  define BEE_CHECK_IMPL(x)      assert(x)
#  define BEE_CHECK_OP(a, b, op) assert((a)op(b))
#else
#  define BEE_CHECK_IMPL(x)      void(0)/* ignore */
#  define BEE_CHECK_OP(a, b, op) void(0)/* ignore */
#endif

#define BEE_CHECK(x)       BEE_CHECK_IMPL(x)
#define BEE_CHECK_EQ(a, b) BEE_CHECK_OP(a, b, ==)
#define BEE_CHECK_NE(a, b) BEE_CHECK_OP(a, b, !=)
#define BEE_CHECK_GE(a, b) BEE_CHECK_OP(a, b, >=)
#define BEE_CHECK_GT(a, b) BEE_CHECK_OP(a, b, >)
#define BEE_CHECK_LE(a, b) BEE_CHECK_OP(a, b, <=)
#define BEE_CHECK_LT(a, b) BEE_CHECK_OP(a, b, <)

} // namespace bee