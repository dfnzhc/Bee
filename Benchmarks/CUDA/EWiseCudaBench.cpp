/**
 * @File EWiseCudaBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 逐元素算子 CUDA 基准：add / mul（F32），对照 CPU 同名基准。
 *
 * 计时包含 kernel launch + 执行；设备上张量已提前驻留，避免把 H2D 开销算入。
 * 同步策略：每次 iter 结束时隐式由 add() 返回前完成（Tensor::to(CPU) 会等待），
 * 但为了仅测 kernel 吞吐，这里通过 `Tensor::to(CUDA)` 初始化后在 state loop
 * 中保持设备内操作。真实严格计时在 B5 里会切到 cudaEvent。
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>

#include "Tensor/Tensor.hpp"
#include "Tensor/Ops/ElementWise.hpp"

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
        std::fprintf(stderr, "CUDA bench must failed: %.*s\n",
                     static_cast<int>(sv.size()), sv.data());
        std::abort();
    }
    return std::move(r.value());
}

static Tensor make_cuda_filled(int64_t n, double v)
{
    auto t = must(Tensor::full(Shape{n}, DType::F32, v, Device::CUDA));
    return t;
}

static void BM_AddF32_CUDA(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_cuda_filled(n, 1.0);
    auto b = make_cuda_filled(n, 2.0);
    for (auto _ : state) {
        auto c = bee::add(a, b);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * 3 * sizeof(float));
}
BENCHMARK(BM_AddF32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)
    ->Unit(benchmark::kMicrosecond);

static void BM_MulF32_CUDA(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = make_cuda_filled(n, 1.5);
    auto b = make_cuda_filled(n, 2.5);
    for (auto _ : state) {
        auto c = bee::mul(a, b);
        benchmark::DoNotOptimize(c);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * n * 3 * sizeof(float));
}
BENCHMARK(BM_MulF32_CUDA)->Arg(kSmall)->Arg(kMedium)->Arg(kLarge)
    ->Unit(benchmark::kMicrosecond);

} // namespace
