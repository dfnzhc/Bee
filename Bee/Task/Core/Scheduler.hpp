/**
 * @File Scheduler.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#pragma once

#include <concepts>
#include <coroutine>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "Base/Core/MoveOnlyFunction.hpp"
#include "Task/Core/Task.hpp"

namespace bee
{

// =========================================================================
// Scheduler Concept — 执行上下文的最小抽象
// =========================================================================

/// 任何满足此 concept 的类型都可以被 schedule()、spawn_task()、
/// 以及并行算法使用。
///
/// 要求：
///   - s.schedule()       返回一个 co_await 后转移执行的 awaiter
///   - s.post(fn)         投递 fire-and-forget 工作
///   - s.thread_count()   工作线程数量
template <typename S>
concept Scheduler = requires(S& s) {
    { s.schedule() };
    { s.post(std::declval<MoveOnlyFunction<void()>>()) };
    { s.thread_count() } -> std::convertible_to<std::size_t>;
};

// =========================================================================
// spawn_task() — 在 Scheduler 上启动工作
// =========================================================================

/// 将可调用对象包装为 Task<T>，在 scheduler 线程上执行。
/// 替代 V1 的 submit(pool, fn)。
///
/// 用法：
///   auto t = spawn_task(pool, [] { return 42; });
///   int r = t.get();
/// 注意：fn 按值传递，避免 MSVC 协程参数生命周期 bug —
/// 右值引用参数在协程帧中可能存储为悬垂引用。
template <Scheduler S, typename Fn>
[[nodiscard]] auto spawn_task(S& scheduler, Fn fn) -> Task<std::invoke_result_t<Fn>>
{
    co_await scheduler.schedule();
    if constexpr (std::is_void_v<std::invoke_result_t<Fn>>) {
        fn();
    } else {
        co_return fn();
    }
}

} // namespace bee
