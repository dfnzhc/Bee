/**
 * @File Tuple.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/5
 * @Brief This file is part of Bee.
 */

#pragma once

#include <array>
#include "Bee/Core/Arithmetic.hpp"

namespace bee
{

template <template<typename> class Derived, ArithType T, int N>
class Tuple;

// clang-format off
template <template<typename> class Derived, ArithType T> using Tuple2 = Tuple<Derived, T, 2>;
template <template<typename> class Derived, ArithType T> using Tuple3 = Tuple<Derived, T, 3>;
template <template<typename> class Derived, ArithType T> using Tuple4 = Tuple<Derived, T, 4>;
// clang-format on

template <template<typename> class Derived, ArithType T, int N>
class Tuple
{
    static_assert(N * sizeof(T) <= kCacheLineSize);

public:
    // 类型定义
    using value_type      = T;
    using size_type       = size_t;
    using reference       = T&;
    using const_reference = const T&;
    using iterator        = typename std::array<T, N>::iterator;
    using const_iterator  = typename std::array<T, N>::const_iterator;

    using SelfType = Derived<T>;
    template <typename U>
    using OtherType = Tuple<Derived, U, N>;

    static constexpr int Dimension = N;

    // 获取派生类引用
    // clang-format off
    constexpr       SelfType& derived()       noexcept { return static_cast<      SelfType&>(*this); }
    constexpr const SelfType& derived() const noexcept { return static_cast<const SelfType&>(*this); }
    // clang-format on

    constexpr Tuple() noexcept = default;

    constexpr Tuple(const Tuple&)            = default;
    constexpr Tuple& operator=(const Tuple&) = default;

    template <typename... Args>
        requires (sizeof...(Args) == N) && AllConvertibleTo<T, Args...>
    constexpr explicit Tuple(Args... args) noexcept :
        data{static_cast<T>(args)...}
    {
        BEE_DCHECK(!hasNaN());
    }

    // clang-format off
    template <ArithType U>
    constexpr explicit Tuple(const OtherType<U>& other) noexcept
    {
        std::transform(other.begin(), other.end(), data.begin(), [](U val) { return static_cast<T>(val); });
        BEE_DCHECK(!hasNaN());
    }

    constexpr       iterator  begin()       noexcept { return data.begin(); }
    constexpr const_iterator  begin() const noexcept { return data.begin(); }
    constexpr const_iterator cbegin() const noexcept { return data.cbegin(); }
        
    constexpr       iterator  end()       noexcept { return data.end(); }
    constexpr const_iterator  end() const noexcept { return data.end(); }
    constexpr const_iterator cend() const noexcept { return data.cend(); }
    
    constexpr T  operator[](int i) const { return data[i]; }
    constexpr T& operator[](int i)       { return data[i]; }

    template <ArithType U> constexpr auto operator+(const OtherType<U>& c) const -> Derived<decltype(T{} + U{})>;
    template <ArithType U> constexpr auto operator-(const OtherType<U>& c) const -> Derived<decltype(T{} - U{})>;
    template <ArithType U> constexpr auto operator*(U s) const -> Derived<decltype(T{} * U{})>;
    template <ArithType U> constexpr auto operator/(U d) const -> Derived<decltype(T{} / U{})>;

    template <ArithType U> constexpr SelfType& operator+=(const OtherType<U>& c);
    template <ArithType U> constexpr SelfType& operator-=(const OtherType<U>& c);
    template <ArithType U> constexpr SelfType& operator*=(U s);
    template <ArithType U> constexpr SelfType& operator/=(U d);

    constexpr SelfType operator-() const;

    constexpr bool operator==(const SelfType& c) const;
    constexpr bool operator!=(const SelfType& c) const;

    constexpr int size() const noexcept { return N; }
    // clang-format on

    constexpr bool hasNaN() const;

    std::array<T, N> data{};
};

namespace detail
{
    struct TupleFunc
    {
        template <ArithmeticType Lhs, typename F>
        static constexpr Lhs& CallUnary(Lhs& lhs, F&& func)
        {
            for (int i = 0; i < Lhs::Dimension; ++i)
            {
                lhs[i] = std::invoke(func, lhs[i]);
            }

            return lhs;
        }

        template <ArithmeticType Lhs, typename Rhs, typename F>
        static constexpr Lhs& CallBinary(Lhs& lhs, const Rhs& rhs, F&& func)
        {
            if constexpr (ArithmeticType<Rhs>)
            {
                for (int i = 0; i < std::min(Lhs::Dimension, Rhs::Dimension); ++i)
                {
                    lhs[i] = std::invoke(func, lhs[i], rhs[i]);
                }
            }
            else
            {
                for (int i = 0; i < Lhs::Dimension; ++i)
                {
                    lhs[i] = std::invoke(func, lhs[i], rhs);
                }
            }
            return lhs;
        }
    };

} // namespace detail

// ==================== 成员 ====================

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr auto Tuple<Derived, T, N>::operator+(const OtherType<U>& c) const -> Derived<decltype(T{} + U{})>
{
    using R  = decltype(T{} + U{});
    auto ret = Derived<R>{derived()};
    return ArithmeticBinaryOp<AddOp>::Apply(ret, c);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr auto Tuple<Derived, T, N>::operator-(const OtherType<U>& c) const -> Derived<decltype(T{} - U{})>
{
    using R  = decltype(T{} + U{});
    auto ret = Derived<R>{derived()};
    return ArithmeticBinaryOp<SubOp>::Apply(ret, c);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr auto Tuple<Derived, T, N>::operator*(U s) const -> Derived<decltype(T{} * U{})>
{
    using R  = decltype(T{} + U{});
    auto ret = Derived<R>{derived()};
    return ArithmeticBinaryOp<MulOp>::Apply(ret, s);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr auto Tuple<Derived, T, N>::operator/(U d) const -> Derived<decltype(T{} / U{})>
{
    BEE_DCHECK(d != 0);
    using R  = decltype(T{} + U{});
    auto ret = Derived<R>{derived()};
    return ArithmeticBinaryOp<DivOp>::Apply(ret, d);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr Tuple<Derived, T, N>::SelfType& Tuple<Derived, T, N>::operator+=(const OtherType<U>& c)
{
    return ArithmeticBinaryOp<AddOp>::Apply(derived(), c);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr Tuple<Derived, T, N>::SelfType& Tuple<Derived, T, N>::operator-=(const OtherType<U>& c)
{
    return ArithmeticBinaryOp<SubOp>::Apply(derived(), c);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr Tuple<Derived, T, N>::SelfType& Tuple<Derived, T, N>::operator*=(U s)
{
    return ArithmeticBinaryOp<MulOp>::Apply(derived(), s);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr Tuple<Derived, T, N>::SelfType& Tuple<Derived, T, N>::operator/=(U d)
{
    BEE_DCHECK(d != 0);
    return ArithmeticBinaryOp<DivOp>::Apply(derived(), d);
}

template <template <typename> class Derived, ArithType T, int N>
constexpr Tuple<Derived, T, N>::SelfType Tuple<Derived, T, N>::operator-() const
{
    auto ret = SelfType{derived()};
    return ArithmeticUnaryOp<UnaryMinusOp>::Apply(ret);
}

template <template <typename> class Derived, ArithType T, int N>
constexpr bool Tuple<Derived, T, N>::operator==(const SelfType& c) const
{
    return ArithmeticComparisonOp<EqualOp>::ApplyAll(derived(), c);
}

template <template <typename> class Derived, ArithType T, int N>
constexpr bool Tuple<Derived, T, N>::operator!=(const SelfType& c) const
{
    return ArithmeticComparisonOp<NotEqualOp>::ApplyAny(derived(), c);
}

template <template <typename> class Derived, ArithType T, int N>
constexpr bool Tuple<Derived, T, N>::hasNaN() const
{
    for (int i = 0; i < N; i++)
        if (std::isnan(data[i]))
            return true;

    return false;
}

// ==================== 外部函数 ====================

template <ArithType U, template <typename> class Derived, ArithType T, int N>
constexpr auto operator*(U s, const Tuple<Derived, T, N>& c) -> Derived<decltype(T{} + U{})>
{
    using R  = decltype(T{} + U{});
    auto ret = Derived<R>{c.derived()};
    return ArithmeticBinaryOp<MulOp>::Apply(ret, s);
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto Abs(const Tuple<Derived, T, N>& c)
{
    auto ret = Tuple<Derived, T, N>{c.derived()};
    return detail::TupleFunc::CallUnary(ret, [](T v)
    {
        return std::abs(v);
    });
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto Floor(const Tuple<Derived, T, N>& c)
{
    auto ret = Tuple<Derived, T, N>{c.derived()};
    return detail::TupleFunc::CallUnary(ret, [](T v)
    {
        return std::floor(v);
    });
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto Ceil(const Tuple<Derived, T, N>& c)
{
    auto ret = Tuple<Derived, T, N>{c.derived()};
    return detail::TupleFunc::CallUnary(ret, [](T v)
    {
        return std::ceil(v);
    });
}

} // namespace bee
