/**
 * @File TransposeCudaBench.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief CUDA 2D transpose 基准：方阵/瘦阵 F32/I32/F64 物化拷贝吞吐。
 *
 * 通过 tensor.transpose(0,1).contiguous() 触发设备侧 transpose kernel；
 * 计时仅覆盖 stream 同步后的 kernel 执行时间。
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>

#include "Tensor/Tensor.hpp"

namespace
{

using namespace bee;

template <class T>
static T must(Result<T>&& r)
{
    if (!r) {
        const auto sv = r.error().message.view();
        std::fprintf(stderr, "CUDA transpose bench must failed: %.*s\n",
                     static_cast<int>(sv.size()), sv.data());
        std::abort();
    }
    return std::move(r.value());
}

template <DType DT>
static void BM_Transpose_Contig_CUDA_Square(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto a = must(Tensor::full(Shape{n, n}, DT, 1.0, Device::CUDA));

    for (auto _ : state) {
        auto view = must(a.transpose(0, 1));
        auto out  = must(view.contiguous());
        benchmark::DoNotOptimize(out);
        benchmark::ClobberMemory();
    }

    const int64_t elem_sz = dtype_size(DT);
    const double  bytes   = 2.0 * static_cast<double>(n) * n * elem_sz; // read + write
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(bytes));
    state.counters["GB/s"] = benchmark::Counter(
        bytes, benchmark::Counter::kIsIterationInvariantRate,
        benchmark::Counter::OneK::kIs1024);
}

BENCHMARK_TEMPLATE(BM_Transpose_Contig_CUDA_Square, DType::F32)
    ->Arg(1024)->Arg(2048)->Arg(4096)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Transpose_Contig_CUDA_Square, DType::I32)
    ->Arg(1024)->Arg(2048)->Arg(4096)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(BM_Transpose_Contig_CUDA_Square, DType::F64)
    ->Arg(1024)->Arg(2048)->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

template <DType DT>
static void BM_Transpose_Contig_CUDA_Tall(benchmark::State& state)
{
    const int64_t rows = state.range(0);
    const int64_t cols = 64;
    auto a = must(Tensor::full(Shape{rows, cols}, DT, 1.0, Device::CUDA));

    for (auto _ : state) {
        auto view = must(a.transpose(0, 1));
        auto out  = must(view.contiguous());
        benchmark::DoNotOptimize(out);
        benchmark::ClobberMemory();
    }

    const int64_t elem_sz = dtype_size(DT);
    const double  bytes   = 2.0 * static_cast<double>(rows) * cols * elem_sz;
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(bytes));
    state.counters["GB/s"] = benchmark::Counter(
        bytes, benchmark::Counter::kIsIterationInvariantRate,
        benchmark::Counter::OneK::kIs1024);
}

BENCHMARK_TEMPLATE(BM_Transpose_Contig_CUDA_Tall, DType::F32)
    ->Arg(16384)->Arg(262144)
    ->Unit(benchmark::kMicrosecond);

} // namespace
