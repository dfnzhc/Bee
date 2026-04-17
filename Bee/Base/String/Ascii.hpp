/**
 * @File Ascii.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#pragma once

namespace bee
{

[[nodiscard]] constexpr auto IsAsciiUpper(char c) noexcept -> bool
{
    return c >= 'A' && c <= 'Z';
}

[[nodiscard]] constexpr auto IsAsciiLower(char c) noexcept -> bool
{
    return c >= 'a' && c <= 'z';
}

[[nodiscard]] constexpr auto IsAsciiDigit(char c) noexcept -> bool
{
    return c >= '0' && c <= '9';
}

[[nodiscard]] constexpr auto IsAsciiAlpha(char c) noexcept -> bool
{
    return IsAsciiUpper(c) || IsAsciiLower(c);
}

[[nodiscard]] constexpr auto ToAsciiLower(char c) noexcept -> char
{
    if (IsAsciiUpper(c)) {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

} // namespace bee
