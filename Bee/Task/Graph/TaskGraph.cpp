/**
 * @File TaskGraph.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#include "Task/Graph/TaskGraph.hpp"
#include "Concurrency/Thread/ThreadPool.hpp"

#include <queue>
#include <stdexcept>

namespace bee
{

// =========================================================================
// 执行上下文 — 在单次图执行的所有线程池任务间共享
// =========================================================================

namespace
{

    struct ExecutionContext
    {
        ThreadPool* pool;
        std::stop_token token;
        std::shared_ptr<detail::SharedState<void>> completion;
        std::atomic<u32> remaining{0};

        std::exception_ptr first_exception;
        std::mutex exception_mutex;
        std::atomic<bool> any_cancelled{false};

        std::vector<std::unique_ptr<detail::NodeEntry>>* nodes;
        std::atomic<bool>* executing_flag;
    };

    // 前向声明 — resolve 与 on_terminal 之间存在相互递归。
    auto resolve_node(std::shared_ptr<ExecutionContext> ctx, detail::NodeId id) -> void;

    auto complete_graph(std::shared_ptr<ExecutionContext> ctx) -> void
    {
        ctx->executing_flag->store(false, std::memory_order_release);

        if (ctx->first_exception) {
            ctx->completion->fail(ctx->first_exception);
        } else if (ctx->any_cancelled.load(std::memory_order_acquire)) {
            ctx->completion->cancel();
        } else {
            ctx->completion->complete();
        }
    }

    auto on_node_terminal(std::shared_ptr<ExecutionContext> ctx, detail::NodeId id) -> void
    {
        auto& node = (*ctx->nodes)[id.value];
        auto state = node->state.load(std::memory_order_acquire);

        if (state == NodeState::Failed && node->exception) {
            std::lock_guard lock(ctx->exception_mutex);
            if (!ctx->first_exception) {
                ctx->first_exception = node->exception;
            }
        }

        if (state == NodeState::Cancelled) {
            ctx->any_cancelled.store(true, std::memory_order_release);
        }

        // 通知后继节点
        for (auto& succ_id : node->successors) {
            auto& succ = (*ctx->nodes)[succ_id.value];
            if (succ->pending_deps.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                succ->state.store(NodeState::Ready, std::memory_order_release);
                resolve_node(ctx, succ_id);
            }
        }

        // 检查图执行是否全部完成
        if (ctx->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            complete_graph(ctx);
        }
    }

    auto submit_node(std::shared_ptr<ExecutionContext> ctx, detail::NodeId id) -> void
    {
        auto& node = (*ctx->nodes)[id.value];
        node->state.store(NodeState::Running, std::memory_order_release);

        ctx->pool->post([ctx, id]() {
            auto& node = (*ctx->nodes)[id.value];

            try {
                std::vector<std::any> inputs;
                inputs.reserve(node->predecessors.size());
                for (auto& pred_id : node->predecessors) {
                    inputs.push_back((*ctx->nodes)[pred_id.value]->result);
                }

                node->result = node->callable(inputs);
                node->state.store(NodeState::Completed, std::memory_order_release);
            } catch (...) {
                node->exception = std::current_exception();
                node->state.store(NodeState::Failed, std::memory_order_release);
            }

            on_node_terminal(ctx, id);
        });
    }

    auto resolve_node(std::shared_ptr<ExecutionContext> ctx, detail::NodeId id) -> void
    {
        auto& node = (*ctx->nodes)[id.value];

        // 检查取消请求
        if (ctx->token.stop_requested()) {
            node->state.store(NodeState::Cancelled, std::memory_order_release);
            on_node_terminal(ctx, id);
            return;
        }

        // 检查前驱节点是否失败或被取消
        for (auto& pred_id : node->predecessors) {
            auto pred_state = (*ctx->nodes)[pred_id.value]->state.load(std::memory_order_acquire);
            if (pred_state == NodeState::Failed) {
                node->exception = (*ctx->nodes)[pred_id.value]->exception;
                node->state.store(NodeState::Failed, std::memory_order_release);
                on_node_terminal(ctx, id);
                return;
            }
            if (pred_state == NodeState::Cancelled) {
                node->state.store(NodeState::Cancelled, std::memory_order_release);
                on_node_terminal(ctx, id);
                return;
            }
        }

        // 所有前驱已完成 — 提交至线程池
        submit_node(ctx, id);
    }

} // anonymous namespace

// =========================================================================
// TaskGraph — 构建方法
// =========================================================================

auto TaskGraph::create_node(MoveOnlyFunction<std::any(const std::vector<std::any>&)> callable, std::vector<detail::NodeId> deps) -> detail::NodeId
{
    BEE_CHECK(!executing_.load(std::memory_order_acquire));

    auto id             = detail::NodeId{static_cast<u32>(nodes_.size())};
    auto entry          = std::make_unique<detail::NodeEntry>();
    entry->callable     = std::move(callable);
    entry->predecessors = std::move(deps);

    for (auto& pred_id : entry->predecessors) {
        BEE_CHECK(pred_id.value < nodes_.size());
        nodes_[pred_id.value]->successors.push_back(id);
    }

    nodes_.push_back(std::move(entry));
    return id;
}

auto TaskGraph::node_count() const noexcept -> size_t
{
    return nodes_.size();
}

auto TaskGraph::has_cycle() const -> bool
{
    auto n = nodes_.size();
    if (n == 0) {
        return false;
    }

    std::vector<u32> in_degree(n, 0);
    for (size_t i = 0; i < n; ++i) {
        in_degree[i] = static_cast<u32>(nodes_[i]->predecessors.size());
    }

    std::queue<u32> ready;
    for (u32 i = 0; i < static_cast<u32>(n); ++i) {
        if (in_degree[i] == 0) {
            ready.push(i);
        }
    }

    u32 processed = 0;
    while (!ready.empty()) {
        auto current = ready.front();
        ready.pop();
        ++processed;

        for (auto& succ_id : nodes_[current]->successors) {
            if (--in_degree[succ_id.value] == 0) {
                ready.push(succ_id.value);
            }
        }
    }

    return processed != static_cast<u32>(n);
}

auto TaskGraph::empty() const noexcept -> bool
{
    return nodes_.empty();
}

// =========================================================================
// TaskGraph — 执行
// =========================================================================

auto TaskGraph::execute(ThreadPool& pool) -> Task<void>
{
    return execute_impl(pool, std::stop_token{});
}

auto TaskGraph::execute(ThreadPool& pool, std::stop_token token) -> Task<void>
{
    return execute_impl(pool, std::move(token));
}

auto TaskGraph::execute_impl(ThreadPool& pool, std::stop_token token) -> Task<void>
{
    auto completion = std::make_shared<detail::SharedState<void>>();

    BEE_CHECK(!executing_.load(std::memory_order_acquire));

    // 空图立即完成
    if (nodes_.empty()) {
        completion->complete();
        return Task<void>(completion);
    }

    // 环检测
    if (has_cycle()) {
        completion->fail(std::make_exception_ptr(std::runtime_error("TaskGraph contains a cycle")));
        return Task<void>(completion);
    }

    executing_.store(true, std::memory_order_release);

    // 重置所有节点以支持（重新）执行
    for (auto& node : nodes_) {
        node->state.store(NodeState::Pending, std::memory_order_relaxed);
        node->pending_deps.store(static_cast<u32>(node->predecessors.size()), std::memory_order_relaxed);
        node->result    = std::any{};
        node->exception = nullptr;
    }

    // 创建在所有线程池任务间共享的执行上下文
    auto ctx        = std::make_shared<ExecutionContext>();
    ctx->pool       = &pool;
    ctx->token      = std::move(token);
    ctx->completion = completion;
    ctx->remaining.store(static_cast<u32>(nodes_.size()), std::memory_order_relaxed);
    ctx->nodes          = &nodes_;
    ctx->executing_flag = &executing_;

    // 提交根节点（入度为 0）
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i]->predecessors.empty()) {
            nodes_[i]->state.store(NodeState::Ready, std::memory_order_release);
            resolve_node(ctx, detail::NodeId{static_cast<u32>(i)});
        }
    }

    return Task<void>(completion);
}

} // namespace bee
