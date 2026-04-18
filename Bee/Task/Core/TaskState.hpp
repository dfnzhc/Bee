/**
 * @File TaskState.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/11
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Base/Core/Config.hpp"
#include "Base/Reflection/Enum.hpp"

namespace bee
{

/**
 * @brief Task 的生命周期状态。
 *
 * 状态转换:
 *   Pending  → Running   （可调用对象开始执行）
 *   Running  → Completed （可调用对象成功返回）
 *   Running  → Failed    （可调用对象抛出异常）
 *
 * 终态（Completed、Failed）不可逆。
 */
enum class TaskState : u8
{
    Pending,
    Running,
    Completed,
    Failed
};

BEE_ENUM_SCAN_COUNT(TaskState, 4);

} // namespace bee
