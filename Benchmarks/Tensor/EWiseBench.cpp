/**
 * @File EWiseBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 逐元素算子 CPU 基准：add / mul / sqrt（F32）。
 *
 * 目的：在 B0 阶段为后续 B2（CPU ElementWise 多线程 + non-temporal store）
 * 提供可对比的基线数字。形状覆盖 tiny/small/medium/large（见 BenchUtil.hpp）。
 */

#include "BenchUtil.hpp"

#include "Tensor/Ops/ElementWise.hpp"

namespace
{

using namespace bee;
using namespace bee::bench;

static void BM_AddF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_1d(n, DType::F32, 1.0);
    auto b = make_filled_1d(n, DType::F32, 2.0);
    for (auto _ : state) {
        auto c = bee::add(a, b);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * 3 * sizeof(float));
}
BENCHMARK(BM_AddF32)->Apply(set_shape_args_1d);

static void BM_MulF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_1d(n, DType::F32, 1.5);
    auto b = make_filled_1d(n, DType::F32, 2.5);
    for (auto _ : state) {
        auto c = bee::mul(a, b);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * 3 * sizeof(float));
}
BENCHMARK(BM_MulF32)->Apply(set_shape_args_1d);

static void BM_SqrtF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_1d(n, DType::F32, 4.0);
    for (auto _ : state) {
        auto c = bee::sqrt(a);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * 2 * sizeof(float));
}
BENCHMARK(BM_SqrtF32)->Apply(set_shape_args_1d);

} // namespace
