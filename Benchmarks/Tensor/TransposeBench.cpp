/**
 * @File TransposeBench.cpp
 * @Brief transpose + contiguous 基线：covers 2D 方阵与高长宽比矩阵。
 *        transpose 自身零拷贝，我们关心 strided→contiguous 物化的带宽。
 */

#include "BenchUtil.hpp"

using bee::Tensor;
using bee::DType;
using bee::Shape;
using bee::bench::bench_must;

namespace {

void BM_Transpose_Contig_F32_Square(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto src = bench_must(Tensor::full(Shape{n, n}, DType::F32, 1.0));
    for (auto _ : state) {
        auto t = bench_must(src.transpose(0, 1));
        auto c = bench_must(t.contiguous());
        benchmark::DoNotOptimize(c.impl().get());
    }
    const int64_t bytes = n * n * 4;
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * bytes * 2);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n * n);
}

void BM_Transpose_Contig_F32_Tall(benchmark::State& state)
{
    // 高长宽比：N x 64 → 转置后 64 x N，考察行/列跨度差异
    const int64_t n = state.range(0);
    constexpr int64_t k = 64;
    auto src = bench_must(Tensor::full(Shape{n, k}, DType::F32, 1.0));
    for (auto _ : state) {
        auto t = bench_must(src.transpose(0, 1));
        auto c = bench_must(t.contiguous());
        benchmark::DoNotOptimize(c.impl().get());
    }
    const int64_t bytes = n * k * 4;
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * bytes * 2);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n * k);
}

} // namespace

BENCHMARK(BM_Transpose_Contig_F32_Square)
    ->Arg(128)->Arg(512)->Arg(1024)->Arg(2048)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Transpose_Contig_F32_Tall)
    ->Arg(1024)->Arg(16384)->Arg(262144)
    ->Unit(benchmark::kMicrosecond);
