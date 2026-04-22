/**
 * @File CastBench.cpp
 * @Brief Cast（DType 转换）基线：覆盖 F32↔F64/I32/I64/U8/Bool 的几组典型组合。
 */

#include "BenchUtil.hpp"

#include "Tensor/Ops/Cast.hpp"

using bee::Tensor;
using bee::DType;
using bee::Shape;
using bee::bench::bench_must;
using bee::bench::kShapeLarge;
using bee::bench::kShapeMedium;
using bee::bench::kShapeSmall;
using bee::bench::kShapeTiny;

namespace {

template <DType SrcDT, DType DstDT>
void BM_Cast(benchmark::State& state)
{
    const int64_t n = state.range(0);
    auto src = bench_must(Tensor::full(Shape{n}, SrcDT, 1.0));
    for (auto _ : state) {
        auto dst = bench_must(bee::cast(src, DstDT));
        benchmark::DoNotOptimize(dst.impl().get());
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * n *
                            (static_cast<int64_t>(dtype_size(SrcDT)) +
                             static_cast<int64_t>(dtype_size(DstDT))));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * n);
}

} // namespace

#define BEE_BENCH_CAST(name, src, dst)                                          \
    BENCHMARK(BM_Cast<bee::DType::src, bee::DType::dst>)                        \
        ->Name("BM_Cast_" #name)                                                \
        ->Arg(kShapeTiny)->Arg(kShapeSmall)->Arg(kShapeMedium)->Arg(kShapeLarge)\
        ->Unit(benchmark::kMicrosecond)

BEE_BENCH_CAST(F32_to_F64, F32, F64);
BEE_BENCH_CAST(F64_to_F32, F64, F32);
BEE_BENCH_CAST(F32_to_I32, F32, I32);
BEE_BENCH_CAST(I32_to_F32, I32, F32);
BEE_BENCH_CAST(F32_to_I64, F32, I64);
BEE_BENCH_CAST(I64_to_F32, I64, F32);
BEE_BENCH_CAST(F32_to_U8,  F32, U8);
BEE_BENCH_CAST(U8_to_F32,  U8,  F32);
BEE_BENCH_CAST(F32_to_Bool, F32, Bool);
