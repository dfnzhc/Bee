/**
 * @File Task.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/18
 * @Brief This file is part of Bee.
 */

#pragma once

// ── Core ──
#include "Task/Core/TaskState.hpp"
#include "Task/Core/Task.hpp"
#include "Task/Core/Scheduler.hpp"
#include "Task/Core/WhenAll.hpp"
#include "Task/Core/WhenAny.hpp"
#include "Task/Core/AsyncScope.hpp"

// ── Parallel ──
#include "Task/Parallel/Partitioner.hpp"
#include "Task/Parallel/ForEach.hpp"
#include "Task/Parallel/Transform.hpp"
#include "Task/Parallel/Reduce.hpp"
#include "Task/Parallel/Scan.hpp"
#include "Task/Parallel/Sort.hpp"
