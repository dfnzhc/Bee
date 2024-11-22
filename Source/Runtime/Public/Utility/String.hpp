/**
 * @File String.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/21
 * @Brief This file is part of Bee.
 */

#pragma once

#include <stringzilla/stringzilla.hpp>

namespace bee {

namespace sz = ashvardanian::stringzilla;

using String     = sz::string;
using StringView = sz::string_view;

template<typename T>
concept CanFormatType = requires(T t) { fmt::format("{}", t); };

} // namespace bee