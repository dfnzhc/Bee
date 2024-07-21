/**
 * @File bit.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/5/27
 * @Brief 
 */

#pragma once

#include "./Setup.hpp"
#include "./Check.hpp"
#include "./FloatingPoint.hpp"

namespace bee {

/**
 * @brief 计算32位浮点数的近似平方根。
 *
 * @param x0 要计算平方根的32位浮点数。
 * @return 输入值的近似平方根。
 * @note 该函数假定x0是非负数。如果x为负数，则行为未定义。
 */
BEE_FUNC constexpr f32 aSqrt(f32 x0)
{
    BEE_CHECK(x0 >= 0);

    union
    {
        i32 ix;
        f32 x;
    } u{};

    u.x  = x0;                       // x可以被视为i32。
    u.ix = 0x1fbb3f80 + (u.ix >> 1); // 初始猜测。
    u.x  = 0.5f * (u.x + x0 / u.x);  // 牛顿步骤。
    u.x  = 0.5f * (u.x + x0 / u.x);  // 再次牛顿步骤。
    return u.x;
}

/**
 * @brief 计算32位浮点数的近似立方根。
 *
 * @param x0 要计算立方根的32位浮点数。
 * @return 输入值的近似立方根。
 * @note 该函数假定x0是非负数。如果x为负数，则行为未定义。
 */
BEE_FUNC constexpr f32 aCbrt(f32 x0)
{
    BEE_CHECK(x0 >= 0);

    union
    {
        i32 ix;
        f32 x;
    } u{};

    u.x  = x0;                                            // x可以被视为i32。
    u.ix = u.ix / 4 + u.ix / 16;                          // 大约除以3。
    u.ix = u.ix + u.ix / 16;
    u.ix = u.ix + u.ix / 256;
    u.ix = 0x2a5137a0 + u.ix;                             // 初始猜测。
    u.x  = 0.33333333f * (2.0f * u.x + x0 / (u.x * u.x)); // 牛顿步骤。
    u.x  = 0.33333333f * (2.0f * u.x + x0 / (u.x * u.x)); // 再次牛顿步骤。
    return u.x;
}

/**
 * @brief 计算浮点数的平方根倒数的近似值。
 *
 * @param x0 要计算平方根倒数的浮点数。
 * @return 输入值的平方根倒数的近似值。
 * @note 该函数假定x0 > 0。如果x为负数，则行为未定义。
 */
template<FloatType T>
BEE_FUNC constexpr T rSqrtFast(T x0)
{
    BEE_CHECK(x0 > 0);

    if constexpr (sizeof(T) == 4) {
        union
        {
            i32 ix;
            f32 x;
        } u{};

        u.x       = x0;                               // x可以被视为一个i32。
        f32 xHalf = 0.5f * u.x;
        u.ix      = 0x5f37599e - (u.ix >> 1);         // 初始猜测。
        u.x       = u.x * (1.5f - xHalf * u.x * u.x); // 牛顿步骤。
        u.x       = u.x * (1.5f - xHalf * u.x * u.x); // 再次牛顿步骤。
        return u.x;
        return u.x;
    }
    else {
        union
        {
            i64 ix;
            f64 x;
        } u{};

        u.x  = x0;
        u.ix = 0x5fe6ec85e8000000LL - (u.ix >> 1);
        return u.x;
    }
}

/**
 * @brief 返回给定无符号整型的大小。
 *
 * @tparam T 一个无符号整型（例如，u8, u16, u32, u64）。
 * @return 指定类型的位数。
 */
template<UnsignedType T>
BEE_FUNC i32 BitWidth()
{
    if constexpr (std::is_same_v<T, u8>)
        return 8;
    else if constexpr (std::is_same_v<T, u16>)
        return 16;
    else if constexpr (std::is_same_v<T, u32>)
        return 32;
    else if constexpr (std::is_same_v<T, u64>)
        return 64;
}

/**
 * @brief 对无符号整数值进行左旋转。
 *
 * @tparam T 一个无符号整型。
 * @param x 要旋转的值。
 * @param k 旋转的位数。
 * @return 旋转后的值。
 */
template<std::unsigned_integral T>
BEE_FUNC T RotLeft(T x, i32 k)
{
    return T(x << k) | T(x >> (BitWidth<T>() - k));
}

/**
 * @brief 对无符号整数值进行右旋转。
 *
 * @tparam T 一个无符号整型。
 * @param x 要旋转的值。
 * @param k 旋转的位数。
 * @return 旋转后的值。
 */
template<std::unsigned_integral T>
BEE_FUNC T RotRight(T x, i32 k)
{
    return T(x >> k) | T(x << (BitWidth<T>() - k));
}

/**
 * @brief 计算32位无符号整数的前导零的数量。
 *
 * @param v 要计算前导零数的32位无符号整数值。
 * @return 该值中的前导零的数量。
 */
BEE_FUNC u32 NumLeadingZero(u32 v)
{
#ifdef BEE_GPU_CODE
    return __clz(v);
#elif defined(BEE_IN_WINDOWS)
    return __lzcnt(v);
#else // BEE_IN_WINDOWS
    return __builtin_clz(v);
#endif
}

/**
 * @brief 计算64位无符号整数的前导零的数量。
 *
 * @param v 要计算前导零数的64位无符号整数值。
 * @return 该值中的前导零的数量。
 */
BEE_FUNC u32 NumLeadingZero(u64 v)
{
#ifdef BEE_GPU_CODE
    return __clzll(v);
#elif defined(BEE_IN_WINDOWS)
    return cast_to<u32>(__lzcnt64(v));
#else // BEE_IN_WINDOWS
    return __builtin_clzll(v);
#endif
}

/**
 * @brief 计算32位无符号整数中设置位（1）的数量。
 *
 * @param x 要计算设置位的32位无符号整数。
 * @return 该值中设置位的数量。
 */
BEE_FUNC u32 BitCount(u32 x)
{
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x0000003F;
}

/**
 * @brief 反转32位无符号整数的位顺序。
 *
 * @param n 要反转位顺序的32位无符号整数。
 * @return 位顺序反转后的值。
 */
BEE_FUNC u32 ReverseBits(u32 n)
{
#ifdef BEE_GPU_CODE
    return __brev(n);
#else
    n = (n << 16) | (n >> 16);
    n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
    n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
    n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
    n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
    return n;
#endif
}

/**
 * @brief 反转64位无符号整数的位顺序。
 *
 * @param n 要反转位顺序的64位无符号整数。
 * @return 位顺序反转后的值。
 */
BEE_FUNC u64 ReverseBits(u64 n)
{
#ifdef BEE_GPU_CODE
    return __brevll(n);
#else
    u64 n0 = ReverseBits((u32)n);
    u64 n1 = ReverseBits((u32)(n >> 32));
    return (n0 << 32) | n1;
#endif
}

/**
 * @brief 计算浮点数的以2为底的对数的整数部分。
 *
 * @tparam T 浮点数类型（例如 `float`, `double`）。
 * @param v 要计算对数的浮点数值。
 * @return 输入值的以2为底的对数的整数部分。
 * @note 该函数假定 x0 > 0。如果 x 是负数，则行为未定义。
 */
BEE_FUNC i32 Log2Int(std::floating_point auto v)
{
    BEE_CHECK(v > 0);
    if (v < 1)
        return -Log2Int(1 / v);

    if constexpr (std::is_same_v<decltype(v), f32>) {
        return Exponent(v) + ((Significand(v) >= 0x3504F3) ? 1 : 0);
    }
    else {
        return Exponent(v) + ((Significand(v) >= 0x6A09E667F3BCD) ? 1 : 0);
    }
}

/**
 * @brief 计算32位无符号整数的以2为底的对数。
 *
 * @param v 要计算对数的32位无符号整数值。
 * @return 输入值的以2为底的对数，如果输入是0，则返回0。
 */
BEE_FUNC i32 Log2Int(u32 v)
{
#ifdef BEE_GPU_CODE
    return 31 - __clz(v);
#elif defined(BEE_IN_WINDOWS)
    auto lz = 0ul;
    if (_BitScanReverse(&lz, v))
        return static_cast<i32>(lz);
    return 0;
#else // BEE_IN_WINDOWS
    return 31 - __builtin_clz(v);
#endif
}

/**
 * @brief 计算64位无符号整数的以2为底的对数。
 *
 * @param v 要计算对数的64位无符号整数值。
 * @return 输入值的以2为底的对数，如果输入是0，则返回0。
 */
BEE_FUNC i32 Log2Int(u64 v)
{
#ifdef BEE_GPU_CODE
    return 64 - __clzll(v);
#elif defined(BEE_IN_WINDOWS)
    auto lz = 0ul;
    _BitScanReverse64(&lz, v);
    return static_cast<i32>(lz);
#else // BEE_IN_WINDOWS
    return 63 - __builtin_clzll(v);
#endif
}

/**
 * @brief 计算32位有符号整数的以2为底的对数。
 *
 * @param v 要计算对数的32位有符号整数值。
 * @return 输入值的以2为底的对数，如果输入是0，则返回0。
 */
BEE_FUNC i32 Log2Int(i32 v)
{
    return Log2Int((u32)v);
}

/**
 * @brief 计算64位有符号整数的以2为底的对数。
 *
 * @param v 要计算对数的64位有符号整数值。
 * @return 输入值的以2为底的对数，如果输入是0，则返回0。
 */
BEE_FUNC i32 Log2Int(i64 v)
{
    return Log2Int((u64)v);
}

/**
 * @brief 计算给定值的以4为底的对数。
 *
 * @tparam T 一个数值类型。必须是无符号整型或浮点类型。
 * @param v 要计算对数的值。
 * @return 输入值的以4为底的对数，作为一个整数。
 */
template<typename T>
BEE_FUNC i32 Log4Int(T v)
{
    return Log2Int(v) / 2;
}

/**
 * @brief 检查一个值是否是2的幂。
 *
 * @tparam T 一个整型（有符号或无符号）。
 * @param v 要检查的值。
 * @return 如果 `v` 是2的幂，则返回 `true`，否则返回 `false`。
 */
template<IntegralType T>
BEE_FUNC constexpr bool IsPowerOf2(T v)
{
    return v && !(v & (v - 1));
}

/**
 * @brief 检查一个值是否是4的幂。
 *
 * @tparam T 一个整型（有符号或无符号）。
 * @param v 要检查的值。
 * @return 如果 `v` 是4的幂，则返回 `true`，否则返回 `false`。
 */
template<IntegralType T>
BEE_FUNC constexpr bool IsPowerOf4(T v)
{
    return v == 1 << (2 * Log4Int(v));
}

/**
 * @brief 将一个整数值向上取到最近的2的幂。
 *
 * @param v 要向上取的整数值。
 * @return 输入值的最近的2的幂，且大于或等于输入值。
 */
template<IntegralType T>
BEE_FUNC constexpr T RoundUpPow2(T v)
{
    if (v == 0)
        return 1;

    if constexpr (sizeof(T) == 4) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v + 1;
    }
    else {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        return v + 1;
    }
}

/**
 * @brief 将一个整数值向下取到最近的2的幂。
 *
 * @param x 要向下取的整数值。
 * @return 输入值的最近的2的幂，且小于或等于输入值。
 */
template<IntegralType T>
BEE_FUNC constexpr T RoundDownPow2(T x)
{
    if constexpr (sizeof(T) == 4) {
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x - (x >> 1);
    }
    else {
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        return x - (x >> 1);
    }
}

/**
 * @brief 将一个整数向上舍入到给定数的最近倍数。
 *
 * @param x 要向上舍入的值。
 * @param y x 应该被舍入到的倍数。
 * @return 一个大于或等于 x 的 y 的最小倍数。
 */
BEE_FUNC u32 RoundUp(u32 x, u32 y)
{
    if (x == 0)
        return y;
    return ((x + y - 1) / y) * y;
    //    return (x + (y - 1)) & ~(y - 1);
}

/**
 * @brief 计算一个32位无符号整数的奇偶性。
 *
 * @param x 要计算奇偶性的32位无符号整数。
 * @return 奇偶性作为一个32位无符号整数（偶数为0，奇数为1）。
 */
BEE_FUNC u32 Parity(u32 x)
{
    x ^= x >> 1;
    x ^= x >> 2;
    x ^= x >> 4;
    x ^= x >> 8;
    x ^= x >> 16;
    return x & 1;
}

/**
 * @brief 打乱一个32位无符号整数的位。
 *
 * @param x 要打乱位的32位无符号整数。
 * @return 位按照特定模式打乱后的值。
 */
BEE_FUNC u32 Shuffle(u32 x)
{
    u32 t;
    // clang-format off
    t = (x ^ (x >> 8)) & 0x0000FF00;  x = x ^ t ^ (t << 8);
    t = (x ^ (x >> 4)) & 0x00F000F0;  x = x ^ t ^ (t << 4);
    t = (x ^ (x >> 2)) & 0x0C0C0C0C;  x = x ^ t ^ (t << 2);
    t = (x ^ (x >> 1)) & 0x22222222;  x = x ^ t ^ (t << 1);
    // clang-format on
    return x;
}

/**
 * @brief 还原一个32位无符号整数的位顺序。
 *
 * @param x 要还原位顺序的32位无符号整数。
 * @return 位顺序还原到原始状态的值。
 */
BEE_FUNC u32 Unshuffle(u32 x)
{
    u32 t;
    // clang-format off
    t = (x ^ (x >> 1)) & 0x22222222;  x = x ^ t ^ (t << 1);
    t = (x ^ (x >> 2)) & 0x0C0C0C0C;  x = x ^ t ^ (t << 2);
    t = (x ^ (x >> 4)) & 0x00F000F0;  x = x ^ t ^ (t << 4);
    t = (x ^ (x >> 8)) & 0x0000FF00;  x = x ^ t ^ (t << 8);
    // clang-format on
    return x;
}

} // namespace bee
