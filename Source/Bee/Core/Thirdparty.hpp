/**
 * @File Thirdparty.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <magic_enum/magic_enum_all.hpp>
#include <stringzilla/stringzilla.hpp>

namespace bee {
namespace me = magic_enum;
namespace mec = magic_enum::containers;
#define BEE_USE_MAGIC_ENUM_BIT_OPERATOR using namespace magic_enum::bitwise_operators

using me::iostream_operators::operator<<;
using me::iostream_operators::operator>>;

template<typename E>
    requires std::is_enum_v<E>
constexpr std::string_view ToString(E e)
{
    return me::enum_name(e);
}

template<typename E>
    requires std::is_enum_v<E>
constexpr bool InSet(E mask, E e)
{
    BEE_USE_MAGIC_ENUM_BIT_OPERATOR;
    return (mask & e) == e;
}

namespace sz = ashvardanian::stringzilla;

using String     = sz::string;
using StringView = sz::string_view;

template<typename T>
concept CanFormatType = requires(T t) { std::format("{}", t); };

} // namespace bee

template<> struct std::formatter<bee::String>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context> constexpr auto format(const bee::String& bs, Context& ctx) const { return format_to(ctx.out(), "{}", bs.data()); }
};

template<> struct std::formatter<bee::StringView>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context> constexpr auto format(const bee::StringView& bsv, Context& ctx) const { return format_to(ctx.out(), "{}", bsv.data()); }
};