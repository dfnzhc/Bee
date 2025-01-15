/**
 * @File String.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/21
 * @Brief This file is part of Bee.
 */

#pragma once

namespace bee {

namespace sz = ashvardanian::stringzilla;

using String     = sz::string;
using StringView = sz::string_view;

template<typename T>
concept CanFormatType = requires(T t) { fmt::format("{}", t); };

} // namespace bee

template<> class fmt::formatter<bee::String>
{
public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context> constexpr auto format(const bee::String& bs, Context& ctx) const { return format_to(ctx.out(), "{}", bs.data()); }
};

template<> class fmt::formatter<bee::StringView>
{
public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context> constexpr auto format(const bee::StringView& bsv, Context& ctx) const
    {
        return format_to(ctx.out(), "{}", bsv.data());
    }
};