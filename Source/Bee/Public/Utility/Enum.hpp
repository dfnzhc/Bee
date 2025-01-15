/**
 * @File Enum.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

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

} // namespace bee