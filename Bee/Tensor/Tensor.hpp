#pragma once

#include <string_view>

namespace bee
{

// 返回 Tensor 组件名称
std::string_view tensor_name() noexcept;

} // namespace bee
