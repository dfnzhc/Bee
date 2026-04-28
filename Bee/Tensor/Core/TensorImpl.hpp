#pragma once

#include <cstdint>
#include <memory>

#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"

namespace bee
{

class Storage;

// Tensor 内部数据节点。
//
// TensorImpl 描述一个张量视图：storage 持有底层内存，dtype/shape/strides
// 描述逻辑布局，offset 表示从 storage 起点开始的元素偏移。offset 与
// strides 都以元素为单位，不以字节为单位；同一个 storage 可被多个视图
// 通过不同 shape/strides/offset 共享。
struct TensorImpl
{
    std::shared_ptr<Storage> storage;
    DType                    dtype;
    Shape                    shape;
    Strides                  strides;    // 元素单位，非字节
    int64_t                  offset = 0; // 元素单位

    [[nodiscard]] auto numel() const noexcept -> int64_t
    {
        return ::bee::numel(shape);
    }
};

} // namespace bee
