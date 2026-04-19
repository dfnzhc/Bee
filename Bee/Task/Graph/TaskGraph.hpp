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
        // 静态拓扑（构建期确定，执行期 const）
        std::vector<NodeEntry>& nodes;
        const std::vector<std::size_t>& root_indices;
        S& scheduler;
        // 运行态
        std::vector<std::atomic<std::size_t>> counters;
        std::atomic<std::size_t> remaining;
        std::coroutine_handle<> continuation{nullptr};
        std::exception_ptr first_exception{nullptr};
        std::mutex exception_mutex;

        GraphExecutionContext(std::vector<NodeEntry>& n,
                              const std::vector<std::size_t>& roots,
                              S& s)
            : nodes(n), root_indices(roots), scheduler(s), counters(n.size()), remaining(n.size() + 1)
        {
            // 采用 "+1 trick"：remaining 初始为 N+1，由 await_suspend 在
            // 全部根节点 post 完毕后投出最后一票。这样可避免某个工作线程
            // 在 await_suspend 还在迭代时就把 remaining 降到 0 并 resume
            // 父协程（违反 C++ 协程"未挂起即不得 resume"的约束，且会令
            // TaskGraph 提前析构、nodes 引用悬垂）。
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
        //
        // 异常安全：即使 scheduler.post() 抛出（队列满/关停），也必须确保
        // remaining 最终递减，否则协程永远不会恢复导致死锁。
        // 策略：post 失败时忽略后继（该后继永远不会执行，但不会阻塞图的完成）。
        for (auto succ : node.successors) {
            if (ctx->counters[succ].fetch_sub(1, std::memory_order_acq_rel) == 1) {
                try {
                    ctx->scheduler.post([ctx, succ]() {
                        run_node(ctx, succ);
                    });
                }
                catch (...) {
                    // post 失败：后继无法执行。
                    // 将其标记为异常并递减 remaining 以防止死锁。
                    {
                        std::lock_guard lock(ctx->exception_mutex);
                        if (!ctx->first_exception) {
                            ctx->first_exception =
                                std::make_exception_ptr(std::runtime_error("TaskGraph: failed to post successor node"));
                        }
                    }
                    // 递减后继节点未能执行的 remaining 计数
                    // 注意：后继的后继也不会执行，但 remaining 只需要所有已执行节点递减即可
                    if (ctx->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                        ctx->continuation.resume();
                        return;
                    }
                }
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
            auto local_ctx          = ctx;
            local_ctx->continuation = h;

            // 将所有根节点 post 到 scheduler。
            //
            // 规范（不变式 C）：根节点判据必须是构建期确定的静态拓扑，
            //   即 TaskGraph 维护的 root_indices_ 向量。严禁使用运行期
            //   counter 值（例如 counters[i]==0）作为"根节点"的判据 ——
            //   一旦某根节点被工作线程提前跑完，会把后继 counter 递减到
            //   0，循环后续迭代若以 counter==0 判根就会把该后继误当作
            //   根节点二次 post，进而破坏后继的后继的计数器同步，最终
            //   在真实前驱尚未写 result 时就启动下游。
            // 异常安全：post 失败时必须把对应的"未启动节点"从 remaining
            //   中扣掉，否则 remaining 永远到不了 0，父协程将永久挂起。
            std::size_t unposted = 0;
            for (auto i : local_ctx->root_indices) {
                try {
                    local_ctx->scheduler.post([local_ctx, i]() {
                        run_node(local_ctx, i);
                    });
                }
                catch (...) {
                    {
                        std::lock_guard lock(local_ctx->exception_mutex);
                        if (!local_ctx->first_exception) {
                            local_ctx->first_exception = std::current_exception();
                        }
                    }
                    ++unposted;
                }
            }

            // "+1 trick" 的最后一票：在所有根节点 post 完毕后才把
            // 父协程纳入计数；同时一并扣除 post 失败的节点（这些节点
            // 永远不会执行 run_node，因此不会自行递减 remaining）。
            const std::size_t parent_vote = unposted + 1;
            if (local_ctx->remaining.fetch_sub(parent_vote, std::memory_order_acq_rel) == parent_vote) {
                local_ctx->continuation.resume();
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
            os << "    " << i << " [label=\"";
            // 转义 DOT 特殊字符
            for (char c : nodes_[i].label) {
                if (c == '"')
                    os << "\\\"";
                else if (c == '\\')
                    os << "\\\\";
                else
                    os << c;
            }
            os << "\"];\n";
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
    // 根节点索引（无前驱）—— 构建期维护，执行期 const。
    // 规范（不变式 C）：所有"根节点"调度判据都必须读取此向量，严禁
    // 使用运行期 counters 派生。
    std::vector<std::size_t> root_indices_;
    bool executed_{false};

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

        // 若无前驱，登记为根节点（构建期即可确定，后续不再变化）。
        if (nodes_[idx].predecessors.empty()) {
            root_indices_.push_back(idx);
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
    if (executed_)
        throw std::logic_error("TaskGraph: execute() can only be called once");
    executed_ = true;

    if (nodes_.empty())
        co_return;

    if (has_cycle())
        throw std::logic_error("TaskGraph contains a cycle");

    auto ctx = std::make_shared<detail::GraphExecutionContext<S>>(nodes_, root_indices_, scheduler);

    co_await detail::GraphAwaitable<S>{ctx};

    if (ctx->first_exception)
        std::rethrow_exception(ctx->first_exception);
}

} // namespace bee
