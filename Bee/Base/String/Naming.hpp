/**
 * @File Naming.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <string>
#include <string_view>

#include "Base/String/Ascii.hpp"

namespace bee
{

[[nodiscard]] inline auto ToSnakeCase(std::string_view text) -> std::string
{
    std::string result;
    result.reserve(text.size() * 2);

    auto append_underscore = [&result]() {
        if (!result.empty() && result.back() != '_') {
            result.push_back('_');
        }
    };

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (IsAsciiUpper(c)) {
            const bool has_prev               = i > 0;
            const bool has_next               = (i + 1) < text.size();
            const bool prev_is_lower_or_digit = has_prev && (IsAsciiLower(text[i - 1]) || IsAsciiDigit(text[i - 1]));
            const bool next_is_lower          = has_next && IsAsciiLower(text[i + 1]);
            if (prev_is_lower_or_digit || next_is_lower) {
                append_underscore();
            }
            result.push_back(ToAsciiLower(c));
            continue;
        }

        if (IsAsciiLower(c) || IsAsciiDigit(c)) {
            result.push_back(c);
            continue;
        }

        append_underscore();
    }

    // Strip leading and trailing underscores in one pass
    std::size_t start = 0;
    while (start < result.size() && result[start] == '_') {
        ++start;
    }
    std::size_t end = result.size();
    while (end > start && result[end - 1] == '_') {
        --end;
    }

    if (start == 0 && end == result.size()) {
        return result;
    }
    return result.substr(start, end - start);
}

} // namespace bee
