#pragma once

// Cast 算子自由函数声明：
// 当前主路径面向 Bool/U8/I32/I64/F32/F64 这些已接通的计算类型；
// 其余扩展 dtype 是否可用取决于下层后端能力。

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// dtype 转换
//   - 相同 dtype：返回 clone()（独立 storage，contiguous）
//   - 输入非 contiguous：先整理为连续再转换
//   - 转换语义：任意类型 → Bool 时 v != 0；Bool → 其它时 true=1/false=0；
//     浮点/整型互转均用 static_cast（截断，无范围检查）
//   - F16/BF16 ↔ F32：CPU 上通过位编码辅助函数实现（参考精度）
[[nodiscard]] auto cast(const Tensor& src, DType dst_dtype, const tensor::cuda::ExecContext* ctx = nullptr) -> Result<Tensor>;

} // namespace bee
