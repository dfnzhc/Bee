#pragma once

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Shape.hpp"

namespace bee
{

// 按 NumPy 广播规则计算两 shape 的广播结果 shape
// 从右对齐；每维要求相等或其中一个为 1，否则返回错误
[[nodiscard]] auto compute_broadcast_shape(const Shape& a, const Shape& b) -> Result<Shape>;

} // namespace bee
