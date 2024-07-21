/**
 * @File FloatingPoint.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/27
 * @Brief 
 */

#pragma once

#include <concepts>
#include <numbers>
#include <numeric>
#include <cmath>
#include "./Common.hpp"

namespace bee {

BEE_FUNC bool IsNaN(std::floating_point auto v)
{
#ifdef BEE_GPU_CODE
    return ::isnan(v);
#else
    return std::isnan(v);
#endif
}

BEE_FUNC bool IsNaN(auto)
{
    return false;
}

BEE_FUNC bool IsInf(std::floating_point auto v)
{
#ifdef BEE_GPU_CODE
    return ::isinf(v);
#else
    return std::isinf(v);
#endif
}

BEE_FUNC bool IsInf(auto)
{
    return false;
}

BEE_FUNC bool IsFinite(std::floating_point auto v)
{
#ifdef BEE_GPU_CODE
    return ::isfinite(v);
#else
    return std::isfinite(v);
#endif
}

BEE_FUNC bool IsFinite(auto)
{
    return true;
}


BEE_FUNC auto Fma(f32 a, f32 b, f32 c)
{
#ifdef BEE_GPU_CODE
    return ::fmaf(a, b, c);
#else
    return std::fmaf(a, b, c);
#endif
}

BEE_FUNC auto Fma(f64 a, f64 b, f64 c)
{
#ifdef BEE_GPU_CODE
    return ::fma(a, b, c);
#else
    return std::fma(a, b, c);
#endif
}

BEE_FUNC auto FloatToBits(std::floating_point auto f)
{
    if constexpr (std::is_same_v<decltype(f), f32>) {
#ifdef BEE_GPU_CODE
        return __float_as_uint(f);
#else
        return BitCast<u32>(f);
#endif
    }
    else {
#ifdef BEE_GPU_CODE
        return __double_as_longlong(f);
#else
        return BitCast<u64>(f);
#endif
    }
}

BEE_FUNC auto BitsToFloat(std::unsigned_integral auto ui)
{
    if constexpr (std::is_same_v<decltype(ui), u32>) {
#ifdef BEE_GPU_CODE
        return __uint_as_float(ui);
#else
        return BitCast<f32>(ui);
#endif
    }
    else {
#ifdef BEE_GPU_CODE
        return __longlong_as_double(ui);
#else
        return BitCast<f64>(ui);
#endif
    }
}

BEE_FUNC i32 Exponent(std::floating_point auto v)
{
    if constexpr (std::is_same_v<decltype(v), f32>) {
        return cast_to<i32>(FloatToBits(v) >> 23) - 127;
    }
    else {
        return cast_to<i32>(FloatToBits(v) >> 52) - 1'023;
    }
}

BEE_FUNC u32 Significand(f32 v)
{
    return cast_to<u32>(FloatToBits(v)) & u32((1 << 23) - 1);
}

BEE_FUNC u64 Significand(f64 v)
{
    return cast_to<u64>(FloatToBits(v)) & u64((1ull << 52) - 1);
}

BEE_FUNC u32 SignBit(f32 v)
{
    return FloatToBits(v) & (u32)0x80000000;
}

BEE_FUNC u64 SignBit(f64 v)
{
    return FloatToBits(v) & (u64)0x8000000000000000;
}

BEE_FUNC auto NextFloatUp(std::floating_point auto v)
{
    if (IsInf(v) && v > 0)
        return v;

    if (v == -0.)
        v = 0.f;

    auto ui = FloatToBits(v);
    return BitsToFloat(v >= 0 ? ++ui : --ui);
}

BEE_FUNC auto NextFloatDown(std::floating_point auto v)
{
    if (IsInf(v) && v < 0)
        return v;
    if (v == 0.f)
        v = -0.f;

    auto ui = FloatToBits(v);
    return BitsToFloat(v >= 0 ? --ui : ++ui);
}

BEE_FUNC Float AddRoundUp(Float a, Float b)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __dadd_ru(a, b);
#  else
    return __fadd_ru(a, b);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatUp(a + b);
#endif
}

BEE_FUNC Float AddRoundDown(Float a, Float b)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __dadd_rd(a, b);
#  else
    return __fadd_rd(a, b);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatDown(a + b);
#endif
}

BEE_FUNC Float SubRoundUp(Float a, Float b)
{
    return AddRoundUp(a, -b);
}

BEE_FUNC Float SubRoundDown(Float a, Float b)
{
    return AddRoundDown(a, -b);
}

BEE_FUNC Float MulRoundUp(Float a, Float b)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __dmul_ru(a, b);
#  else
    return __fmul_ru(a, b);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatUp(a * b);
#endif
}

BEE_FUNC Float MulRoundDown(Float a, Float b)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __dmul_rd(a, b);
#  else
    return __fmul_rd(a, b);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatDown(a * b);
#endif
}

BEE_FUNC Float DivRoundUp(Float a, Float b)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __ddiv_ru(a, b);
#  else
    return __fdiv_ru(a, b);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatUp(a / b);
#endif
}

BEE_FUNC Float DivRoundDown(Float a, Float b)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __ddiv_rd(a, b);
#  else
    return __fdiv_rd(a, b);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatDown(a / b);
#endif
}

BEE_FUNC Float RcpRoundUp(Float a)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __drcp_ru(a);
#  else
    return __frcp_ru(a);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatUp(Float(1) / a);
#endif
}

BEE_FUNC Float RcpRoundDown(Float a)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __drcp_rd(a);
#  else
    return __frcp_rd(a);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatDown(Float(1) / a);
#endif
}

BEE_FUNC Float SqrtRoundUp(Float a)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __dsqrt_ru(a);
#  else
    return __fsqrt_ru(a);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatUp(Sqrt(a));
#endif
}

BEE_FUNC Float SqrtRoundDown(Float a)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __dsqrt_rd(a);
#  else
    return __fsqrt_rd(a);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return std::max<Float>(0, NextFloatDown(Sqrt(a)));
#endif
}

BEE_FUNC Float FmaRoundUp(Float a, Float b, Float c)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __fma_ru(a, b, c);
#  else
    return __fmaf_ru(a, b, c);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatUp(Fma(a, b, c));
#endif
}

BEE_FUNC Float FmaRoundDown(Float a, Float b, Float c)
{
#ifdef BEE_GPU_CODE
#  ifdef BEE_FLOAT_AS_DOUBLE
    return __fma_rd(a, b, c);
#  else
    return __fmaf_rd(a, b, c);
#  endif // BEE_FLOAT_AS_DOUBLE
#else    // BEE_GPU_CODE
    return NextFloatDown(Fma(a, b, c));
#endif
}

} // namespace bee