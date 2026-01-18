/**
 * @File Tuple.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/5
 * @Brief This file is part of Bee.
 */

#pragma once

#include <array>
#include <numeric>

#include "Bee/Core/Arithmetic.hpp"
#include "Bee/Core/Check.hpp"
#include "Bee/Core/Concepts.hpp"
#include "Bee/Math/Common.hpp"

namespace bee
{

template <template<typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
class Tuple;

// clang-format off
template <template<typename> class Derived, ArithType T> using Tuple2 = Tuple<Derived, T, 2>;
template <template<typename> class Derived, ArithType T> using Tuple3 = Tuple<Derived, T, 3>;
template <template<typename> class Derived, ArithType T> using Tuple4 = Tuple<Derived, T, 4>;
// clang-format on

template <template<typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
class Tuple
{
    static_assert(N * sizeof(T) <= kCacheLineSize, "Tuple size exceeds cache line");

public:
    // ========== 类型定义 ==========
    using value_type      = T;
    using size_type       = size_t;
    using reference       = T&;
    using const_reference = const T&;

    using SelfType = Derived<T>;
    template <typename U>
    using OtherType = Derived<U>;

    static constexpr int Dimension = N;

    // ========== 编译期断言 ==========
    static_assert(N > 0 && N <= 4, "Only 1-4 dimensional tuples supported");

    // ========== 获取派生类引用 ==========
    // clang-format off
    constexpr       SelfType& derived()       noexcept { return static_cast<      SelfType&>(*this); }
    constexpr const SelfType& derived() const noexcept { return static_cast<const SelfType&>(*this); }
    // clang-format on

    constexpr Tuple() noexcept = default;

    constexpr Tuple(const Tuple&)            = default;
    constexpr Tuple& operator=(const Tuple&) = default;

    template <typename... Args>
        requires (sizeof...(Args) == N) && AllConvertibleTo<T, Args...>
    constexpr explicit Tuple(Args... args) noexcept
    {
        BEE_DCHECK(((!std::isnan(static_cast<T>(args))) && ...));
    }

    template <ArithType U>
    constexpr explicit Tuple(const OtherType<U>& other) noexcept
    {
        for (int i = 0; i < N; ++i)
            derived()[i] = static_cast<T>(other[i]);
        BEE_DCHECK(!hasNaN());
    }

    // ========== 数据访问接口 ==========

    constexpr T& at(int i) noexcept
    {
        BEE_DCHECK(i >= 0 && i < N);
        return derived()[i];
    }

    constexpr T at(int i) const noexcept
    {
        BEE_DCHECK(i >= 0 && i < N);
        return derived()[i];
    }

    constexpr int size() const noexcept { return N; }

    // ========== 算术运算符 ==========

    template <ArithType U>
    constexpr auto operator+(const OtherType<U>& other) const -> Derived<decltype(T{} + U{})>
    {
        using R = decltype(T{} + U{});
        Derived<R> result;
        for (int i = 0; i < N; ++i)
            result[i] = at(i) + other[i];
        return result;
    }

    template <ArithType U>
    constexpr auto operator-(const OtherType<U>& other) const -> Derived<decltype(T{} - U{})>
    {
        using R = decltype(T{} - U{});
        Derived<R> result;
        for (int i = 0; i < N; ++i)
            result[i] = at(i) - other[i];
        return result;
    }

    template <ArithType U>
    constexpr auto operator*(U scalar) const -> Derived<decltype(T{} * U{})>
    {
        using R = decltype(T{} * U{});
        Derived<R> result;
        for (int i = 0; i < N; ++i)
            result[i] = at(i) * scalar;
        return result;
    }

    template <ArithType U>
    constexpr auto operator/(U divisor) const -> Derived<decltype(T{} / U{})>
    {
        BEE_DCHECK(divisor != 0);
        using R = decltype(T{} / U{});
        Derived<R> result;
        for (int i = 0; i < N; ++i)
            result[i] = at(i) / divisor;
        return result;
    }

    // ========== 复合赋值运算符 ==========

    template <ArithType U>
    constexpr SelfType& operator+=(const OtherType<U>& other)
    {
        for (int i = 0; i < N; ++i)
            derived()[i] += static_cast<T>(other[i]);
        return derived();
    }

    template <ArithType U>
    constexpr SelfType& operator-=(const OtherType<U>& other)
    {
        for (int i = 0; i < N; ++i)
            derived()[i] -= static_cast<T>(other[i]);
        return derived();
    }

    template <ArithType U>
    constexpr SelfType& operator*=(U scalar)
    {
        for (int i = 0; i < N; ++i)
            derived()[i] *= static_cast<T>(scalar);
        return derived();
    }

    template <ArithType U>
    constexpr SelfType& operator/=(U divisor)
    {
        BEE_DCHECK(divisor != 0);
        for (int i = 0; i < N; ++i)
            derived()[i] /= static_cast<T>(divisor);
        return derived();
    }

    // ========== 一元运算符 ==========

    constexpr SelfType operator-() const
    {
        SelfType result(derived());
        for (int i = 0; i < N; ++i)
            result[i] = -at(i);
        return result;
    }

    // ========== 比较运算符 ==========

    constexpr bool operator==(const SelfType& other) const
    {
        for (int i = 0; i < N; ++i)
            if (at(i) != other[i])
                return false;
        return true;
    }

    constexpr bool operator!=(const SelfType& other) const
    {
        return !(*this == other);
    }

    // ========== 实用方法 ==========

    constexpr bool hasNaN() const
    {
        for (int i = 0; i < N; ++i)
            if (std::isnan(at(i)))
                return true;
        return false;
    }
};

// ==================== 外部函数 ====================

template <ArithType U, template <typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
constexpr auto operator*(U scalar, const Tuple<Derived, T, N>& tuple) -> Derived<decltype(T{} * U{})>
{
    return tuple * scalar;
}

// ==================== Tuple 实用函数 ====================

namespace detail
{
    template <typename Result, typename T, int N>
    struct TupleConstructor
    {
        template <typename... Ts>
        static constexpr Result construct(Ts&&... vals)
        {
            Result result{};
            int idx = 0;
            ((result[idx++] = std::forward<Ts>(vals)), ...);
            return result;
        }
    };
}

template <template <typename> class Derived, ArithType T, ArithType U, int N>
    requires (N > 0 && N <= 4)
constexpr auto Min(const Tuple<Derived, T, N>& a, const Tuple<Derived, U, N>& b)
{
    using R = std::common_type_t<T, U>;
    Derived<R> result{};
    for (int i = 0; i < N; ++i)
        result[i] = bee::Min(a.at(i), b.at(i));
    return result;
}

template <template <typename> class Derived, ArithType T, ArithType U, int N>
    requires (N > 0 && N <= 4)
constexpr auto Max(const Tuple<Derived, T, N>& a, const Tuple<Derived, U, N>& b)
{
    using R = std::common_type_t<T, U>;
    Derived<R> result{};
    for (int i = 0; i < N; ++i)
        result[i] = bee::Max(a.at(i), b.at(i));
    return result;
}

template <template <typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
constexpr auto Abs(const Tuple<Derived, T, N>& t)
{
    using R = std::decay_t<decltype(bee::Abs(T{}))>;
    Derived<R> result{};
    for (int i = 0; i < N; ++i)
        result[i] = bee::Abs(t.at(i));
    return result;
}

template <template <typename> class Derived, FloatType T, int N>
    requires (N > 0 && N <= 4)
constexpr auto Floor(const Tuple<Derived, T, N>& t)
{
    Derived<T> result{};
    for (int i = 0; i < N; ++i)
        result[i] = bee::Floor(t.at(i));
    return result;
}

template <template <typename> class Derived, FloatType T, int N>
    requires (N > 0 && N <= 4)
constexpr auto Ceil(const Tuple<Derived, T, N>& t)
{
    Derived<T> result{};
    for (int i = 0; i < N; ++i)
        result[i] = bee::Ceil(t.at(i));
    return result;
}

template <template <typename> class Derived, ArithType T, ArithType U, ArithType V, int N>
    requires (N > 0 && N <= 4)
constexpr auto Lerp(const Tuple<Derived, T, N>& a, const Tuple<Derived, U, N>& b, V t)
{
    using R = std::common_type_t<T, U>;
    Derived<R> result{};
    for (int i = 0; i < N; ++i)
        result[i] = bee::Lerp(a.at(i), b.at(i), t);
    return result;
}

template <template <typename> class Derived, ArithType T, ArithType U, ArithType V, int N>
    requires (N > 0 && N <= 4)
constexpr auto FMA(const Tuple<Derived, T, N>& a, const Tuple<Derived, U, N>& b, V c)
{
    using R = std::common_type_t<T, U>;
    Derived<R> result{};
    for (int i = 0; i < N; ++i)
        result[i] = bee::FMA(a.at(i), b.at(i), c);
    return result;
}

template <template <typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
constexpr T MinValue(const Tuple<Derived, T, N>& t)
{
    T minVal = t.at(0);
    for (int i = 1; i < N; ++i)
        if (t.at(i) < minVal)
            minVal = t.at(i);
    return minVal;
}

template <template <typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
constexpr T MaxValue(const Tuple<Derived, T, N>& t)
{
    T maxVal = t.at(0);
    for (int i = 1; i < N; ++i)
        if (t.at(i) > maxVal)
            maxVal = t.at(i);
    return maxVal;
}

template <template <typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
constexpr int MinIndex(const Tuple<Derived, T, N>& t)
{
    int minIdx = 0;
    T minVal = t.at(0);
    for (int i = 1; i < N; ++i)
        if (t.at(i) < minVal) {
            minVal = t.at(i);
            minIdx = i;
        }
    return minIdx;
}

template <template <typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
constexpr int MaxIndex(const Tuple<Derived, T, N>& t)
{
    int maxIdx = 0;
    T maxVal = t.at(0);
    for (int i = 1; i < N; ++i)
        if (t.at(i) > maxVal) {
            maxVal = t.at(i);
            maxIdx = i;
        }
    return maxIdx;
}

template <template <typename> class Derived, ArithType T, int N>
    requires (N > 0 && N <= 4)
constexpr T Product(const Tuple<Derived, T, N>& t)
{
    T prod = T{1};
    for (int i = 0; i < N; ++i)
        prod = prod * t.at(i);
    return prod;
}

} // namespace bee
