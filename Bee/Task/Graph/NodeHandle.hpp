/**
 * @File NodeHandle.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Core/Config.hpp"
#include "Base/Reflection/Nameof.hpp"

namespace bee
{

class TaskGraph;

namespace detail
{
    struct NodeId
    {
        u32 value;

        auto operator==(const NodeId&) const noexcept -> bool = default;
    };

} // namespace detail

/**
 * @brief 图节点的生命周期状态。
 *
 * 状态转换：
 *   Pending   → Ready     （所有依赖已完成）
 *   Ready     → Running   （已提交至线程池）
 *   Running   → Completed （可调用体成功返回）
 *   Running   → Failed    （可调用体抛出异常）
 *   Pending   → Cancelled （执行前图被取消）
 *   Ready     → Cancelled （执行前图被取消）
 *   Pending   → Failed    （前驱节点失败，本节点被跳过）
 *
 * 终态（Completed、Failed、Cancelled）不可逆。
 */
enum class NodeState : u8
{
    Pending,
    Ready,
    Running,
    Completed,
    Failed,
    Cancelled
};

BEE_ENUM_SCAN_RANGE(NodeState, 0, 5, false)

/**
 * @brief TaskGraph 中节点的类型化句柄。
 *
 * T 为节点的输出类型（即发即忘节点为 void）。
 * 由 add_node() 返回，传递给后续 add_node() 调用以声明依赖关系。
 *
 * NodeHandle 可拷贝——多个节点可以依赖同一个前驱。
 */
template <typename T>
class NodeHandle
{
public:
    NodeHandle(const NodeHandle&)                    = default;
    auto operator=(const NodeHandle&) -> NodeHandle& = default;

    [[nodiscard]] auto id() const noexcept -> detail::NodeId
    {
        return id_;
    }

private:
    explicit NodeHandle(detail::NodeId id) noexcept
        : id_(id)
    {
    }

    detail::NodeId id_;

    friend class TaskGraph;
};

} // namespace bee
