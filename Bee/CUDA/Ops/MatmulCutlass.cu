/**
 * @File Ops/MatmulCutlass.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief B8：CUTLASS 2.x Ampere TF32 F32 GEMM（sm_120 向下兼容运行）。
 *
 * 设计要点：
 * - 仅 F32（A/B/C 全 F32，累加 F32）；输入在 TF32 tensor core 上按 TF32 截断参与 WMMA，
 *   因此会损失 ~10 位尾数精度，适合 CNN/MLP 典型工作负载。
 * - Threadblock 128×128×32、Warp 64×64×32、Instruction 16×8×8（TF32 WMMA），3-stage pipeline。
 * - 对齐要求：M/N/K 均需为 4 的倍数（TF32 Gemm alignment = 4 × sizeof(float) = 16 B）。
 *   任一维度不满足时返回非零 sentinel，由 Matmul.cu 回退到手写 tile kernel。
 *
 * 独立 TU 编译，只在 BEE_HAS_CUTLASS=1 时被加入构建（CMakeLists.txt 控制）。
 */

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>

// CUTLASS 4.x 仍然保留 2.x device API
#include <cutlass/cutlass.h>
#include <cutlass/numeric_types.h>
#include <cutlass/gemm/device/gemm.h>
#include <cutlass/epilogue/thread/linear_combination.h>

namespace bee::cuda::detail
{

namespace
{

using ElementIn  = float;
using ElementOut = float;
using ElementAcc = float;

using LayoutA = cutlass::layout::RowMajor;
using LayoutB = cutlass::layout::RowMajor;
using LayoutC = cutlass::layout::RowMajor;

using ThreadblockShape = cutlass::gemm::GemmShape<128, 128, 32>;
using WarpShape        = cutlass::gemm::GemmShape<64, 64, 32>;
using InstructionShape = cutlass::gemm::GemmShape<16, 8, 8>; // TF32 WMMA

using EpilogueOp = cutlass::epilogue::thread::LinearCombination<
    ElementOut,
    128 / cutlass::sizeof_bits<ElementOut>::value,
    ElementAcc,
    ElementAcc>;

using GemmKernel = cutlass::gemm::device::Gemm<
    ElementIn,  LayoutA,
    ElementIn,  LayoutB,
    ElementOut, LayoutC,
    ElementAcc,
    cutlass::arch::OpClassTensorOp,
    cutlass::arch::Sm80,
    ThreadblockShape,
    WarpShape,
    InstructionShape,
    EpilogueOp,
    cutlass::gemm::threadblock::GemmIdentityThreadblockSwizzle<>,
    3 // stages
>;

} // namespace

// 返回 0 表示成功；非 0 表示 CUDA / CUTLASS 错误或配置不支持。
// - 若 M/N/K 不满足 4 对齐，返回 cudaErrorInvalidConfiguration（由调用方回退）。
int ops_matmul_f32_cutlass(const void* A,
                           const void* B,
                           void*       C,
                           std::size_t M,
                           std::size_t K,
                           std::size_t N,
                           cudaStream_t stream) noexcept
{
    // TF32 tensor op 要求 K / 向量化维度 4 对齐（亦即 A 的列、B 的列、C 的列）。
    if ((M % 4) != 0 || (N % 4) != 0 || (K % 4) != 0) {
        return static_cast<int>(cudaErrorInvalidConfiguration);
    }

    GemmKernel gemm;

    typename GemmKernel::Arguments args(
        {static_cast<int>(M), static_cast<int>(N), static_cast<int>(K)},
        {static_cast<const ElementIn*>(A), static_cast<int>(K)}, // lda = K
        {static_cast<const ElementIn*>(B), static_cast<int>(N)}, // ldb = N
        {static_cast<ElementOut*>(C),      static_cast<int>(N)}, // ldc = N
        {static_cast<ElementOut*>(C),      static_cast<int>(N)}, // ldd = N
        {ElementAcc(1), ElementAcc(0)}                            // alpha, beta
    );

    cutlass::Status st = gemm.can_implement(args);
    if (st != cutlass::Status::kSuccess) {
        return static_cast<int>(cudaErrorInvalidConfiguration);
    }

    st = gemm.initialize(args, nullptr, stream);
    if (st != cutlass::Status::kSuccess) {
        return static_cast<int>(cudaErrorInvalidConfiguration);
    }

    st = gemm.run(stream);
    if (st != cutlass::Status::kSuccess) {
        return static_cast<int>(cudaErrorLaunchFailure);
    }

    return 0;
}

} // namespace bee::cuda::detail
