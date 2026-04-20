#pragma once

// Cast 算子自由函数声明：支持全 6 种 dtype 两两互转（36 组合）

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// dtype 转换
//   - 相同 dtype：返回 clone()（独立 storage，contiguous）
//   - 输入非 contiguous：先整理为连续再转换
//   - 转换语义：任意类型 → Bool 时 v != 0；Bool → 其它时 true=1/false=0；
//     浮点/整型互转均用 static_cast（截断，无范围检查）
[[nodiscard]] auto cast(const Tensor& src, DType dst_dtype) -> Result<Tensor>;

} // namespace bee
