/**
 * @File Text.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <string_view>

namespace bee
{

[[nodiscard]] constexpr auto TrimSpaces(std::string_view text) noexcept -> std::string_view
{
    while (!text.empty() && text.front() == ' ') {
        text.remove_prefix(1);
    }
    while (!text.empty() && text.back() == ' ') {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] constexpr auto Slice(std::string_view text, std::string_view prefix, std::string_view suffix, std::string_view fallback = {}) noexcept -> std::string_view
{
    const auto start = text.find(prefix);
    if (start == std::string_view::npos) {
        return fallback;
    }

    const auto value_start = start + prefix.size();
    const auto end         = text.find(suffix, value_start);
    if (end == std::string_view::npos || end <= value_start) {
        return fallback;
    }

    return text.substr(value_start, end - value_start);
}

} // namespace bee
