/**
 * @File TaskGraph.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <any>
#include <atomic>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <format>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Base/Core/MoveOnlyFunction.hpp"
#include "Base/Reflection/Nameof.hpp"
#include "Task/Core/Scheduler.hpp"
#include "Task/Core/Task.hpp"

namespace bee
{

// =========================================================================
// NodeHandle<T> — 类型安全的节点句柄
// =========================================================================

template <typename T>
struct NodeHandle
{
    std::size_t index;
};

namespace detail
{

    // =====================================================================
    // NodeEntry — 节点的内部统一表示
    // =====================================================================

    struct NodeEntry
    {
        MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable;
        std::vector<std::size_t> predecessors;
        std::vector<std::size_t> successors;
        std::any result;
        std::exception_ptr exception;
        std::string label;
    };

    // =====================================================================
    // extract_args — 从 std::vector<std::any> 按类型提取参数元组
    // =====================================================================

    template <typename... Ts, std::size_t... Is>
    auto extract_args_impl(const std::vector<std::any>& inputs, std::index_sequence<Is...>)
        -> std::tuple<Ts...>
    {
        return std::tuple<Ts...>{std::any_cast<Ts>(inputs[Is])...};
    }

    template <typename... Ts>
    auto extract_args(const std::vector<std::any>& inputs) -> std::tuple<Ts...>
    {
        return extract_args_impl<Ts...>(inputs, std::index_sequence_for<Ts...>{});
    }

    // =====================================================================
    // GraphExecutionContext — 图执行的共享状态
    // =====================================================================

    template <Scheduler S>
    struct GraphExecutionContext
    {
        std::vector<NodeEntry>& nodes;
        S& scheduler;
        std::vector<std::atomic<std::size_t>> counters;
        std::atomic<std::size_t> remaining;
        std::coroutine_handle<> continuation{nullptr};
        std::exception_ptr first_exception{nullptr};
        std::mutex exception_mutex;

        GraphExecutionContext(std::vector<NodeEntry>& n, S& s)
            : nodes(n), scheduler(s), counters(n.size()), remaining(n.size())
        {
            for (std::size_t i = 0; i < n.size(); ++i) {
                counters[i].store(n[i].predecessors.size(), std::memory_order_relaxed);
            }
        }
    };

    // =====================================================================
    // run_node — 执行单个节点并推进后继
    // =====================================================================

    template <Scheduler S>
    auto run_node(std::shared_ptr<GraphExecutionContext<S>> ctx, std::size_t index) -> void
    {
        auto& node = ctx->nodes[index];

        // 检查前驱是否有异常（级联失败）
        bool predecessor_failed = false;
        for (auto pred : node.predecessors) {
            if (ctx->nodes[pred].exception) {
                node.exception     = ctx->nodes[pred].exception;
                predecessor_failed = true;
                break;
            }
        }

        if (!predecessor_failed) {
            try {
                // 收集前驱结果
                std::vector<std::any> inputs;
                inputs.reserve(node.predecessors.size());
                for (auto pred : node.predecessors) {
                    inputs.push_back(ctx->nodes[pred].result);
                }
                node.result = node.callable(inputs);
            }
            catch (...) {
                node.exception = std::current_exception();
            }
        }

        // 存储第一个异常（线程安全）
        if (node.exception) {
            std::lock_guard lock(ctx->exception_mutex);
            if (!ctx->first_exception) {
                ctx->first_exception = node.exception;
            }
        }

        // 通知后继节点
        // note: node.result/exception 的写入通过 counters[succ].fetch_sub(acq_rel) 对后继可见
        for (auto succ : node.successors) {
            if (ctx->counters[succ].fetch_sub(1, std::memory_order_acq_rel) == 1) {
                ctx->scheduler.post([ctx, succ]() {
                    run_node(ctx, succ);
                });
            }
        }

        // 全局计数递减
        if (ctx->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            ctx->continuation.resume();
        }
    }

    // =====================================================================
    // GraphAwaitable — 协程挂起/恢复桥接
    // =====================================================================

    template <Scheduler S>
    struct GraphAwaitable
    {
        std::shared_ptr<GraphExecutionContext<S>> ctx;

        [[nodiscard]] auto await_ready() const noexcept -> bool
        {
            return false;
        }

        auto await_suspend(std::coroutine_handle<> h) -> void
        {
            // 复制 ctx 到局部变量：一旦 post 后工作线程可能 resume 协程，
            // 导致 GraphAwaitable 临时对象被销毁，this->ctx 悬垂。
            auto local_ctx        = ctx;
            local_ctx->continuation = h;

            // 将所有根节点（无前驱）post 到 scheduler
            for (std::size_t i = 0; i < local_ctx->nodes.size(); ++i) {
                if (local_ctx->counters[i].load(std::memory_order_relaxed) == 0) {
                    local_ctx->scheduler.post([local_ctx, i]() {
                        run_node(local_ctx, i);
                    });
                }
            }
        }

        auto await_resume() const noexcept -> void {}
    };

} // namespace detail

// =========================================================================
// TaskGraph — DAG 构建器 + 执行器
// =========================================================================

class TaskGraph
{
public:
    // ===== 根节点（无依赖）=====

    template <typename Fn>
    auto node(Fn fn) -> NodeHandle<std::invoke_result_t<Fn>>
    {
        using R = std::invoke_result_t<Fn>;
        return add_node_impl<R>(
            make_label<R>(nodes_.size(), ""),
            [fn = std::move(fn)](const std::vector<std::any>&) -> std::any {
                if constexpr (std::is_void_v<R>) {
                    fn();
                    return std::any{};
                }
                else {
                    return std::any{fn()};
                }
            },
            {});
    }

    template <typename Fn>
    auto node(std::string_view label, Fn fn) -> NodeHandle<std::invoke_result_t<Fn>>
    {
        using R = std::invoke_result_t<Fn>;
        return add_node_impl<R>(
            std::string(label),
            [fn = std::move(fn)](const std::vector<std::any>&) -> std::any {
                if constexpr (std::is_void_v<R>) {
                    fn();
                    return std::any{};
                }
                else {
                    return std::any{fn()};
                }
            },
            {});
    }

    // ===== 类型化依赖节点 =====

    template <typename Fn, typename... Deps>
    auto node(Fn fn, NodeHandle<Deps>... deps) -> NodeHandle<std::invoke_result_t<Fn, Deps...>>
    {
        using R = std::invoke_result_t<Fn, Deps...>;
        return add_node_impl<R>(
            make_label<R>(nodes_.size(), ""),
            [fn = std::move(fn)](const std::vector<std::any>& inputs) -> std::any {
                auto args = detail::extract_args<Deps...>(inputs);
                if constexpr (std::is_void_v<R>) {
                    std::apply(fn, std::move(args));
                    return std::any{};
                }
                else {
                    return std::any{std::apply(fn, std::move(args))};
                }
            },
            {deps.index...});
    }

    template <typename Fn, typename... Deps>
    auto node(std::string_view label, Fn fn, NodeHandle<Deps>... deps)
        -> NodeHandle<std::invoke_result_t<Fn, Deps...>>
    {
        using R = std::invoke_result_t<Fn, Deps...>;
        return add_node_impl<R>(
            std::string(label),
            [fn = std::move(fn)](const std::vector<std::any>& inputs) -> std::any {
                auto args = detail::extract_args<Deps...>(inputs);
                if constexpr (std::is_void_v<R>) {
                    std::apply(fn, std::move(args));
                    return std::any{};
                }
                else {
                    return std::any{std::apply(fn, std::move(args))};
                }
            },
            {deps.index...});
    }

    // ===== void 依赖节点（不消费前驱结果）=====

    template <typename Fn, typename... Deps>
    auto node_after(Fn fn, NodeHandle<Deps>... deps) -> NodeHandle<std::invoke_result_t<Fn>>
    {
        using R = std::invoke_result_t<Fn>;
        return add_node_impl<R>(
            make_label<R>(nodes_.size(), ""),
            [fn = std::move(fn)](const std::vector<std::any>&) -> std::any {
                if constexpr (std::is_void_v<R>) {
                    fn();
                    return std::any{};
                }
                else {
                    return std::any{fn()};
                }
            },
            {deps.index...});
    }

    template <typename Fn, typename... Deps>
    auto node_after(std::string_view label, Fn fn, NodeHandle<Deps>... deps)
        -> NodeHandle<std::invoke_result_t<Fn>>
    {
        using R = std::invoke_result_t<Fn>;
        return add_node_impl<R>(
            std::string(label),
            [fn = std::move(fn)](const std::vector<std::any>&) -> std::any {
                if constexpr (std::is_void_v<R>) {
                    fn();
                    return std::any{};
                }
                else {
                    return std::any{fn()};
                }
            },
            {deps.index...});
    }

    // ===== 查询 API =====

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return nodes_.empty();
    }

    [[nodiscard]] auto node_count() const noexcept -> std::size_t
    {
        return nodes_.size();
    }

    [[nodiscard]] auto has_cycle() const -> bool
    {
        if (nodes_.empty())
            return false;

        // Kahn 算法
        std::vector<std::size_t> in_degree(nodes_.size(), 0);
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            in_degree[i] = nodes_[i].predecessors.size();
        }

        std::vector<std::size_t> queue;
        for (std::size_t i = 0; i < in_degree.size(); ++i) {
            if (in_degree[i] == 0) {
                queue.push_back(i);
            }
        }

        std::size_t count = 0;
        while (!queue.empty()) {
            auto n = queue.back();
            queue.pop_back();
            ++count;
            for (auto s : nodes_[n].successors) {
                if (--in_degree[s] == 0) {
                    queue.push_back(s);
                }
            }
        }

        return count != nodes_.size();
    }

    // ===== DOT 输出 =====

    [[nodiscard]] auto to_dot() const -> std::string
    {
        std::ostringstream os;
        os << "digraph TaskGraph {\n";
        os << "    rankdir=TB;\n";
        os << "    node [shape=box, style=rounded];\n";

        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            os << "    " << i << " [label=\"" << nodes_[i].label << "\"];\n";
        }

        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            for (auto pred : nodes_[i].predecessors) {
                os << "    " << pred << " -> " << i << ";\n";
            }
        }

        os << "}\n";
        return os.str();
    }

    // ===== 结果访问 =====

    template <typename T>
    [[nodiscard]] auto result(NodeHandle<T> handle) const -> const T&
    {
        BEE_CHECK(handle.index < nodes_.size());
        const auto& entry = nodes_[handle.index];
        if (entry.exception) {
            throw std::logic_error("TaskGraph: cannot access result of a failed node");
        }
        return std::any_cast<const T&>(entry.result);
    }

    // ===== 执行 API（Task 2 实现）=====

    template <Scheduler S>
    auto execute(S& scheduler) -> Task<void>;

private:
    std::vector<detail::NodeEntry> nodes_;

    template <typename R>
    auto add_node_impl(
        std::string label,
        MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable,
        std::vector<std::size_t> predecessors) -> NodeHandle<R>
    {
        auto idx    = nodes_.size();
        auto& entry = nodes_.emplace_back();
        entry.callable     = std::move(callable);
        entry.predecessors = std::move(predecessors);
        entry.label        = std::move(label);

        // 更新前驱节点的 successors
        for (auto pred : entry.predecessors) {
            nodes_[pred].successors.push_back(idx);
        }

        return NodeHandle<R>{idx};
    }

    template <typename R>
    static auto make_label(std::size_t idx, std::string_view user_label) -> std::string
    {
        if (!user_label.empty()) {
            return std::string(user_label);
        }
        if constexpr (std::is_void_v<R>) {
            return std::format("node_{} (void)", idx);
        }
        else {
            return std::format("node_{} ({})", idx, type_name_short<R>());
        }
    }
};

// =========================================================================
// TaskGraph::execute() — 图执行入口
// =========================================================================

template <Scheduler S>
auto TaskGraph::execute(S& scheduler) -> Task<void>
{
    if (nodes_.empty())
        co_return;

    if (has_cycle())
        throw std::logic_error("TaskGraph contains a cycle");

    auto ctx = std::make_shared<detail::GraphExecutionContext<S>>(nodes_, scheduler);

    co_await detail::GraphAwaitable<S>{ctx};

    if (ctx->first_exception)
        std::rethrow_exception(ctx->first_exception);
}

} // namespace bee
