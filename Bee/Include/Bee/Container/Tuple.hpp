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
    using OtherType = Derived<U>;

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
        template <ArrayLike AL, typename F>
        static constexpr AL& CallUnary(AL& al, F&& func)
        {
            for (int i = 0; i < AL::Dimension; ++i)
            {
                al[i] = std::invoke(func, al[i]);
            }

            return al;
        }

        template <ArrayLike Lhs, typename Rhs, typename F>
        static constexpr Lhs& CallBinary(Lhs& lhs, const Rhs& rhs, F&& func)
        {
            using T = Lhs::value_type;

            if constexpr (ArrayLike<Rhs>)
            {
                for (int i = 0; i < std::min(Lhs::Dimension, Rhs::Dimension); ++i)
                {
                    lhs[i] = std::invoke(func, lhs[i], To<T>(rhs[i]));
                }
            }
            else
            {
                for (int i = 0; i < Lhs::Dimension; ++i)
                {
                    lhs[i] = std::invoke(func, lhs[i], To<T>(rhs));
                }
            }
            return lhs;
        }

        template <ArrayLike A, ArrayLike B, typename C, typename F>
        static constexpr A& CallTernary(A& a, const B& b, const C& c, F&& func)
        {
            using T = A::value_type;

            if constexpr (ArrayLike<C>)
            {
                constexpr int N = std::min(A::Dimension, std::min(B::Dimension, C::Dimension));
                for (int i = 0; i < N; ++i)
                {
                    a[i] = std::invoke(func, a[i], To<T>(b[i]), To<T>(c[i]));
                }
            }
            else
            {
                constexpr int N = std::min(A::Dimension, B::Dimension);
                for (int i = 0; i < N; ++i)
                {
                    a[i] = std::invoke(func, a[i], To<T>(b[i]), To<T>(c));
                }
            }
            return a;
        }

        template <ArrayLike AL, typename I, typename F>
        static constexpr auto Reduce(const AL& al, I init, F&& func)
        {
            using T = AL::value_type;

            return std::reduce(al.begin(), al.end(), To<T>(init), std::forward<F>(func));
        }
    };

} // namespace detail

// ==================== 成员 ====================

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr auto Tuple<Derived, T, N>::operator+(const Derived<U>& c) const -> Derived<decltype(T{} + U{})>
{
    using R  = decltype(T{} + U{});
    auto ret = Derived<R>{derived()};
    return ArithmeticBinaryOp<AddOp>::Apply(ret, c);
}

template <template <typename> class Derived, ArithType T, int N>
template <ArithType U>
constexpr auto Tuple<Derived, T, N>::operator-(const Derived<U>& c) const -> Derived<decltype(T{} - U{})>
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

template <template <typename> class Derived, ArithType T, int N>
constexpr auto Min(const Tuple<Derived, T, N>& c0, const Tuple<Derived, T, N>& c1)
{
    auto ret = Tuple<Derived, T, N>{c0.derived()};
    return detail::TupleFunc::CallBinary(ret, c1, [](T v0, T v1)
    {
        return std::min(v0, v1);
    });
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto Max(const Tuple<Derived, T, N>& c0, const Tuple<Derived, T, N>& c1)
{
    auto ret = Tuple<Derived, T, N>{c0.derived()};
    return detail::TupleFunc::CallBinary(ret, c1, [](T v0, T v1)
    {
        return std::max(v0, v1);
    });
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto MinValue(const Tuple<Derived, T, N>& c)
{
    return detail::TupleFunc::Reduce(c, c[0], [](T v0, T v1)
    {
        return std::min(v0, v1);
    });
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto MaxValue(const Tuple<Derived, T, N>& c)
{
    return detail::TupleFunc::Reduce(c, c[0], [](T v0, T v1)
    {
        return std::max(v0, v1);
    });
}

template <template <typename> class Derived, ArithType T, int N>
constexpr int MinIndex(const Tuple<Derived, T, N>& c)
{
    int index = 0;
    for (int i = 1; i < N; ++i)
    {
        if (c[i] < c[index])
        {
            index = i;
        }
    }
    return index;
}

template <template <typename> class Derived, ArithType T, int N>
constexpr int MaxIndex(const Tuple<Derived, T, N>& c)
{
    int index = 0;
    for (int i = 1; i < N; ++i)
    {
        if (c[i] > c[index])
        {
            index = i;
        }
    }
    return index;
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto Sum(const Tuple<Derived, T, N>& c)
{
    return detail::TupleFunc::Reduce(c, 0, std::plus<T>{});
}

template <template <typename> class Derived, ArithType T, int N>
constexpr auto Product(const Tuple<Derived, T, N>& c)
{
    return detail::TupleFunc::Reduce(c, 1, std::multiplies<T>{});
}

template <template <typename> class Derived, ArithType T, ArithType U, int N, FloatType F>
constexpr auto Lerp(const Tuple<Derived, T, N>& c0, const Tuple<Derived, U, N>& c1, F alpha)
{
    using R = CommonFloatType<T, U, F>;
    
    auto ret = Tuple<Derived, R, N>{c0.derived()};
    return detail::TupleFunc::CallTernary(ret, c1, alpha, [](R v0, R v1, R a)
    {
        return std::lerp(v0, v1, a);
    });
}

template <template <typename> class Derived, ArithType T, ArithType U, ArithType V, int N>
constexpr auto FMA(const Tuple<Derived, T, N>& c0, const Tuple<Derived, U, N>& c1, const Tuple<Derived, V, N>& c2)
{
    using R = CommonFloatType<T, U, V>;
    
    auto ret = Tuple<Derived, R, N>{c0.derived()};
    return detail::TupleFunc::CallTernary(ret, c1, c2, [](R x, R y, R z)
    {
        return std::fma(x, y, z);
    });
}

template <template <typename> class Derived, ArithType T, ArithType U, ArithType V, int N>
constexpr auto FMA(const Tuple<Derived, T, N>& c0, const Tuple<Derived, U, N>& c1, V c2)
{
    using R = CommonFloatType<T, U, V>;
    
    auto ret = Tuple<Derived, R, N>{c0.derived()};
    return detail::TupleFunc::CallTernary(ret, c1, c2, [](R x, R y, R z)
    {
        return std::fma(x, y, z);
    });
}

} // namespace bee
