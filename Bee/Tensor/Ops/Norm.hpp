#pragma once

// RMSNorm 算子：Root Mean Square Layer Normalization。
//
// 公式：y = x / sqrt(mean(x^2) + eps) * weight
//
// 接口约束：
//   x      ：任意形状，dtype 为 F32 或 F64；
//   weight ：1-D 张量，形状为 [d]，d 须等于 x 最后一维；
//   eps    ：数值稳定项，须为有限正数（std::isfinite(eps) && eps > 0）；
//   设备约束：x 与 weight 须在同一设备；输出与输入同设备。
//   输出形状与 x.shape 一致。
//
// CUDA 路径：
//   当 x 与 weight 均位于 CUDA 设备时，调用设备端 RMSNorm 后端；ctx 用于
//   传递执行上下文，当前保持同步可见语义。

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

// 返回与 x 同 shape 的 RMSNorm 结果张量。
[[nodiscard]] auto rms_norm(const Tensor& x, const Tensor& weight, double eps, const tensor::cuda::ExecContext* ctx = nullptr) -> Result<Tensor>;

} // namespace bee
