#pragma once

// matmul 自由函数声明：支持 2D × 2D 及 N-D 批次矩阵乘（batch 维广播）

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// 矩阵乘法：a={...,M,K}，b={...,K,N} → 输出 {...,M,N}
// - 两侧 dtype/device 必须相同；
// - CPU 路径支持 F32/F64/I32/I64，另有 I8×I8→I32 的特化；
// - F16/BF16 输入自动提升至 F32（依据 dtype_accumulate_type(Matmul, F16/BF16)==F32），
//   输出 dtype 为 F32；CPU/CUDA 语义一致，不直接落入无对应内核的低精度后端路径；
// - 支持 2D × 2D 及 N-D 批次维广播；
// - CUDA 路径支持 F32/F64/I32/I64，批次维以分批循环方式调用底层 2D kernel。
[[nodiscard]] auto matmul(const Tensor& a, const Tensor& b) -> Result<Tensor>;

} // namespace bee
