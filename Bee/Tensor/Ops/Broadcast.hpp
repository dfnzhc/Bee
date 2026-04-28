#pragma once

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Shape.hpp"

namespace bee
{

// 按 NumPy 广播规则计算两 shape 的广播结果 shape。
// 规则从最右维开始对齐；每一维要求长度相等或其中一方为 1，否则返回
// 可恢复错误。该函数只计算 shape，不分配张量或修改输入。
[[nodiscard]] auto compute_broadcast_shape(const Shape& a, const Shape& b) -> Result<Shape>;

} // namespace bee
