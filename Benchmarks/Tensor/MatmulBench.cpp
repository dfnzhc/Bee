/**
 * @File MatmulBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief GEMM CPU 基准：方阵 F32，尺寸覆盖 {128, 256, 512, 1024}。
 *
 * 当前实现：朴素 ikj 三重循环，B4 会改为 tiled + AVX2 microkernel + 多线程。
 * 基准额外注册 "flops" Counter 以 2·M·N·K 计，便于换算 TFLOPS。
 */

#include "BenchUtil.hpp"

#include "Tensor/Ops/Matmul.hpp"

namespace
{

using namespace bee;
using namespace bee::bench;

static void BM_MatmulF32_Square(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_2d(n, n, DType::F32, 1.0);
    auto b = make_filled_2d(n, n, DType::F32, 2.0);
    for (auto _ : state) {
        auto c = bee::matmul(a, b);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    const double flops = 2.0 * static_cast<double>(n) * n * n;
    state.counters["flops/iter"] = flops;
    state.counters["gflops"] = benchmark::Counter(
        flops, benchmark::Counter::kIsIterationInvariantRate,
        benchmark::Counter::OneK::kIs1000);
    state.SetItemsProcessed(state.iterations() * n * n);
}
BENCHMARK(BM_MatmulF32_Square)
    ->Arg(128)->Arg(256)->Arg(512)->Arg(1024)
    ->Unit(benchmark::kMillisecond);

static void BM_MatmulF64_Square(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_2d(n, n, DType::F64, 1.0);
    auto b = make_filled_2d(n, n, DType::F64, 2.0);
    for (auto _ : state) {
        auto c = bee::matmul(a, b);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    const double flops = 2.0 * static_cast<double>(n) * n * n;
    state.counters["flops/iter"] = flops;
    state.counters["gflops"] = benchmark::Counter(
        flops, benchmark::Counter::kIsIterationInvariantRate,
        benchmark::Counter::OneK::kIs1000);
    state.SetItemsProcessed(state.iterations() * n * n);
}
BENCHMARK(BM_MatmulF64_Square)
    ->Arg(128)->Arg(256)->Arg(512)->Arg(1024)
    ->Unit(benchmark::kMillisecond);

static void BM_MatmulI32_Square(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_filled_2d(n, n, DType::I32, 1.0);
    auto b = make_filled_2d(n, n, DType::I32, 2.0);
    for (auto _ : state) {
        auto c = bee::matmul(a, b);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    const double flops = 2.0 * static_cast<double>(n) * n * n;
    state.counters["flops/iter"] = flops;
    state.counters["gflops"] = benchmark::Counter(
        flops, benchmark::Counter::kIsIterationInvariantRate,
        benchmark::Counter::OneK::kIs1000);
    state.SetItemsProcessed(state.iterations() * n * n);
}
BENCHMARK(BM_MatmulI32_Square)
    ->Arg(128)->Arg(256)->Arg(512)->Arg(1024)
    ->Unit(benchmark::kMillisecond);

} // namespace
