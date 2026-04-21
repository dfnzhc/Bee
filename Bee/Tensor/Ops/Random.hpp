#pragma once

// Random 初始化算子自由函数声明：rand / randn / randint
// 基于 std::mt19937_64；seed==0 时使用 std::random_device 生成随机种子

#include "Base/Diagnostics/Error.hpp"
#include "Base/Memory/Device.hpp"
#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"
#include "Tensor/Core/Tensor.hpp"

namespace bee
{

// 均匀分布 [0, 1)；dtype 仅支持 F32/F64，其他返回 Err
[[nodiscard]] auto rand(Shape shape, DType dtype = DType::F32, uint64_t seed = 0,
                        Device device = Device::CPU) -> Result<Tensor>;

// 标准正态分布 N(0, 1)；dtype 仅支持 F32/F64，其他返回 Err
[[nodiscard]] auto randn(Shape shape, DType dtype = DType::F32, uint64_t seed = 0,
                         Device device = Device::CPU) -> Result<Tensor>;

// 离散均匀分布 [low, high)；dtype 支持 I32/I64/U8（U8 要求 low >= 0）；
// low >= high 或 dtype 不合法时返回 Err
[[nodiscard]] auto randint(int64_t low, int64_t high, Shape shape, DType dtype = DType::I64,
                           uint64_t seed = 0, Device device = Device::CPU) -> Result<Tensor>;

} // namespace bee
