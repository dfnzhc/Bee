#pragma once

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

// logits 形状为 {N, C}、target 形状为 {N} 的类别索引 mean cross entropy。
// target dtype 必须为 I32 或 I64。F16/BF16 logits 会提升为 F32。
[[nodiscard]] auto cross_entropy(const Tensor& logits, const Tensor& target, const tensor::cuda::ExecContext* ctx = nullptr) -> Result<Tensor>;

} // namespace bee
