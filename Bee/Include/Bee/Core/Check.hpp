/**
 * @File Check.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Log.hpp"

#ifdef BEE_HOST_CODE
#include <sstream>
#endif

namespace bee
{
namespace detail
{
    #ifndef BEE_GPU_CODE
    template <typename T>
    void HostLogValue(std::ostream& os, const T& val)
    {
        os << val;
    }

    inline void HostLogValue(std::ostream& os, bool val)
    {
        os << (val ? "true" : "false");
    }
    #endif

    // clang-format off
    BEE_FUNC void DevicePrintVal(int v)                 { printf("%d", v); }
    BEE_FUNC void DevicePrintVal(unsigned int v)        { printf("%u", v); }
    BEE_FUNC void DevicePrintVal(long v)                { printf("%ld", v); }
    BEE_FUNC void DevicePrintVal(unsigned long v)       { printf("%lu", v); }
    BEE_FUNC void DevicePrintVal(long long v)           { printf("%lld", v); }
    BEE_FUNC void DevicePrintVal(unsigned long long v)  { printf("%llu", v); }
    BEE_FUNC void DevicePrintVal(float v)               { printf("%f", v); }
    BEE_FUNC void DevicePrintVal(double v)              { printf("%f", v); }
    BEE_FUNC void DevicePrintVal(bool v)                { printf(v ? "true" : "false"); }
    BEE_FUNC void DevicePrintVal(const char* v)         { printf("%s", v); }

    template <typename T> BEE_FUNC void DevicePrintVal(const T&) { printf("??"); }
    // clang-format on

    BEE_FUNC void CheckFail(const char* file, int line, const char* func, const char* pred)
    {
        #ifdef BEE_GPU_CODE
        printf("\n[CUDA Check Failed]\n\tExpr: %s\n\tFile: %s:%d\n\tFunc: %s\n", pred, file, line, func);
        #else
        BEE_FATAL("[Check failed] {} (in {})", pred, func);
        #endif
        BEE_DEBUG_BREAK();
    }

    template <typename T1, typename T2>
    BEE_FUNC void CheckOpFail(const char* file, int line, const char* func, const char* expr1, const char* expr2,
                              const char* op_str, const T1& v1, const T2& v2)
    {
        #ifdef BEE_GPU_CODE
        printf("\n[CUDA Check Failed]\n\tExpr: %s %s %s\n\tFile: %s:%d\n\tFunc: %s\n\tValues: ", expr1, op_str, expr2, file, line, func);
        DevicePrintVal(v1);
        printf(" vs ");
        DevicePrintVal(v2);
        printf("\n");
        #else
        BEE_FATAL("[Check failed]: {} {} {} (Values: {} vs {}) (in {})", expr1, op_str, expr2, v1, v2, func);
        #endif

        BEE_DEBUG_BREAK();
    }

} // namespace detail

#define BEE_CHECK(x)                                                        \
    do {                                                                    \
        if (BEE_UNLIKELY(!(x))) {                                           \
            bee::detail::CheckFail(__FILE__, __LINE__, __func__, #x);       \
        }                                                                   \
    } while (0)

#define BEE_CHECK_OP_IMPL(val1, val2, op, op_str)                                                       \
    do {                                                                                                \
        auto const& _v1 = (val1);                                                                       \
        auto const& _v2 = (val2);                                                                       \
        if (BEE_UNLIKELY(!(_v1 op _v2))) {                                                              \
            bee::detail::CheckOpFail(__FILE__, __LINE__, __func__, #val1, #val2, op_str, _v1, _v2);     \
        }                                                                                               \
    } while (0)

#define BEE_CHECK_EQ(a, b) BEE_CHECK_OP_IMPL(a, b, ==, "==")
#define BEE_CHECK_NE(a, b) BEE_CHECK_OP_IMPL(a, b, !=, "!=")
#define BEE_CHECK_GT(a, b) BEE_CHECK_OP_IMPL(a, b, >,  ">" )
#define BEE_CHECK_GE(a, b) BEE_CHECK_OP_IMPL(a, b, >=, ">=")
#define BEE_CHECK_LT(a, b) BEE_CHECK_OP_IMPL(a, b, <,  "<" )
#define BEE_CHECK_LE(a, b) BEE_CHECK_OP_IMPL(a, b, <=, "<=")

#define BEE_CHECK_NEAR(val1, val2, tol)                                                                 \
    do {                                                                                                \
        auto const& _v1 = (val1);                                                                       \
        auto const& _v2 = (val2);                                                                       \
        auto const& _tol = (tol);                                                                       \
        if (BEE_UNLIKELY(std::abs(_v1 - _v2) > _tol)) {                                                 \
            bee::detail::CheckOpFail(__FILE__, __LINE__, __func__, #val1, #val2, "near", _v1, _v2);     \
        }                                                                                               \
    } while (0)

#if BEE_DEBUG_BUILD

#define BEE_DCHECK(x)            BEE_CHECK(x)
#define BEE_DCHECK_EQ(a, b)      BEE_CHECK_EQ(a, b)
#define BEE_DCHECK_NE(a, b)      BEE_CHECK_NE(a, b)
#define BEE_DCHECK_GT(a, b)      BEE_CHECK_GT(a, b)
#define BEE_DCHECK_GE(a, b)      BEE_CHECK_GE(a, b)
#define BEE_DCHECK_LT(a, b)      BEE_CHECK_LT(a, b)
#define BEE_DCHECK_LE(a, b)      BEE_CHECK_LE(a, b)
#define BEE_DCHECK_NEAR(a, b, t) BEE_CHECK_NEAR(a, b, t)

#else

#define BEE_UNUSED(...) do { (void)sizeof(__VA_ARGS__); } while(0)

#define BEE_DCHECK(x)            BEE_UNUSED(x)
#define BEE_DCHECK_EQ(a, b)      BEE_UNUSED(a, b)
#define BEE_DCHECK_NE(a, b)      BEE_UNUSED(a, b)
#define BEE_DCHECK_GT(a, b)      BEE_UNUSED(a, b)
#define BEE_DCHECK_GE(a, b)      BEE_UNUSED(a, b)
#define BEE_DCHECK_LT(a, b)      BEE_UNUSED(a, b)
#define BEE_DCHECK_LE(a, b)      BEE_UNUSED(a, b)
#define BEE_DCHECK_NEAR(a, b, t) BEE_UNUSED(a, b, t)

#endif

} // namespace bee
