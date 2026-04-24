#pragma once

// Embedding 查表算子：按整数 token_ids 从权重矩阵中索引行向量。
//
// 接口约束：
//   weight    ：必须为 2-D，形状 [vocab, hidden]，dtype 为 F32 或 F64；
//   token_ids ：任意形状的整数张量（I32 或 I64）；
//   输出形状  ：token_ids.shape + [hidden]；
//   越界 id   ：返回 Recoverable 错误；
//   设备约束  ：weight 与 token_ids 须在同一设备；输出与输入同设备。
//
// CUDA 过渡路径：
//   当前仅有 CPU 原生实现。若两者均在 CUDA 设备上，
//   函数会先将其迁移至 CPU 完成计算，再将结果迁移回 CUDA（同步语义）。

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Cuda/Backend.hpp"

namespace bee
{

[[nodiscard]] auto embedding(
    const Tensor&                        weight,
    const Tensor&                        token_ids,
    const tensor::cuda::ExecContext*     ctx = nullptr
) -> Result<Tensor>;

} // namespace bee
