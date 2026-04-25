#pragma once

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

// 沿指定维度计算稳定 softmax
// 直接支持 F32/F64。F16/BF16 提升为 F32 并返回 F32
// CUDA 输入当前通过同步 CPU 桥接并返回 CUDA
[[nodiscard]] auto softmax(const Tensor& x, int dim, const tensor::cuda::ExecContext* ctx = nullptr) -> Result<Tensor>;

} // namespace bee
