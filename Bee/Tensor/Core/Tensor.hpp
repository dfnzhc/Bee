#pragma once

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <span>

#include "Base/Diagnostics/Error.hpp"
#include "Base/Memory/Device.hpp"
#include "Tensor/Core/DType.hpp"
#include "Tensor/Core/Shape.hpp"
#include "Tensor/Core/TensorImpl.hpp"

namespace bee
{

class Storage;

namespace tensor::cuda
{
struct ExecContext;
}


// 用户面向的 Tensor 外壳：值语义，内部通过 shared_ptr<TensorImpl> 共享数据
class Tensor
{
public:
    // 默认构造：未定义状态（defined() == false）
    Tensor() = default;

    // 工厂：在指定设备上分配未初始化的连续内存
    [[nodiscard]] static auto empty(Shape shape, DType dtype, Device device = Device::CPU) -> Result<Tensor>;

    // 工厂：所有元素置零
    [[nodiscard]] static auto zeros(Shape shape, DType dtype, Device device = Device::CPU) -> Result<Tensor>;

    // 工厂：所有元素填充为 1（按 dtype 解释）
    [[nodiscard]] static auto ones(Shape shape, DType dtype, Device device = Device::CPU) -> Result<Tensor>;

    // 工厂：所有元素填充为指定标量值（value 以 double 传入，按 dtype 转换）
    [[nodiscard]] static auto full(Shape shape, DType dtype, double value, Device device = Device::CPU) -> Result<Tensor>;

    // 工厂：生成 1D 等差序列 [start, start+step, ..., < end)，dtype 默认 I64
    [[nodiscard]] static auto arange(int64_t start, int64_t end, int64_t step = 1, DType dtype = DType::I64, Device device = Device::CPU)
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

    // 内部结构访问
    [[nodiscard]] auto storage() const noexcept -> const std::shared_ptr<Storage>&;
    [[nodiscard]] auto impl() const noexcept -> const std::shared_ptr<TensorImpl>&;

    // 深拷贝：始终返回 contiguous 的独立 storage（支持非连续张量）
    [[nodiscard]] auto clone() const -> Result<Tensor>;

    // 设备迁移：在目标设备上分配等形状张量并搬运数据。
    // 若已在目标设备上则直接返回浅拷贝；非连续张量会先 contiguous() 再搬运。
    [[nodiscard]] auto to(Device target) const -> Result<Tensor>;
    
    // 异步设备迁移（接受执行上下文）。
    [[nodiscard]] auto to(Device target, const bee::tensor::cuda::ExecContext* exec_context) const -> Result<Tensor>;

    // ── 视图与形状变换（零拷贝，除非另行说明）──────────────────────────────

    // 返回新形状的视图；要求当前张量 contiguous；支持一个 -1 占位推断
    [[nodiscard]] auto view(Shape new_shape) const -> Result<Tensor>;

    // 重塑形状；非 contiguous 时先物化为 contiguous，再返回新的连续视图
    [[nodiscard]] auto reshape(Shape new_shape) const -> Result<Tensor>;

    // 返回连续布局张量；若已连续则共享 storage，否则重新排列数据。
    // 当前 CPU 有 2D 专用快路径；CUDA 仅对部分 2D transpose 形态做设备端物化。
    [[nodiscard]] auto contiguous() const -> Result<Tensor>;

    // 维度排列；dims 为 {0..ndim-1} 的一个排列
    [[nodiscard]] auto permute(std::span<const int> dims) const -> Result<Tensor>;
    [[nodiscard]] auto permute(std::initializer_list<int> dims) const -> Result<Tensor>;

    // 交换两个维度；支持负数索引
    [[nodiscard]] auto transpose(int dim0, int dim1) const -> Result<Tensor>;

    // 移除大小为 1 的指定维度；若该维度 size != 1 则返回浅拷贝（不报错）
    [[nodiscard]] auto squeeze(int dim) const -> Result<Tensor>;

    // 在 dim 处插入大小为 1 的维度；dim 范围 [-ndim-1, ndim]
    [[nodiscard]] auto unsqueeze(int dim) const -> Result<Tensor>;

    // 在指定维度上切片（半开区间 [start, end)，步长 step >= 1）
    [[nodiscard]] auto slice(int dim, int64_t start, int64_t end, int64_t step = 1) const -> Result<Tensor>;

private:
    explicit Tensor(std::shared_ptr<TensorImpl> impl);

    std::shared_ptr<TensorImpl> impl_;
};

} // namespace bee
