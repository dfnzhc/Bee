#pragma once

// Reduce 算子自由函数声明：
//   - 全局 reduce（返回 shape={} 的 0-rank 标量张量，numel=1）
//   - 按轴 reduce（接受单一 dim，keepdim 默认 false）
//
// dtype 支持矩阵：
//   sum/prod  ： F32/F64/I32/I64（Bool/U8 → Err）
//   min/max   ： F32/F64/I32/I64/U8（Bool → Err）
//   mean      ： F32→F32, F64→F64, I32/I64→F64（Bool/U8 → Err）
//
// CUDA 侧说明：
// - 全局 reduce 已接入设备端规约；
// - axis reduce 也支持 CUDA，但实现仍比全局 reduce 更朴素；
// - mean 的 I32/I64 → F64 目前只在 CPU 路径实现，CUDA 路径返回可恢复错误。

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// ─── 全局 reduce（返回 0-rank 标量张量）───────────────────────────────────────

// 对全部元素求和，返回 0-rank 标量张量。
[[nodiscard]] auto sum(const Tensor& a) -> Result<Tensor>;
// 对全部元素求平均值，返回 0-rank 标量张量。
[[nodiscard]] auto mean(const Tensor& a) -> Result<Tensor>;
// 对全部元素求最小值，返回 0-rank 标量张量。
[[nodiscard]] auto min(const Tensor& a) -> Result<Tensor>;
// 对全部元素求最大值，返回 0-rank 标量张量。
[[nodiscard]] auto max(const Tensor& a) -> Result<Tensor>;
// 对全部元素求乘积，返回 0-rank 标量张量。
[[nodiscard]] auto prod(const Tensor& a) -> Result<Tensor>;

// ─── 按轴 reduce（单一 dim，支持负索引；keepdim=false 时移除该维）────────────

// 沿 dim 求和；keepdim 为 true 时保留长度为 1 的规约维度。
[[nodiscard]] auto sum(const Tensor& a, int dim, bool keepdim = false) -> Result<Tensor>;
// 沿 dim 求平均值。
[[nodiscard]] auto mean(const Tensor& a, int dim, bool keepdim = false) -> Result<Tensor>;
// 沿 dim 求最小值。
[[nodiscard]] auto min(const Tensor& a, int dim, bool keepdim = false) -> Result<Tensor>;
// 沿 dim 求最大值。
[[nodiscard]] auto max(const Tensor& a, int dim, bool keepdim = false) -> Result<Tensor>;
// 沿 dim 求乘积。
[[nodiscard]] auto prod(const Tensor& a, int dim, bool keepdim = false) -> Result<Tensor>;

} // namespace bee
