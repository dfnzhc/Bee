/**
 * @File Polynomial.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/6/1
 * @Brief 
 */

#pragma once

#include "./Common.hpp"

namespace bee {

template<typename Float, typename C>
BEE_FUNC constexpr Float EvaluatePolynomial(Float, C c)
{
    return cast_to<Float>(c);
}

template<typename Float, typename C, typename... Args>
BEE_FUNC constexpr Float EvaluatePolynomial(Float t, C c, Args... cRemaining)
{
    return Fma(t, EvaluatePolynomial(t, cRemaining...), c);
}

BEE_FUNC f32 FastExp(f32 x)
{
#ifdef BEE_GPU_CODE
    return __expf(x);
#else
    // Compute $x'$ such that $\roman{e}^x = 2^{x'}$
    f32 xp = x * 1.442695041f;

    // Find integer and fractional components of $x'$
    f32 fxp = Floor(xp), f = xp - fxp;
    int i = (int)fxp;

    // Evaluate polynomial approximation of $2^f$
    f32 twoToF = EvaluatePolynomial(f, 1.f, 0.695556856f, 0.226173572f, 0.0781455737f);

    // Scale $2^f$ by $2^i$ and return final result
    int expo = Exponent(twoToF) + i;
    if (expo < -126)
        return 0;
    if (expo > 127)
        return kInfinity;
    u32 bits  = FloatToBits(twoToF);
    bits     &= 0b10000000011111111111111111111111u;
    bits     |= static_cast<u32>(expo + 127) << 23;
    return BitsToFloat(bits);
#endif
}

/**
 * @brief (a * b) - (c * d)
 */
template<typename Ta, typename Tb, typename Tc, typename Td>
BEE_FUNC constexpr auto DifferenceOfProducts(Ta a, Tb b, Tc c, Td d)
{
    auto cd                   = c * d;
    auto differenceOfProducts = Fma(a, b, -cd);
    auto error                = Fma(-c, d, cd);
    return differenceOfProducts + error;
}

/**
 * @brief (a * b) + (c * d)
 */
template<ArithmeticType Ta, ArithmeticType Tb, ArithmeticType Tc, ArithmeticType Td>
BEE_FUNC constexpr auto SumOfProducts(Ta a, Tb b, Tc c, Td d)
{
    auto cd            = c * d;
    auto sumOfProducts = Fma(a, b, cd);
    auto error         = Fma(c, d, -cd);
    return sumOfProducts + error;
}

/**
 * @brief ax^2 + bx + c = 0
 */
template<FloatType T>
BEE_FUNC bool Quadratic(T a, T b, T c, T* t0, T* t1)
{
    auto disc = DifferenceOfProducts(b, b, 4 * a, c);
    if (disc < 0)
        return false;

    auto rootDisc = Sqrt(disc);
    if (rootDisc == 0) {
        // Both roots are equal
        *t0 = *t1 = -c / b;
        return true;
    }

    auto q = T(-0.5) * (b + CopySign(rootDisc, b));
    *t0    = q / a;
    *t1    = c / q;
    if (*t0 > *t1)
        Swap(*t0, *t1);
    return true;
}

} // namespace bee