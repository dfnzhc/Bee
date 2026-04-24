#pragma once

// matmul 自由函数声明：仅支持 2D × 2D，输出连续张量

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// 矩阵乘法：a={M,K}，b={K,N} → 输出 {M,N}
// - 两侧 dtype/device 必须相同；
// - CPU 路径支持 F32/F64/I32/I64，另有 I8×I8→I32 的特化；
// - CUDA 路径支持 F32/F64/I32/I64，并会在必要时先整理为 contiguous。
[[nodiscard]] auto matmul(const Tensor& a, const Tensor& b) -> Result<Tensor>;

} // namespace bee
