#pragma once

// RoPE（Rotary Position Embedding）算子：对张量最后一维施加旋转位置编码。
//
// 约定（split-half 配对）：
//   输入形状为 [..., seq_len, dim]，dim 须为偶数；
//   沿倒数第二维（seq_len）枚举序列位置，从 position_offset 开始；
//   每一位置的旋转角 theta_i = (position_offset + pos) / base^(2i / dim)；
//   最后一维按 split-half 配对：(x[..., i], x[..., i + dim/2])，i ∈ [0, dim/2)；
//   旋转变换：(x0, x1) → (x0*cos(θ) - x1*sin(θ), x0*sin(θ) + x1*cos(θ))；
//   输出形状与输入完全相同。
//
// 接口约束：
//   x               ：任意 ndim >= 2，dtype 为 F32 或 F64，最后维须为偶数；
//   base            ：基数（默认 10000.0），须为有限正数（std::isfinite(base) && base > 0）；
//   position_offset ：序列偏移（整数，通常为 0 或 KV cache 长度）；
//
// CUDA 路径：
//   CUDA 输入会调用设备端 RoPE 后端；ctx 用于传递执行上下文，当前保持
//   同步可见语义。

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

// 返回应用 RoPE 后的新张量，输出 shape 与输入完全一致。
[[nodiscard]] auto apply_rope(const Tensor& x, double base, int64_t position_offset, const tensor::cuda::ExecContext* ctx = nullptr)
    -> Result<Tensor>;

} // namespace bee
