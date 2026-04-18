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

} // namespace bee
