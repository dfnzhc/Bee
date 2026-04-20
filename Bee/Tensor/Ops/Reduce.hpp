#pragma once

// Reduce 算子自由函数声明：
//   - 全局 reduce（返回 shape={} 的 0-rank 标量张量，numel=1）
//   - 按轴 reduce（接受单一 dim，keepdim 默认 false）
//
// dtype 支持矩阵：
//   sum/prod  ： F32/F64/I32/I64（Bool/U8 → Err）
//   min/max   ： F32/F64/I32/I64/U8（Bool → Err）
//   mean      ： F32→F32, F64→F64, I32/I64→F64（Bool/U8 → Err）

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// ─── 全局 reduce（返回 0-rank 标量张量）───────────────────────────────────────

[[nodiscard]] auto sum(const Tensor& a)  -> Result<Tensor>;
[[nodiscard]] auto mean(const Tensor& a) -> Result<Tensor>;
[[nodiscard]] auto min(const Tensor& a)  -> Result<Tensor>;
[[nodiscard]] auto max(const Tensor& a)  -> Result<Tensor>;
[[nodiscard]] auto prod(const Tensor& a) -> Result<Tensor>;

// ─── 按轴 reduce（单一 dim，支持负索引；keepdim=false 时移除该维）────────────

[[nodiscard]] auto sum(const Tensor& a,  int dim, bool keepdim = false) -> Result<Tensor>;
[[nodiscard]] auto mean(const Tensor& a, int dim, bool keepdim = false) -> Result<Tensor>;
[[nodiscard]] auto min(const Tensor& a,  int dim, bool keepdim = false) -> Result<Tensor>;
[[nodiscard]] auto max(const Tensor& a,  int dim, bool keepdim = false) -> Result<Tensor>;
[[nodiscard]] auto prod(const Tensor& a, int dim, bool keepdim = false) -> Result<Tensor>;

} // namespace bee
