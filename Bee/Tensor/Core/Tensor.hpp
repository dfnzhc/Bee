#pragma once

#include <cstdint>
#include <memory>

#include "Base/Diagnostics/Error.hpp"
#include "Tensor/Core/Device.hpp"
#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"
#include "Tensor/Core/TensorImpl.hpp"

namespace bee
{

class Storage;

// 用户面向的 Tensor 外壳：值语义，内部通过 shared_ptr<TensorImpl> 共享数据
// 不包含算子、视图变换（属于后续任务）
class Tensor
{
public:
    // 默认构造：未定义状态（defined() == false）
    Tensor() = default;

    // 工厂：在指定设备上分配未初始化的连续内存
    [[nodiscard]] static auto empty(Shape shape, DType dtype, Device device = Device::CPU)
        -> Result<Tensor>;

    // ── 查询访问器 ──────────────────────────────────────────────────────────

    [[nodiscard]] auto defined() const noexcept -> bool;
    [[nodiscard]] auto ndim() const noexcept -> int64_t;
    [[nodiscard]] auto numel() const noexcept -> int64_t;
    [[nodiscard]] auto shape() const noexcept -> const Shape&;
    [[nodiscard]] auto strides() const noexcept -> const Strides&;
    [[nodiscard]] auto dtype() const noexcept -> DType;
    [[nodiscard]] auto device() const noexcept -> Device;
    [[nodiscard]] auto is_contiguous() const noexcept -> bool;
    [[nodiscard]] auto storage_offset() const noexcept -> int64_t;

    // 原始数据指针（已加上元素偏移）
    [[nodiscard]] auto data_ptr() noexcept -> void*;
    [[nodiscard]] auto data_ptr() const noexcept -> const void*;

    // 内部结构访问（后续视图/算子任务使用）
    [[nodiscard]] auto storage() const noexcept -> const std::shared_ptr<Storage>&;
    [[nodiscard]] auto impl() const noexcept -> const std::shared_ptr<TensorImpl>&;

    // 深拷贝；MVP 仅支持连续张量，非连续或 CUDA 路径返回 Err
    [[nodiscard]] auto clone() const -> Result<Tensor>;

private:
    explicit Tensor(std::shared_ptr<TensorImpl> impl);

    std::shared_ptr<TensorImpl> impl_;
};

} // namespace bee
