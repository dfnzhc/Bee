/**
 * @File RandomBench.cpp
 * @Brief rand / randn / randint 基线。
 */

#include "BenchUtil.hpp"

#include "Tensor/Ops/Random.hpp"

using bee::Tensor;
using bee::DType;
using bee::Shape;
using bee::bench::bench_must;
using bee::bench::kShapeLarge;
using bee::bench::kShapeMedium;
using bee::bench::kShapeSmall;
using bee::bench::kShapeTiny;

namespace {

void BM_RandF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    uint64_t seed = 1;
    for (auto _ : state) {
        auto t = bench_must(bee::rand(Shape{n}, DType::F32, seed++));
        benchmark::DoNotOptimize(t.impl().get());
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * n * 4);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n);
}

void BM_RandnF32(benchmark::State& state)
{
    const int64_t n = state.range(0);
    uint64_t seed = 1;
    for (auto _ : state) {
        auto t = bench_must(bee::randn(Shape{n}, DType::F32, seed++));
        benchmark::DoNotOptimize(t.impl().get());
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * n * 4);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n);
}

void BM_RandintI64(benchmark::State& state)
{
    const int64_t n = state.range(0);
    uint64_t seed = 1;
    for (auto _ : state) {
        auto t = bench_must(bee::randint(0, 1000, Shape{n}, DType::I64, seed++));
        benchmark::DoNotOptimize(t.impl().get());
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * n * 8);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n);
}

} // namespace

#define BEE_BENCH_ARGS_1D ->Arg(kShapeTiny)->Arg(kShapeSmall)->Arg(kShapeMedium)->Arg(kShapeLarge)->Unit(benchmark::kMicrosecond)

BENCHMARK(BM_RandF32)BEE_BENCH_ARGS_1D;
BENCHMARK(BM_RandnF32)BEE_BENCH_ARGS_1D;
BENCHMARK(BM_RandintI64)BEE_BENCH_ARGS_1D;
