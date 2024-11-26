/**
 * @File Enum.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/19
 * @Brief This file is part of Bee.
 */

#pragma once

#include <magic_enum/magic_enum_all.hpp>

namespace bee {

namespace me = magic_enum;
namespace mec = magic_enum::containers;
using namespace magic_enum::bitwise_operators;

using me::iostream_operators::operator<<;
using me::iostream_operators::operator>>;

} // namespace bee