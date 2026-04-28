// 通用 CPU 算子分派实现：此 TU 会被 4 个 OBJECT library 各编译一次
// 通过 BEE_DISPATCH_ISA_<X> 宏选择命名空间与 ISA 标签

#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cpu/ElementWiseCpu.hpp"
#include "Tensor/Cpu/ReduceCpu.hpp"
#include "Tensor/Cpu/MatmulCpu.hpp"
#include "Tensor/Cpu/CastCpu.hpp"
#include "Tensor/Cpu/TransposeCpu.hpp"
#include "Tensor/Cpu/Gemm/GemmDispatch.hpp"

#include "SIMD/SIMD.hpp"

#include <cstring>

// ─── 按当前 ISA include GEMM 实现 .inl（在对应 OBJECT 库沿用 ISA flags）────────
#if defined(BEE_DISPATCH_ISA_AVX2)
    #include "Tensor/Cpu/Gemm/GemmAvx2.inl"
#elif defined(BEE_DISPATCH_ISA_SSE2)
    #include "Tensor/Cpu/Gemm/GemmSse2.inl"
#elif defined(BEE_DISPATCH_ISA_SCALAR)
    #include "Tensor/Cpu/Gemm/GemmScalar.inl"
#endif
// AVX512 OBJECT 库不定义 gemm::avx512::*；AVX512 dispatch 下游复用 avx2 实现

// ─── 选择当前 TU 的 ISA 标签与命名空间 ────────────────────────────────────────
#if defined(BEE_DISPATCH_ISA_AVX512)
    #define BEE_CURRENT_ISA ::bee::simd::IsaAvx512
    #define BEE_CURRENT_NS  avx512
#elif defined(BEE_DISPATCH_ISA_AVX2)
    #define BEE_CURRENT_ISA ::bee::simd::IsaAvx2
    #define BEE_CURRENT_NS  avx2
#elif defined(BEE_DISPATCH_ISA_SSE2)
    #define BEE_CURRENT_ISA ::bee::simd::IsaSse2
    #define BEE_CURRENT_NS  sse2
#elif defined(BEE_DISPATCH_ISA_SCALAR)
    #define BEE_CURRENT_ISA ::bee::simd::IsaScalar
    #define BEE_CURRENT_NS  scalar
#else
    #error "KernelDispatch.cpp requires one of BEE_DISPATCH_ISA_{SCALAR,SSE2,AVX2,AVX512}"
#endif

namespace bee::cpu
{
namespace BEE_CURRENT_NS
{

    using _ISA = BEE_CURRENT_ISA;

    // ─── 辅助宏：按 dtype switch 分派到模板化内核 ────────────────────────────────

#define BEE_EW_BIN_DTYPE_DISPATCH(OP, A, B, OUT)                                                \
    switch ((OUT).dtype()) {                                                                    \
    case ::bee::DType::F32: cpu_elementwise_binary<float, _ISA, OP>((A), (B), (OUT)); return;   \
    case ::bee::DType::F64: cpu_elementwise_binary<double, _ISA, OP>((A), (B), (OUT)); return;  \
    case ::bee::DType::I32: cpu_elementwise_binary<int32_t, _ISA, OP>((A), (B), (OUT)); return; \
    case ::bee::DType::I64: cpu_elementwise_binary<int64_t, _ISA, OP>((A), (B), (OUT)); return; \
    case ::bee::DType::U8: cpu_elementwise_binary<uint8_t, _ISA, OP>((A), (B), (OUT)); return;  \
    default: return;                                                                            \
    }

#define BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OP, A, OUT)                                       \
    switch ((OUT).dtype()) {                                                             \
    case ::bee::DType::F32: cpu_elementwise_unary<float, _ISA, OP>((A), (OUT)); return;  \
    case ::bee::DType::F64: cpu_elementwise_unary<double, _ISA, OP>((A), (OUT)); return; \
    default: return;                                                                     \
    }

#define BEE_EW_UN_ANY_DTYPE_DISPATCH(OP, A, OUT)                                          \
    switch ((OUT).dtype()) {                                                              \
    case ::bee::DType::F32: cpu_elementwise_unary<float, _ISA, OP>((A), (OUT)); return;   \
    case ::bee::DType::F64: cpu_elementwise_unary<double, _ISA, OP>((A), (OUT)); return;  \
    case ::bee::DType::I32: cpu_elementwise_unary<int32_t, _ISA, OP>((A), (OUT)); return; \
    case ::bee::DType::I64: cpu_elementwise_unary<int64_t, _ISA, OP>((A), (OUT)); return; \
    default: return;                                                                      \
    }

    // ─── 二元 elementwise ────────────────────────────────────────────────────────
    auto ew_add(const Tensor& a, const Tensor& b, Tensor& out) -> void
    {
        BEE_EW_BIN_DTYPE_DISPATCH(OpAdd, a, b, out);
    }
    auto ew_sub(const Tensor& a, const Tensor& b, Tensor& out) -> void
    {
        BEE_EW_BIN_DTYPE_DISPATCH(OpSub, a, b, out);
    }
    auto ew_mul(const Tensor& a, const Tensor& b, Tensor& out) -> void
    {
        BEE_EW_BIN_DTYPE_DISPATCH(OpMul, a, b, out);
    }
    auto ew_div(const Tensor& a, const Tensor& b, Tensor& out) -> void
    {
        BEE_EW_BIN_DTYPE_DISPATCH(OpDiv, a, b, out);
    }

    // ─── 一元 elementwise ────────────────────────────────────────────────────────
    auto ew_neg(const Tensor& a, Tensor& out) -> void
    {
        BEE_EW_UN_ANY_DTYPE_DISPATCH(OpNeg, a, out);
    }

    auto ew_abs(const Tensor& a, Tensor& out) -> void
    {
        BEE_EW_UN_ANY_DTYPE_DISPATCH(OpAbs, a, out);
    }

    auto ew_sqrt(const Tensor& a, Tensor& out) -> void
    {
        BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OpSqrt, a, out);
    }
    auto ew_exp(const Tensor& a, Tensor& out) -> void
    {
        BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OpExp, a, out);
    }
    auto ew_log(const Tensor& a, Tensor& out) -> void
    {
        BEE_EW_UN_FLOAT_DTYPE_DISPATCH(OpLog, a, out);
    }

// ─── 全局 reduce ─────────────────────────────────────────────────────────────
#define BEE_RD_GLOBAL_DTYPE_DISPATCH(OP, A, OUT)                                               \
    switch ((A).dtype()) {                                                                     \
    case ::bee::DType::F32: cpu_reduce_global_dispatch<float, _ISA, OP>((A), (OUT)); return;   \
    case ::bee::DType::F64: cpu_reduce_global_dispatch<double, _ISA, OP>((A), (OUT)); return;  \
    case ::bee::DType::I32: cpu_reduce_global_dispatch<int32_t, _ISA, OP>((A), (OUT)); return; \
    case ::bee::DType::I64: cpu_reduce_global_dispatch<int64_t, _ISA, OP>((A), (OUT)); return; \
    case ::bee::DType::U8: cpu_reduce_global_dispatch<uint8_t, _ISA, OP>((A), (OUT)); return;  \
    default: return;                                                                           \
    }

    auto rd_sum_global(const Tensor& a, Tensor& out) -> void
    {
        BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceSum, a, out);
    }
    auto rd_min_global(const Tensor& a, Tensor& out) -> void
    {
        BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceMin, a, out);
    }
    auto rd_max_global(const Tensor& a, Tensor& out) -> void
    {
        BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceMax, a, out);
    }
    auto rd_prod_global(const Tensor& a, Tensor& out) -> void
    {
        BEE_RD_GLOBAL_DTYPE_DISPATCH(OpReduceProd, a, out);
    }

    // ─── matmul ──────────────────────────────────────────────────────────────────
    // 选择 GEMM 实现命名空间：AVX512 复用 AVX2（x86 下 AVX512F 蕴含 AVX2）
#if defined(BEE_DISPATCH_ISA_AVX512)
    namespace gemm_impl = ::bee::cpu::gemm::avx2;
#elif defined(BEE_DISPATCH_ISA_AVX2)
    namespace gemm_impl = ::bee::cpu::gemm::avx2;
#elif defined(BEE_DISPATCH_ISA_SSE2)
    namespace gemm_impl = ::bee::cpu::gemm::sse2;
#else
    namespace gemm_impl = ::bee::cpu::gemm::scalar;
#endif

    auto mm_f32(int64_t M, int64_t K, int64_t N, const float* A, const float* B, float* C) -> void
    {
        std::memset(C, 0, static_cast<size_t>(M) * N * sizeof(float));
        gemm_impl::gemm_f32(M, K, N, A, B, C);
    }
    auto mm_f64(int64_t M, int64_t K, int64_t N, const double* A, const double* B, double* C) -> void
    {
        std::memset(C, 0, static_cast<size_t>(M) * N * sizeof(double));
        gemm_impl::gemm_f64(M, K, N, A, B, C);
    }
    auto mm_i32(int64_t M, int64_t K, int64_t N, const int32_t* A, const int32_t* B, int32_t* C) -> void
    {
        std::memset(C, 0, static_cast<size_t>(M) * N * sizeof(int32_t));
        gemm_impl::gemm_i32(M, K, N, A, B, C);
    }
    auto mm_i64(int64_t M, int64_t K, int64_t N, const int64_t* A, const int64_t* B, int64_t* C) -> void
    {
        // I64 无 SIMD GEMM 实现，沿用原模板化内核（它自身初始化 C）
        cpu_matmul_kernel<int64_t, _ISA>(M, K, N, A, B, C);
    }
    auto mm_i8(int64_t M, int64_t K, int64_t N, const int8_t* A, const int8_t* B, int32_t* C) -> void
    {
        std::memset(C, 0, static_cast<size_t>(M) * N * sizeof(int32_t));
        gemm_impl::gemm_i8_i32(M, K, N, A, B, C);
    }

    // ─── dtype 转换 ───────────────────────────────────────────────────────────────
    auto ct_cast(::bee::DType src_dt, ::bee::DType dst_dt, const void* src, void* dst, int64_t n) -> void
    {
        cpu_cast_dispatch<_ISA>(src_dt, dst_dt, src, dst, n);
    }

    // ─── 2D strided→contiguous 拷贝 ──────────────────────────────────────────────
    auto tr_copy_2d(
        const void* src,
        void*       dst,
        int64_t     rows,
        int64_t     cols,
        int64_t     src_row_stride_elems,
        int64_t     src_col_stride_elems,
        std::size_t elem_sz
    ) -> void
    {
        cpu_transpose_2d_dispatch<_ISA>(src, dst, rows, cols, src_row_stride_elems, src_col_stride_elems, elem_sz);
    }

} // namespace BEE_CURRENT_NS
} // namespace bee::cpu
