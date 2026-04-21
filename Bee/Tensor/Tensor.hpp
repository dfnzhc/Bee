#pragma once

// Tensor 组件门面头文件：聚合所有 Core 元数据类型

#include "Tensor/Core/Device.hpp"
#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"
#include "Tensor/Core/Allocator.hpp"
#include "Tensor/Core/Storage.hpp"
#include "Tensor/Core/TensorImpl.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Ops/Broadcast.hpp"
#include "Tensor/Ops/Cast.hpp"
#include "Tensor/Ops/ElementWise.hpp"
#include "Tensor/Ops/Matmul.hpp"
#include "Tensor/Ops/Random.hpp"
#include "Tensor/Ops/Reduce.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <string_view>

namespace bee
{

// 返回 Tensor 组件名称
std::string_view tensor_name() noexcept;

} // namespace bee
