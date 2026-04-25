#pragma once

// Tensor 组件门面头文件：聚合 Core 元数据、公开算子与 CUDA 桥接接口

#include "Base/Memory/Device.hpp"
#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"
#include "Base/Memory/Allocator.hpp"
#include "Tensor/Core/Storage.hpp"
#include "Tensor/Core/TensorImpl.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Ops/Broadcast.hpp"
#include "Tensor/Ops/Cast.hpp"
#include "Tensor/Ops/ElementWise.hpp"
#include "Tensor/Ops/Softmax.hpp"
#include "Tensor/Ops/Matmul.hpp"
#include "Tensor/Ops/Random.hpp"
#include "Tensor/Ops/Reduce.hpp"
#include "Tensor/Ops/Embedding.hpp"
#include "Tensor/Ops/Norm.hpp"
#include "Tensor/Ops/Positional.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <string_view>

namespace bee
{

// 返回 Tensor 组件名称
std::string_view tensor_name() noexcept;

} // namespace bee
