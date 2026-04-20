#pragma once

#include <cstdint>
#include <memory>

#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"

namespace bee
{

class Storage;

// Tensor 内部数据节点：持有 storage、数据类型、shape/strides 及存储偏移
struct TensorImpl
{
    std::shared_ptr<Storage> storage;
    DType   dtype;
    Shape   shape;
    Strides strides;    // 元素单位，非字节
    int64_t offset = 0; // 元素单位

    [[nodiscard]] auto numel() const noexcept -> int64_t
    {
        return ::bee::numel(shape);
    }
};

} // namespace bee
