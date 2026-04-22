/**
 * @File MatmulCudaBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief CUDA GEMM 基准：方阵 F32 × {256, 512, 1024, 2048}。
 *
 * 当前实现：16×16 tile shared-memory kernel（plan-cuda M5）。
 * 里程碑 B8/B10 将切换到 CUTLASS / TMA + tcgen05.mma 并注册新的后端变体。
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>

#include "Tensor/Tensor.hpp"
#include "Tensor/Ops/Matmul.hpp"

namespace
{

using namespace bee;

template <class T>
static T must(Result<T>&& r)
{
    if (!r) {
        const auto sv = r.error().message.view();
        std::fprintf(stderr, "CUDA matmul bench must failed: %.*s\n",
                     static_cast<int>(sv.size()), sv.data());
        std::abort();
    }
    return std::move(r.value());
}

static void BM_MatmulF32_Square_CUDA(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = must(Tensor::full(Shape{n, n}, DType::F32, 1.0, Device::CUDA));
    auto b = must(Tensor::full(Shape{n, n}, DType::F32, 2.0, Device::CUDA));
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
BENCHMARK(BM_MatmulF32_Square_CUDA)
    ->Arg(256)->Arg(512)->Arg(1024)->Arg(2048)
    ->Unit(benchmark::kMillisecond);

} // namespace
