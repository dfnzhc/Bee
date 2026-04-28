#pragma once

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

// 沿指定维度计算数值稳定 softmax。
// 直接支持 F32/F64；F16/BF16 会提升为 F32 并返回 F32。CUDA 输入使用
// 设备端 softmax 后端，ctx 用于传递执行上下文，当前保持同步可见语义。
[[nodiscard]] auto softmax(const Tensor& x, int dim, const tensor::cuda::ExecContext* ctx = nullptr) -> Result<Tensor>;

} // namespace bee
