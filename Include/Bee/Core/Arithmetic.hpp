/**
 * @File Arithmetic.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/12/5
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Concepts.hpp"
#include <algorithm>

namespace bee
{

template <typename T>
concept ArithmeticType = requires(T t)
{
    T::Dimension;
    { t[0] };
};

template <typename Op>
struct ArithmeticUnaryOp
{
    template <ArithmeticType Lhs>
    static constexpr Lhs& Apply(Lhs& lhs)
    {
        for (int i = 0; i < Lhs::Dimension; ++i)
        {
            lhs[i] = Op::Apply(lhs[i]);
        }
        return lhs;
    }
};

template <typename Op>
struct ArithmeticBinaryOp
{
    template <ArithmeticType Lhs, typename Rhs>
    static constexpr Lhs& Apply(Lhs& lhs, const Rhs& rhs)
    {
        if constexpr (ArithmeticType<Rhs>)
        {
            for (int i = 0; i < std::min(Lhs::Dimension, Rhs::Dimension); ++i)
            {
                lhs[i] = Op::Apply(lhs[i], rhs[i]);
            }
        }
        else
        {
            for (int i = 0; i < Lhs::Dimension; ++i)
            {
                lhs[i] = Op::Apply(lhs[i], rhs);
            }
        }
        return lhs;
    }
};

template <typename Op>
struct ArithmeticComparisonOp
{
    template <ArithmeticType Lhs, typename Rhs>
    static constexpr bool ApplyAll(const Lhs& lhs, const Rhs& rhs)
    {
        if constexpr (ArithmeticType<Rhs>)
        {
            for (int i = 0; i < std::min(Lhs::Dimension, Rhs::Dimension); ++i)
            {
                if (!Op::Apply(lhs[i], rhs[i]))
                    return false;
            }
        }
        else
        {
            for (int i = 0; i < Lhs::Dimension; ++i)
            {
                if (!Op::Apply(lhs[i], rhs))
                    return false;
            }
        }
        return true;
    }

    template <ArithmeticType Lhs, typename Rhs>
    static constexpr bool ApplyAny(const Lhs& lhs, const Rhs& rhs)
    {
        if constexpr (ArithmeticType<Rhs>)
        {
            for (int i = 0; i < std::min(Lhs::Dimension, Rhs::Dimension); ++i)
            {
                if (Op::Apply(lhs[i], rhs[i]))
                    return true;
            }
        }
        else
        {
            for (int i = 0; i < Lhs::Dimension; ++i)
            {
                if (Op::Apply(lhs[i], rhs))
                    return true;
            }
        }
        return false;
    }
};

// ==================== 各种数学操作 ====================

struct AddOp
{
    template <typename T, typename U>
    BEE_FUNC static auto Apply(T a, U b) -> decltype(a + b)
    {
        return a + b;
    }
};

struct SubOp
{
    template <typename T, typename U>
    BEE_FUNC static auto Apply(T a, U b) -> decltype(a - b)
    {
        return a - b;
    }
};

struct MulOp
{
    template <typename T, typename U>
    BEE_FUNC static auto Apply(T a, U b) -> decltype(a * b)
    {
        return a * b;
    }
};

struct DivOp
{
    template <typename T, typename U>
    BEE_FUNC static auto Apply(T a, U b) -> decltype(a / b)
    {
        return a / b;
    }
};

struct ModOp
{
    template <typename T, typename U>
    BEE_FUNC static auto Apply(T a, U b) -> decltype(a % b)
    {
        return a % b;
    }
};

struct AndOp
{
    template <typename T, typename U>
    BEE_FUNC static T Apply(T a, U b)
    {
        return a & b;
    }
};

struct OrOp
{
    template <typename T, typename U>
    BEE_FUNC static T Apply(T a, U b)
    {
        return a | b;
    }
};

struct XorOp
{
    template <typename T, typename U>
    BEE_FUNC static T Apply(T a, U b)
    {
        return a ^ b;
    }
};

struct LeftShiftOp
{
    template <typename T, typename U>
    BEE_FUNC static T Apply(T a, U b)
    {
        return a << b;
    }
};

struct RightShiftOp
{
    template <typename T, typename U>
    BEE_FUNC static T Apply(T a, U b)
    {
        return a >> b;
    }
};

struct UnaryPlusOp
{
    template <typename T>
    BEE_FUNC static auto Apply(T a) -> decltype(+a)
    {
        return +a;
    }
};

struct UnaryMinusOp
{
    template <typename T>
    BEE_FUNC static auto Apply(T a) -> decltype(-a)
    {
        return -a;
    }
};

struct EqualOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a == b;
    }
};

struct NotEqualOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a != b;
    }
};

struct LessOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a < b;
    }
};

struct LessEqualOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a <= b;
    }
};

struct GreaterOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a > b;
    }
};

struct GreaterEqualOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a >= b;
    }
};

struct LogicalAndOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a && b;
    }
};

struct LogicalOrOp
{
    template <typename T, typename U>
    BEE_FUNC static bool Apply(T a, U b)
    {
        return a || b;
    }
};

struct LogicalNotOp
{
    template <typename T>
    BEE_FUNC static bool Apply(T a)
    {
        return !a;
    }
};

} // namespace bee
