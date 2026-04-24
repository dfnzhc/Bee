#pragma once

// RMSNorm 算子：Root Mean Square Layer Normalization。
//
// 公式：y = x / sqrt(mean(x^2) + eps) * weight
//
// 接口约束：
//   x      ：任意形状，dtype 为 F32 或 F64；
//   weight ：1-D 张量，形状为 [d]，d 须等于 x 最后一维；
//   eps    ：数值稳定项，须 > 0；
//   输出形状与 x.shape 一致。
//
// CUDA 过渡路径：
//   当前仅有 CPU 原生实现。若输入在 CUDA 设备上，先迁移至 CPU 计算，
//   再将结果迁移回原设备（同步语义）。

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

[[nodiscard]] auto rms_norm(
    const Tensor&                        x,
    const Tensor&                        weight,
    double                               eps,
    const tensor::cuda::ExecContext*     ctx = nullptr
) -> Result<Tensor>;

} // namespace bee
