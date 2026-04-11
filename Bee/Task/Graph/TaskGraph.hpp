/**
 * @File TaskGraph.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include <any>
#include <atomic>
#include <memory>
#include <stop_token>
#include <type_traits>
#include <utility>
#include <vector>

#include "Base/Core/Defines.hpp"
#include "Base/Core/MoveOnlyFunction.hpp"
#include "Base/Diagnostics/Check.hpp"
#include "Task/Graph/NodeHandle.hpp"
#include "Task/Core/Task.hpp"

namespace bee
{

class ThreadPool;

namespace detail
{
    struct NodeEntry
    {
        MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable;

        std::vector<NodeId> predecessors;
        std::vector<NodeId> successors;

        std::atomic<NodeState> state{NodeState::Pending};
        std::atomic<u32> pending_deps{0};

        std::any result;
        std::exception_ptr exception;
    };

} // namespace detail

// =========================================================================
// TaskGraph — 静态、可复用的类型化节点 DAG 调度器
// =========================================================================

class TaskGraph
{
public:
    TaskGraph() = default;

    TaskGraph(const TaskGraph&)                    = delete;
    auto operator=(const TaskGraph&) -> TaskGraph& = delete;
    TaskGraph(TaskGraph&&)                         = delete;
    auto operator=(TaskGraph&&) -> TaskGraph&      = delete;

    // -----------------------------------------------------------------
    // 节点创建
    // -----------------------------------------------------------------

    /// 添加根节点（无依赖）。
    template <typename Fn>
    auto add_node(Fn&& fn) -> NodeHandle<std::invoke_result_t<Fn>>
    {
        using R     = std::invoke_result_t<Fn>;
        auto fn_ptr = std::make_shared<std::decay_t<Fn>>(std::forward<Fn>(fn));

        MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable = [fn_ptr](const std::vector<std::any>& /*inputs*/) -> std::any {
            if constexpr (std::is_void_v<R>) {
                (*fn_ptr)();
                return std::any{};
            } else {
                return std::any((*fn_ptr)());
            }
        };

        auto id = create_node(std::move(callable), {});
        return NodeHandle<R>(id);
    }

    /// 添加带类型化依赖的节点（1 个或多个）。
    /// fn 按顺序接收各依赖的结果值。
    template <typename Fn, typename... Ts>
        requires(sizeof...(Ts) >= 1) && (!std::is_void_v<Ts> && ...)
    auto add_node(Fn&& fn, const NodeHandle<Ts>&... deps) -> NodeHandle<std::invoke_result_t<Fn, Ts...>>
    {
        using R     = std::invoke_result_t<Fn, Ts...>;
        auto fn_ptr = std::make_shared<std::decay_t<Fn>>(std::forward<Fn>(fn));

        MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable = [fn_ptr](const std::vector<std::any>& inputs) -> std::any {
            return [&]<size_t... Is>(std::index_sequence<Is...>) -> std::any {
                if constexpr (std::is_void_v<R>) {
                    (*fn_ptr)(std::any_cast<const Ts&>(inputs[Is])...);
                    return std::any{};
                } else {
                    return std::any((*fn_ptr)(std::any_cast<const Ts&>(inputs[Is])...));
                }
            }(std::index_sequence_for<Ts...>{});
        };

        auto id = create_node(std::move(callable), {deps.id()...});
        return NodeHandle<R>(id);
    }

    /// 添加仅用于排序的 void 依赖节点。
    /// fn 不接收参数；在所有依赖完成后运行。
    template <typename Fn, typename... Deps>
        requires(sizeof...(Deps) >= 1) && (std::same_as<Deps, NodeHandle<void>> && ...)
    auto add_node_after(Fn&& fn, const Deps&... deps) -> NodeHandle<std::invoke_result_t<Fn>>
    {
        using R     = std::invoke_result_t<Fn>;
        auto fn_ptr = std::make_shared<std::decay_t<Fn>>(std::forward<Fn>(fn));

        MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable = [fn_ptr](const std::vector<std::any>& /*inputs*/) -> std::any {
            if constexpr (std::is_void_v<R>) {
                (*fn_ptr)();
                return std::any{};
            } else {
                return std::any((*fn_ptr)());
            }
        };

        auto id = create_node(std::move(callable), {deps.id()...});
        return NodeHandle<R>(id);
    }

    // -----------------------------------------------------------------
    // 结果访问
    // -----------------------------------------------------------------

    /// 在执行完成后获取节点的结果。
    /// 仅对处于 Completed 状态的非 void 节点有效。
    template <typename T>
        requires(!std::is_void_v<T>)
    [[nodiscard]] auto result(const NodeHandle<T>& handle) const -> const T&
    {
        BEE_CHECK(handle.id().value < nodes_.size());
        auto& node = nodes_[handle.id().value];
        BEE_CHECK(node->state.load(std::memory_order_acquire) == NodeState::Completed);
        return std::any_cast<const T&>(node->result);
    }

    // -----------------------------------------------------------------
    // 执行
    // -----------------------------------------------------------------

    /// 在线程池上执行图。
    /// 返回当所有节点完成时完成的 Task<void>。
    auto execute(ThreadPool& pool) -> Task<void>;

    /// 带取消支持的执行。
    auto execute(ThreadPool& pool, std::stop_token token) -> Task<void>;

    // -----------------------------------------------------------------
    // 查询
    // -----------------------------------------------------------------

    /// 图中的节点数量。
    [[nodiscard]] auto node_count() const noexcept -> size_t;

    /// 通过 Kahn 算法检测环。
    [[nodiscard]] auto has_cycle() const -> bool;

    /// 图为空时返回 true。
    [[nodiscard]] auto empty() const noexcept -> bool;

private:
    auto create_node(MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable, std::vector<detail::NodeId> deps) -> detail::NodeId;

    auto execute_impl(ThreadPool& pool, std::stop_token token) -> Task<void>;

    std::vector<std::unique_ptr<detail::NodeEntry>> nodes_;
    std::atomic<bool> executing_{false};
};

} // namespace bee
