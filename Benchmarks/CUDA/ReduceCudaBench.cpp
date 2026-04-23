/**
 * @File ReduceCudaBench.cpp
 * @Brief CUDA Reduce 基准：sum/min/max × F32/I32。B6 warp-shuffle 对照。
 *        包含 kernel + cudaStreamSynchronize 的 wall-clock。
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>

#include "Tensor/Tensor.hpp"
#include "Tensor/Ops/Reduce.hpp"

namespace
{

using namespace bee;

static constexpr int64_t kSmall  = 4 * 1024;
static constexpr int64_t kMedium = 1 * 1024 * 1024;
static constexpr int64_t kLarge  = 16 * 1024 * 1024;

template <class T>
static T must(Result<T>&& r)
{
    if (!r) {
        const auto sv = r.error().message.view();
        std::fprintf(stderr, "CUDA reduce bench must failed: %.*s\n",
                     static_cast<int>(sv.size()), sv.data());
        std::abort();
    }
    return std::move(r.value());
}

static void BM_SumF32_CUDA(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = must(Tensor::full(Shape{n}, DType::F32, 1.0, Device::CUDA));
    for (auto _ : state) {
        auto r = bee::sum(a);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_SumF32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)->Unit(benchmark::kMicrosecond);

static void BM_SumI32_CUDA(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = must(Tensor::full(Shape{n}, DType::I32, 1.0, Device::CUDA));
    for (auto _ : state) {
        auto r = bee::sum(a);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(std::int32_t));
}
BENCHMARK(BM_SumI32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)->Unit(benchmark::kMicrosecond);

static void BM_MaxF32_CUDA(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = must(Tensor::full(Shape{n}, DType::F32, 3.0, Device::CUDA));
    for (auto _ : state) {
        auto r = bee::max(a);
        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * sizeof(float));
}
BENCHMARK(BM_MaxF32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)->Unit(benchmark::kMicrosecond);

} // namespace
