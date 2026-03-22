/**
 * @File Ascii.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#pragma once

namespace bee
{

constexpr auto IsAsciiUpper(char c) -> bool
{
    return c >= 'A' && c <= 'Z';
}

constexpr auto IsAsciiLower(char c) -> bool
{
    return c >= 'a' && c <= 'z';
}

constexpr auto IsAsciiDigit(char c) -> bool
{
    return c >= '0' && c <= '9';
}

constexpr auto IsAsciiAlpha(char c) -> bool
{
    return IsAsciiUpper(c) || IsAsciiLower(c);
}

constexpr auto ToAsciiLower(char c) -> char
{
    if (IsAsciiUpper(c)) {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

} // namespace bee
