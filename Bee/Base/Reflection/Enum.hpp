/**
 * @File Enum.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include <bit>
#include <type_traits>

namespace bee
{

template <typename E>
struct EnableEnumBitmaskOperators : std::false_type
{
};

template <typename E>
inline constexpr bool EnableEnumBitmaskOperatorsV = EnableEnumBitmaskOperators<E>::value;

template <typename E>
concept EnumBitmask = std::is_enum_v<E> && EnableEnumBitmaskOperatorsV<E>;

template <typename E>
[[nodiscard]] constexpr auto ToUnderlying(E value) noexcept -> std::underlying_type_t<E>
{
    static_assert(std::is_enum_v<E>);
    return static_cast<std::underlying_type_t<E>>(value);
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumIsZero(E value) noexcept -> bool
{
    return ToUnderlying(value) == 0;
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumHasAny(E value, E flags) noexcept -> bool
{
    using U = std::underlying_type_t<E>;
    return (static_cast<U>(value) & static_cast<U>(flags)) != 0;
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumHasAll(E value, E flags) noexcept -> bool
{
    using U               = std::underlying_type_t<E>;
    const auto value_bits = static_cast<U>(value);
    const auto flag_bits  = static_cast<U>(flags);
    return (value_bits & flag_bits) == flag_bits;
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumHasNone(E value, E flags) noexcept -> bool
{
    return !EnumHasAny(value, flags);
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumSet(E value, E flags) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(value) | static_cast<U>(flags));
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumClear(E value, E flags) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(value) & ~static_cast<U>(flags));
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumToggle(E value, E flags) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(value) ^ static_cast<U>(flags));
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumBitCount(E value) noexcept -> int
{
    using U = std::make_unsigned_t<std::underlying_type_t<E>>;
    return std::popcount(static_cast<U>(ToUnderlying(value)));
}

template <EnumBitmask E>
[[nodiscard]] constexpr auto EnumIsSingleBit(E value) noexcept -> bool
{
    using U         = std::make_unsigned_t<std::underlying_type_t<E>>;
    const auto bits = static_cast<U>(ToUnderlying(value));
    return std::has_single_bit(bits);
}

} // namespace bee

#define BEE_ENABLE_ENUM_BITMASK_OPERATORS(type)                  \
    namespace bee                                                \
    {                                                            \
        template <>                                              \
        struct EnableEnumBitmaskOperators<type> : std::true_type \
        {                                                        \
        };                                                       \
    }

template <typename E>
    requires bee::EnumBitmask<E>
constexpr auto operator|(E left, E right) noexcept -> E
{
    return bee::EnumSet(left, right);
}

template <typename E>
    requires bee::EnumBitmask<E>
constexpr auto operator&(E left, E right) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(left) & static_cast<U>(right));
}

template <typename E>
    requires bee::EnumBitmask<E>
constexpr auto operator^(E left, E right) noexcept -> E
{
    return bee::EnumToggle(left, right);
}

template <typename E>
    requires bee::EnumBitmask<E>
constexpr auto operator~(E value) noexcept -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(~static_cast<U>(value));
}

template <typename E>
    requires bee::EnumBitmask<E>
constexpr auto operator|=(E& left, E right) noexcept -> E&
{
    left = left | right;
    return left;
}

template <typename E>
    requires bee::EnumBitmask<E>
constexpr auto operator&=(E& left, E right) noexcept -> E&
{
    left = left & right;
    return left;
}

template <typename E>
    requires bee::EnumBitmask<E>
constexpr auto operator^=(E& left, E right) noexcept -> E&
{
    left = left ^ right;
    return left;
}
