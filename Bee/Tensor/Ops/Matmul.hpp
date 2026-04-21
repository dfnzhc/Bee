#pragma once

// matmul 自由函数声明：仅支持 2D × 2D，输出连续张量

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// 矩阵乘法：a={M,K}，b={K,N} → 输出 {M,N}
// 仅支持 F32/F64/I32/I64，两侧 dtype/device 必须相同且为 CPU
[[nodiscard]] auto matmul(const Tensor& a, const Tensor& b) -> Result<Tensor>;

} // namespace bee
