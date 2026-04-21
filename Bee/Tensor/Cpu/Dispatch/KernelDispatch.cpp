// 通用 CPU 算子分派实现：此 TU 会被 4 个 OBJECT library 各编译一次
// 通过 BEE_DISPATCH_ISA_<X> 宏选择命名空间与 ISA 标签

#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cpu/ElementWiseCpu.hpp"
#include "Tensor/Cpu/ReduceCpu.hpp"
#include "Tensor/Cpu/MatmulCpu.hpp"

#include "SIMD/SIMD.hpp"

// ─── 选择当前 TU 的 ISA 标签与命名空间 ────────────────────────────────────────
#if defined(BEE_DISPATCH_ISA_AVX512)
#  define BEE_CURRENT_ISA ::bee::simd::IsaAvx512
#  define BEE_CURRENT_NS  avx512
#elif defined(BEE_DISPATCH_ISA_AVX2)
#  define BEE_CURRENT_ISA ::bee::simd::IsaAvx2
#  define BEE_CURRENT_NS  avx2
#elif defined(BEE_DISPATCH_ISA_SSE2)
#  define BEE_CURRENT_ISA ::bee::simd::IsaSse2
#  define BEE_CURRENT_NS  sse2
#elif defined(BEE_DISPATCH_ISA_SCALAR)
#  define BEE_CURRENT_ISA ::bee::simd::IsaScalar
#  define BEE_CURRENT_NS  scalar
#else
#  error "KernelDispatch.cpp requires one of BEE_DISPATCH_ISA_{SCALAR,SSE2,AVX2,AVX512}"
#endif

namespace bee::cpu
{
namespace BEE_CURRENT_NS
{

using _ISA = BEE_CURRENT_ISA;

// ─── 辅助宏：按 dtype switch 分派到模板化内核 ────────────────────────────────

#define BEE_EW_BIN_DTYPE_DISPATCH(OP, A, B, OUT)                                \
    switch ((OUT).dtype()) {                                                    \
        case ::bee::DType::F32:  cpu_elementwise_binary<float,    _ISA, OP>((A),(B),(OUT)); return; \
        case ::bee::DType::F64:  cpu_elementwise_binary<double,   _ISA, OP>((A),(B),(OUT)); return; \
        case ::bee::DType::I32:  cpu_elementwise_binary<int32_t,  _ISA, OP>((A),(B),(OUT)); return; \
        case ::bee::DType::I64:  cpu_elementwise_binary<int64_t,  _ISA, OP>((A),(B),(OUT)); return; \
        case ::bee::DType::U8:   cpu_elementwise_binary<uint8_t,  _ISA, OP>((A),(B),(OUT)); return; \
        default: return;                                                        \
    }

#define BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OP, A, OUT)                              \
    switch ((OUT).dtype()) {                                                    \
        case ::bee::DType::F32: cpu_elementwise_unary<float,  _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::F64: cpu_elementwise_unary<double, _ISA, OP>((A),(OUT)); return; \
        default: return;                                                        \
    }

#define BEE_EW_UN_ANY_DTYPE_DISPATCH(OP, A, OUT)                                \
    switch ((OUT).dtype()) {                                                    \
        case ::bee::DType::F32: cpu_elementwise_unary<float,   _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::F64: cpu_elementwise_unary<double,  _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::I32: cpu_elementwise_unary<int32_t, _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::I64: cpu_elementwise_unary<int64_t, _ISA, OP>((A),(OUT)); return; \
        default: return;                                                        \
    }

// ─── 二元 elementwise ────────────────────────────────────────────────────────
auto ew_add(const Tensor& a, const Tensor& b, Tensor& out) -> void { BEE_EW_BIN_DTYPE_DISPATCH(OpAdd, a, b, out); }
auto ew_sub(const Tensor& a, const Tensor& b, Tensor& out) -> void { BEE_EW_BIN_DTYPE_DISPATCH(OpSub, a, b, out); }
auto ew_mul(const Tensor& a, const Tensor& b, Tensor& out) -> void { BEE_EW_BIN_DTYPE_DISPATCH(OpMul, a, b, out); }
auto ew_div(const Tensor& a, const Tensor& b, Tensor& out) -> void { BEE_EW_BIN_DTYPE_DISPATCH(OpDiv, a, b, out); }

// ─── 一元 elementwise ────────────────────────────────────────────────────────
auto ew_neg(const Tensor& a, Tensor& out) -> void
{
    switch (out.dtype()) {
        case ::bee::DType::F32: cpu_elementwise_unary<float,   _ISA, OpNeg>(a, out); return;
        case ::bee::DType::F64: cpu_elementwise_unary<double,  _ISA, OpNeg>(a, out); return;
        case ::bee::DType::I32: cpu_elementwise_unary<int32_t, _ISA, OpNeg>(a, out); return;
        case ::bee::DType::I64: cpu_elementwise_unary<int64_t, _ISA, OpNeg>(a, out); return;
        default: return;
    }
}

auto ew_abs(const Tensor& a, Tensor& out) -> void
{
    switch (out.dtype()) {
        case ::bee::DType::F32: cpu_elementwise_unary<float,   _ISA, OpAbs>(a, out); return;
        case ::bee::DType::F64: cpu_elementwise_unary<double,  _ISA, OpAbs>(a, out); return;
        case ::bee::DType::I32: cpu_elementwise_unary<int32_t, _ISA, OpAbs>(a, out); return;
        case ::bee::DType::I64: cpu_elementwise_unary<int64_t, _ISA, OpAbs>(a, out); return;
        default: return;
    }
}

auto ew_sqrt(const Tensor& a, Tensor& out) -> void { BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OpSqrt, a, out); }
auto ew_exp (const Tensor& a, Tensor& out) -> void { BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OpExp,  a, out); }
auto ew_log (const Tensor& a, Tensor& out) -> void { BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OpLog,  a, out); }

// ─── 全局 reduce ─────────────────────────────────────────────────────────────
#define BEE_RD_GLOBAL_DTYPE_DISPATCH(OP, A, OUT)                                \
    switch ((A).dtype()) {                                                      \
        case ::bee::DType::F32: cpu_reduce_global_dispatch<float,   _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::F64: cpu_reduce_global_dispatch<double,  _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::I32: cpu_reduce_global_dispatch<int32_t, _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::I64: cpu_reduce_global_dispatch<int64_t, _ISA, OP>((A),(OUT)); return; \
        case ::bee::DType::U8:  cpu_reduce_global_dispatch<uint8_t, _ISA, OP>((A),(OUT)); return; \
        default: return;                                                        \
    }

auto rd_sum_global (const Tensor& a, Tensor& out) -> void { BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceSum,  a, out); }
auto rd_min_global (const Tensor& a, Tensor& out) -> void { BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceMin,  a, out); }
auto rd_max_global (const Tensor& a, Tensor& out) -> void { BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceMax,  a, out); }
auto rd_prod_global(const Tensor& a, Tensor& out) -> void { BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceProd, a, out); }

// ─── matmul ──────────────────────────────────────────────────────────────────
auto mm_f32(int64_t M, int64_t K, int64_t N, const float*   A, const float*   B, float*   C) -> void { cpu_matmul_kernel<float,   _ISA>(M,K,N,A,B,C); }
auto mm_f64(int64_t M, int64_t K, int64_t N, const double*  A, const double*  B, double*  C) -> void { cpu_matmul_kernel<double,  _ISA>(M,K,N,A,B,C); }
auto mm_i32(int64_t M, int64_t K, int64_t N, const int32_t* A, const int32_t* B, int32_t* C) -> void { cpu_matmul_kernel<int32_t, _ISA>(M,K,N,A,B,C); }
auto mm_i64(int64_t M, int64_t K, int64_t N, const int64_t* A, const int64_t* B, int64_t* C) -> void { cpu_matmul_kernel<int64_t, _ISA>(M,K,N,A,B,C); }

} // namespace BEE_CURRENT_NS
} // namespace bee::cpu
