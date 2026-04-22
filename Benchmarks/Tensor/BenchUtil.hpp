/**
 * @File BenchUtil.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Tensor 基准共享工具：形状常量 / 张量构造 / dtype 迭代宏。
 *
 * 所有基准复用本文件定义的形状矩阵（tiny/small/medium/large）和便捷
 * 构造函数，避免样板重复；同时约定了以下 benchmark 标签字段：
 *   Counter("bytes/s", ...) ↔ 内存带宽视角
 *   Counter("elems/s", ...) ↔ 吞吐视角
 *   Counter("flops", ...)   ↔ 算力视角（matmul 专用）
 */

#pragma once

#include <benchmark/benchmark.h>

#include <cstdint>
#include <cstdlib>

#include "Tensor/Tensor.hpp"

namespace bee::bench
{

// ── 形状常量（见 docs/perf/OptimizationLog.md "形状矩阵"） ──────────────
inline constexpr int64_t kShapeTiny   = 256;      // < L1
inline constexpr int64_t kShapeSmall  = 4 * 1024; // < L2
inline constexpr int64_t kShapeMedium = 256 * 1024;  // ~L3 / bandwidth
inline constexpr int64_t kShapeLarge  = 16 * 1024 * 1024; // main memory

// 强制中止：基准路径里任何 Result 失败都是基础设施 bug
template <class T>
inline T bench_must(Result<T>&& r)
{
    if (!r) {
        const auto sv = r.error().message.view();
        std::fprintf(stderr, "bench_must failed: %.*s\n",
                     static_cast<int>(sv.size()), sv.data());
        std::abort();
    }
    return std::move(r.value());
}

// 指定 dtype + numel 创建一维填满 value 的 Tensor
inline Tensor make_filled_1d(int64_t numel, DType dt, double value,
                             Device dev = Device::CPU)
{
    return bench_must(Tensor::full(Shape{numel}, dt, value, dev));
}

// 指定二维 shape 填满 value
inline Tensor make_filled_2d(int64_t r, int64_t c, DType dt, double value,
                             Device dev = Device::CPU)
{
    return bench_must(Tensor::full(Shape{r, c}, dt, value, dev));
}

// 基准 "Args" 工具：把形状常量作为 int64 range 参数注入（单参数）
inline void set_shape_args_1d(benchmark::internal::Benchmark* b)
{
    b->Arg(kShapeTiny)->Arg(kShapeSmall)->Arg(kShapeMedium)->Arg(kShapeLarge)
     ->Unit(benchmark::kMicrosecond);
}

} // namespace bee::bench
