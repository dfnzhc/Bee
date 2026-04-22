/**
 * @File ReduceBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Reduce 算子 CPU 基准：global sum / mean / max（F32）。
 *
 * 当前 CPU Reduce 串行 + SIMD 水平归约尚未实装（B3），本文件提供基线。
 */

#include "BenchUtil.hpp"

#include "Tensor/Ops/Reduce.hpp"

namespace
{

using namespace bee;
using namespace bee::bench;

static void BM_SumF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_1d(n, DType::F32, 1.0);
    for (auto _ : state) {
        auto r = bee::sum(a);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_SumF32)->Apply(set_shape_args_1d);

static void BM_MeanF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_1d(n, DType::F32, 2.0);
    for (auto _ : state) {
        auto r = bee::mean(a);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_MeanF32)->Apply(set_shape_args_1d);

static void BM_MaxF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_1d(n, DType::F32, 3.0);
    for (auto _ : state) {
        auto r = bee::max(a);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_MaxF32)->Apply(set_shape_args_1d);

} // namespace
