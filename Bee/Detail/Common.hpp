/**
 * @File common.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/27
 * @Brief 
 */

#pragma once

#include "./Setup.hpp"
#include "./Check.hpp"
#include "./Constants.hpp"

#ifdef BEE_HOST_CODE
#  include <cstring>
#endif

namespace bee {

template<typename T>
BEE_FUNC void Swap(T& a, T& b)
{
#ifdef BEE_GPU_CODE
    ::swap(a, b);
#else
    return std::swap(a, b);
#endif
}

BEE_FUNC void* Memcpy(void* dst, const void* src, size sz)
{
#ifdef BEE_GPU_CODE
    return ::memcpy(dst, src, sz);
#else
    return std::memcpy(dst, src, sz);
#endif
}

BEE_FUNC int Memcmp(const void* buf1, const void* buf2, size sz)
{
#ifdef BEE_GPU_CODE
    return ::memcmp(buf1, buf2, sz);
#else
    return std::memcmp(buf1, buf2, sz);
#endif
}

BEE_FUNC void* Memset(void* dst, int val, size sz)
{
#ifdef BEE_GPU_CODE
    return ::memset(dst, val, sz);
#else
    return std::memset(dst, val, sz);
#endif
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wc++20-extensions"
template<class To, class From>
    requires requires {
        sizeof(To) == sizeof(From);
        std::is_trivially_copyable_v<From>;
        std::is_trivially_copyable_v<To>;
        std::is_trivially_constructible_v<To>;
    }
BEE_FUNC auto BitCast(const From& src) noexcept
{
    auto dst = To(0);
    Memcpy(&dst, &src, sizeof(To));
    return dst;
}
#pragma clang diagnostic pop

// clang-format off
template<ArithmeticType T> BEE_FUNC constexpr bool Equal(T x, T y)      noexcept { return x == y; }
template<ArithmeticType T> BEE_FUNC constexpr bool NotEqual(T x, T y)   noexcept { return not Equal(x, y); }

template<ArithmeticType T> BEE_FUNC constexpr T Min(T x, T y)           noexcept { return x < y ? x : y; }
template<ArithmeticType T> BEE_FUNC constexpr T Min(T x, T y, T z)      noexcept { return Min(x, Min(y, z)); }
template<ArithmeticType T> BEE_FUNC constexpr T Min(T x, T y, T z, T w) noexcept { return Min(x, Min(y, Min(z, w))); }

template<ArithmeticType T> BEE_FUNC constexpr T Max(T x, T y)           noexcept { return x > y ? x : y; }
template<ArithmeticType T> BEE_FUNC constexpr T Max(T x, T y, T z)      noexcept { return Max(x, Max(y, z)); }
template<ArithmeticType T> BEE_FUNC constexpr T Max(T x, T y, T z, T w) noexcept { return Max(x, Max(y, Max(z, w))); }

template<ArithmeticType T> BEE_FUNC constexpr T Clamp(T v, T lo, T hi) noexcept { return Max(lo, Min(v, hi)); }
template<ArithmeticType T> BEE_FUNC constexpr T Clamp(T v, T hi)       noexcept { return Max(T(0), Min(v, hi)); }

template<SignedType T>     BEE_FUNC constexpr T Abs(T x)               noexcept { return x > 0 ? x : -x; }
template<SignedType T>     BEE_FUNC constexpr T Sign(T x)              noexcept { return x < T(0) ? T(-1) : (x > T(0) ? T(1) : T(0)); }
// clang-format on

template<FloatType T>
BEE_FUNC constexpr T Floor(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::floorf(x);
    else
        return ::floor(x);
#else
    return std::floor(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Ceil(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::ceilf(x);
    else
        return ::ceil(x);
#else
    return std::ceil(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Trunc(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::truncf(x);
    else
        return ::trunc(x);
#else
    return std::trunc(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Round(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::roundf(x);
    else
        return ::round(x);
#else
    return std::round(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Sqrt(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::sqrtf(x);
    else
        return ::sqrt(x);
#else
    return std::sqrt(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T rSqrt(T x)
{
    return One<T>() / Sqrt(x);
}

template<FloatType T>
BEE_FUNC constexpr T Exp(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::expf(x);
    else
        return ::exp(x);
#else
    return std::exp(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Exp2(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::exp2f(x);
    else
        return ::exp2(x);
#else
    return std::exp2(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Log(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::logf(x);
    else
        return ::log(x);
#else
    return std::log(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Log2(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::log2f(x);
    else
        return ::log2(x);
#else
    return std::log2(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Log10(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::log10f(x);
    else
        return ::log10(x);
#else
    return std::log10(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Sin(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::sinf(x);
    else
        return ::sin(x);
#else
    return std::sin(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Sinh(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::sinhf(x);
    else
        return ::sinh(x);
#else
    return std::sinh(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Cos(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::cosf(x);
    else
        return ::cos(x);
#else
    return std::cos(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Cosh(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::coshf(x);
    else
        return ::cosh(x);
#else
    return std::cosh(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Tan(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::tanf(x);
    else
        return ::tan(x);
#else
    return std::tan(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Tanh(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::tanhf(x);
    else
        return ::tanh(x);
#else
    return std::tanh(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T aSin(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::asinf(x);
    else
        return ::asin(x);
#else
    return std::asin(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T aSinh(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::asinhf(x);
    else
        return ::asinh(x);
#else
    return std::asinh(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T aCos(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::acosf(x);
    else
        return ::acos(x);
#else
    return std::acos(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T aCosh(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::acoshf(x);
    else
        return ::acosh(x);
#else
    return std::acosh(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T aTan(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::atanf(x);
    else
        return ::atan(x);
#else
    return std::atan(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T aTanh(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::atanhf(x);
    else
        return ::atanh(x);
#else
    return std::atanh(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T aTan2(T y, T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::atan2f(y, x);
    else
        return ::atan2(y, x);
#else
    return std::atan2(y, x);
#endif
}

} // namespace bee

#include "FloatingPoint.hpp"

namespace bee {

template<FloatType T>
BEE_FUNC constexpr T Erf(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::erff(x);
    else
        return ::erf(x);
#else
    return std::erf(x);
#endif
}

namespace internal {

// clang-format off
template<FloatType T> BEE_FUNC constexpr T ErfInv(T x) noexcept
{
    // https://stackoverflow.com/a/49743348
    T p;
    T t = Log(Max(Fma(x, -x, 1), cast_to<T>(kFloatMin)));
    BEE_CHECK(!IsNaN(t) && !IsInf(t));
    if (Abs(t) > 6.125f) {              // maximum ulp error = 2.35793
        p = 3.03697567e-10f;            //  0x1.4deb44p-32
        p = Fma(p, t, cast_to<T>(2.93243101e-8f));  //  0x1.f7c9aep-26
        p = Fma(p, t, cast_to<T>(1.22150334e-6f));  //  0x1.47e512p-20
        p = Fma(p, t, cast_to<T>(2.84108955e-5f));  //  0x1.dca7dep-16
        p = Fma(p, t, cast_to<T>(3.93552968e-4f));  //  0x1.9cab92p-12
        p = Fma(p, t, cast_to<T>(3.02698812e-3f));  //  0x1.8cc0dep-9
        p = Fma(p, t, cast_to<T>(4.83185798e-3f));  //  0x1.3ca920p-8
        p = Fma(p, t, cast_to<T>(-2.64646143e-1f)); // -0x1.0eff66p-2
        p = Fma(p, t, cast_to<T>(8.40016484e-1f));  //  0x1.ae16a4p-1
    }
    else {                              // maximum ulp error = 2.35456
        p = 5.43877832e-9f;             //  0x1.75c000p-28
        p = Fma(p, t, cast_to<T>(1.43286059e-7f));  //  0x1.33b458p-23
        p = Fma(p, t, cast_to<T>(1.22775396e-6f));  //  0x1.49929cp-20
        p = Fma(p, t, cast_to<T>(1.12962631e-7f));  //  0x1.e52bbap-24
        p = Fma(p, t, cast_to<T>(-5.61531961e-5f)); // -0x1.d70c12p-15
        p = Fma(p, t, cast_to<T>(-1.47697705e-4f)); // -0x1.35be9ap-13
        p = Fma(p, t, cast_to<T>(2.31468701e-3f));  //  0x1.2f6402p-9
        p = Fma(p, t, cast_to<T>(1.15392562e-2f));  //  0x1.7a1e4cp-7
        p = Fma(p, t, cast_to<T>(-2.32015476e-1f)); // -0x1.db2aeep-3
        p = Fma(p, t, cast_to<T>(8.86226892e-1f));  //  0x1.c5bf88p-1
    }
    return x * p;
}
// clang-format on

} // namespace internal

template<FloatType T>
BEE_FUNC constexpr T ErfInv(T x) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::erfinvf(x);
    else
        return ::erfinv(x);
#else
    return internal::ErfInv(x);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T CopySign(T num, T sign) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::copysignf(num, sign);
    else
        return ::copysign(num, Sign);
#else
    return std::copysign(num, sign);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T fMod(T x, T y) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::fmodf(x, y);
    else
        return ::fmod(x, y);
#else
    return std::fmod(x, y);
#endif
}

template<FloatType T>
BEE_FUNC constexpr T Modf(T x, T* iptr) noexcept
{
#ifdef BEE_GPU_CODE
    if constexpr (std::is_same_v<T, f32>)
        return ::modff(x, iptr);
    else
        return ::modf(x, iptr);
#else
    return std::modf(x, iptr);
#endif
}

template<FloatType T>
BEE_FUNC constexpr auto Radians(T x) noexcept
{
    return x * decltype(x)(0.01745329251994329576923690768489);
}

template<FloatType T>
BEE_FUNC constexpr auto Degrees(T x) noexcept
{
    return x * decltype(x)(57.295779513082320876798154814105);
}

template<FloatType T>
BEE_FUNC constexpr auto Frac(T x) noexcept
{
    if (x >= 0)
        return x - Floor(x);

    T ip;
    return Modf(x, &ip);
}

template<FloatType T>
BEE_FUNC constexpr auto Rcp(T x) noexcept
{
    BEE_CHECK(x != Zero<T>());

    return One<T>() / x;
}

template<FloatType T>
BEE_FUNC constexpr auto Saturate(T x) noexcept
{
    return Max(Zero<T>(), Min(One<T>(), x));
}

template<FloatType T>
BEE_FUNC constexpr auto Step(T x, T edge) noexcept
{
    return edge < x ? Zero<T>() : One<T>();
}

template<FloatType T>
BEE_FUNC constexpr auto Lerp(T x, T y, T s) noexcept
{
    return (One<T>() - s) * x + s * y;
}

template<FloatType T>
BEE_FUNC constexpr auto BiLerp(T v00, T v01, T v10, T v11, T u, T v) noexcept
{
    return Lerp(Lerp(v00, v01, u), Lerp(v10, v11, u), v);
}

template<FloatType T>
BEE_FUNC constexpr auto SmoothStep(T v, T lo, T hi) noexcept
{
    if (lo > hi)
        Swap(lo, hi);

    v = Saturate((v - lo) / (hi - lo));
    return v * v * (T(3) - T(2) * v);
}

template<FloatType T>
BEE_FUNC constexpr T Length(T v)
{
    return Abs(v);
}

template<FloatType T>
BEE_FUNC constexpr T Distance(T p0, T p1)
{
    return Length(p1 - p0);
}

template<FloatType T>
BEE_FUNC constexpr T Dot(T x, T y)
{
    return x * y;
}

template<FloatType T>
BEE_FUNC T FaceForward(T N, T I, T refN)
{
    return Dot(refN, I) < Zero<T>() ? N : -N;
}

template<ArithmeticType T>
BEE_FUNC bool Approx(T x, T y, T epsilon = kShadowEpsilon)
{
    using Type = T;
    return Abs(Type(x) - Type(y)) < epsilon;
}

template<ArithmeticType T, ArithmeticType U>
BEE_FUNC bool Approx(T x, U y, std::common_type_t<T, U> epsilon = kShadowEpsilon)
{
    using Type = std::common_type_t<T, U>;
    return Approx(Type(x), Type(y), Type(epsilon));
}

template<FloatType T>
BEE_FUNC T SafeSqrt(T x)
{
    BEE_CHECK_GE(x, -1e-3); // not too negative
    return Sqrt(Max(T(0), x));
}

template<typename T>
BEE_FUNC constexpr T Sqr(T v)
{
    return v * v;
}

template<FloatType T>
BEE_FUNC T SinXOverX(T x)
{
    if (1 + x * x == 1)
        return 1;
    return Sin(x) / x;
}

template<FloatType T>
BEE_FUNC T SafeASin(T x)
{
    BEE_CHECK(x >= -1.0001 && x <= 1.0001);
    return aSin(Clamp(x, -One<T>(), One<T>()));
}

template<FloatType T>
BEE_FUNC T SafeACos(T x)
{
    BEE_CHECK(x >= -1.0001 && x <= 1.0001);
    return aCos(Clamp(x, -One<T>(), One<T>()));
}

// clang-format off
template<ArithmeticType T> BEE_FUNC constexpr T Gauss(T x, T expectedValue, T standardDeviation)
{
    return Exp(-((x - expectedValue) * (x - expectedValue)) / (Two<T>() * standardDeviation * standardDeviation)) / (standardDeviation * Sqrt(cast_to<T>(kTwoPi)));
}
// clang-format on

BEE_FUNC constexpr Float Gamma(int n)
{
    return ((Float)n * kMachineEpsilon) / (1 - (Float)n * kMachineEpsilon);
}

} // namespace bee