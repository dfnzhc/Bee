/**
 * @File Ops/MatmulTmaWmma.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief B10：手写 TMA + WMMA TF32 F32 GEMM，针对 sm_120 (RTX 50 系) 优化。
 *
 * 设计要点：
 * - 数据流：
 *   * Host 端用 driver API `cuTensorMapEncodeTiled` 为 A/B 各创建一个 2D `CUtensorMap`，
 *     通过 `__grid_constant__` 传入 kernel；
 *   * Kernel 中由 thread 0 触发 `cp.async.bulk.tensor.2d.shared::cluster.global`
 *     将 BM×BK / BK×BN tile 一次性载入 shared，配合 mbarrier 同步异步完成；
 *   * 计算阶段所有 warp 用 nvcuda::wmma TF32 fragment（m16n16k8）做 mma.sync。
 *
 * - Tile 配置：
 *   * Threadblock tile  BM×BN×BK = 128×128×32
 *   * 每 block 4 warps（2×2 warp grid），每 warp 处理 64×64 输出（4×4 m16n16 fragment）
 *   * 主 K 循环每 stage 4 个 m16n16k8 子积（K=32/8=4）
 *   * 流水线深度 STAGES=3，shared 用量 = 3 × 32 KB = 96 KB（>48 KB 默认上限，需
 *     `cudaFuncSetAttribute(cudaFuncAttributeMaxDynamicSharedMemorySize, ...)` 提升至 100 KB）
 *
 * - 触发条件：F32、M/N/K 分别是 BM/BN/BK 的倍数、A/B/C 设备指针 16B 对齐。
 *   任一不满足时返回非零 sentinel，由 Matmul.cu 回退到 CUTLASS 或手写 tile。
 *
 * - 与 B8 (CUTLASS 2.x Ampere TF32) 关系：并行存在；二者性能曲线在不同 size
 *   有差异（B10 主攻流水线/异步 supply，B8 是 cp.async.ca 路径）。
 */

#include "Core/Check.cuh"

#include <cuda.h> // CUtensorMap 与 cuTensorMapEncodeTiled
#include <cuda_runtime.h>
#include <cuda/barrier> // cuda::barrier + cp_async_bulk_tensor_2d_global_to_shared
#include <mma.h>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace bee::cuda::detail
{

namespace
{

    namespace cde = ::cuda::device::experimental;
    using barrier = ::cuda::barrier<::cuda::thread_scope_block>;

    constexpr int BM          = 128;
    constexpr int BN          = 128;
    constexpr int BK          = 32;
    constexpr int WM          = 64;
    constexpr int WN          = 64;
    constexpr int WARPS_M     = BM / WM;           // 2
    constexpr int WARPS_N     = BN / WN;           // 2
    constexpr int NUM_WARPS   = WARPS_M * WARPS_N; // 4
    constexpr int NUM_THREADS = NUM_WARPS * 32;    // 128
    constexpr int STAGES      = 3;

    constexpr int FRAG_M = 16, FRAG_N = 16, FRAG_K = 8;
    constexpr int FM_TILES = WM / FRAG_M; // 4
    constexpr int FN_TILES = WN / FRAG_N; // 4
    constexpr int FK_TILES = BK / FRAG_K; // 4

    // 单 stage 的 shared 布局：紧凑放置 A/B tile（无 padding）。
    // load_matrix_sync 端会发生 bank conflict（约 10-15% smem 吞吐损失），
    // 通过双缓冲 + 大量计算覆盖。
    struct alignas(128) StageTile
    {
        float A[BM][BK];
        float B[BK][BN];
    };

    constexpr int kStageBytes = sizeof(StageTile); // 32 KB

    __global__ __launch_bounds__(NUM_THREADS, 1) void gemm_tma_wmma_tf32_kernel(
        const __grid_constant__ CUtensorMap tmap_a,
        const __grid_constant__ CUtensorMap tmap_b,
        float* __restrict__ C,
        int M,
        int N,
        int K
    )
    {
        extern __shared__ __align__(128) std::uint8_t smem_raw[];

        StageTile* stages = reinterpret_cast<StageTile*>(smem_raw);

        __shared__ barrier bars[STAGES];
        if (threadIdx.x == 0) {
            BEE_UNROLL
            for (int s = 0; s < STAGES; ++s) {
                ::new (&bars[s]) barrier(NUM_THREADS);
            }
            cde::fence_proxy_async_shared_cta();
        }
        __syncthreads();

        const int warp_id  = threadIdx.x >> 5;
        const int warp_row = warp_id / WARPS_N; // 0..1
        const int warp_col = warp_id % WARPS_N; // 0..1

        const int tile_y  = blockIdx.y * BM; // M offset
        const int tile_x  = blockIdx.x * BN; // N offset
        const int K_TILES = K / BK;

        // tokens 在 register 中追踪每 stage 的 arrival_token。
        // arrival_token 不可默认构造，使用 raw 字节缓冲规避，prologue/主循环必定在 wait 前赋值。
        alignas(barrier::arrival_token) std::uint8_t token_buf[STAGES][sizeof(barrier::arrival_token)];

        auto token_at = [&](int s) -> barrier::arrival_token& {
            return *reinterpret_cast<barrier::arrival_token*>(&token_buf[s][0]);
        };

        auto issue_tma = [&](int kt, int stage) {
            // A: [M,K] row-major，TMA dim0(fastest)=K, dim1=M；coord (k, y)
            // B: [K,N] row-major，TMA dim0(fastest)=N, dim1=K；coord (x, k)
            if (threadIdx.x == 0) {
                cde::cp_async_bulk_tensor_2d_global_to_shared(&stages[stage].A[0][0], &tmap_a, kt * BK, tile_y, bars[stage]);
                cde::cp_async_bulk_tensor_2d_global_to_shared(&stages[stage].B[0][0], &tmap_b, tile_x, kt * BK, bars[stage]);
                ::new (&token_at(stage)) barrier::arrival_token(::cuda::device::barrier_arrive_tx(bars[stage], 1, kStageBytes));
            } else {
                ::new (&token_at(stage)) barrier::arrival_token(bars[stage].arrive());
            }
        };

        // ── Prologue：注入前 STAGES 个 K-tile 的 TMA 加载 ───────────────────
        const int prologue = K_TILES < STAGES ? K_TILES : STAGES;
        BEE_UNROLL
        for (int s = 0; s < STAGES; ++s) {
            if (s < prologue) {
                issue_tma(s, s);
            }
        }

        // ── 累加器初始化 ───────────────────────────────────────────────────
        using namespace nvcuda::wmma;
        fragment<accumulator, FRAG_M, FRAG_N, FRAG_K, float> c_frag[FM_TILES][FN_TILES];
        BEE_UNROLL
        for (int i = 0; i < FM_TILES; ++i) {
            BEE_UNROLL
            for (int j = 0; j < FN_TILES; ++j) {
                fill_fragment(c_frag[i][j], 0.0f);
            }
        }

        // ── 主循环 ─────────────────────────────────────────────────────────
        for (int kt = 0; kt < K_TILES; ++kt) {
            const int stage = kt % STAGES;

            bars[stage].wait(std::move(token_at(stage)));

            StageTile& tile = stages[stage];

            fragment<matrix_a, FRAG_M, FRAG_N, FRAG_K, precision::tf32, row_major> a_frag[FM_TILES];
            fragment<matrix_b, FRAG_M, FRAG_N, FRAG_K, precision::tf32, row_major> b_frag[FN_TILES];

            BEE_UNROLL
            for (int kf = 0; kf < FK_TILES; ++kf) {
                BEE_UNROLL
                for (int i = 0; i < FM_TILES; ++i) {
                    load_matrix_sync(a_frag[i], &tile.A[warp_row * WM + i * FRAG_M][kf * FRAG_K], BK);
                    BEE_UNROLL
                    for (int t = 0; t < a_frag[i].num_elements; ++t) {
                        a_frag[i].x[t] = __float_to_tf32(a_frag[i].x[t]);
                    }
                }
                BEE_UNROLL
                for (int j = 0; j < FN_TILES; ++j) {
                    load_matrix_sync(b_frag[j], &tile.B[kf * FRAG_K][warp_col * WN + j * FRAG_N], BN);
                    BEE_UNROLL
                    for (int t = 0; t < b_frag[j].num_elements; ++t) {
                        b_frag[j].x[t] = __float_to_tf32(b_frag[j].x[t]);
                    }
                }
                BEE_UNROLL
                for (int i = 0; i < FM_TILES; ++i) {
                    BEE_UNROLL
                    for (int j = 0; j < FN_TILES; ++j) {
                        mma_sync(c_frag[i][j], a_frag[i], b_frag[j], c_frag[i][j]);
                    }
                }
            }

            // 流水线尾部预取
            const int next_kt = kt + STAGES;
            if (next_kt < K_TILES) {
                issue_tma(next_kt, stage);
            }
        }

        // ── 写回 C ─────────────────────────────────────────────────────────
        BEE_UNROLL
        for (int i = 0; i < FM_TILES; ++i) {
            BEE_UNROLL
            for (int j = 0; j < FN_TILES; ++j) {
                const int crow = tile_y + warp_row * WM + i * FRAG_M;
                const int ccol = tile_x + warp_col * WN + j * FRAG_N;
                store_matrix_sync(&C[crow * N + ccol], c_frag[i][j], N, mem_row_major);
            }
        }
    }

    // ── Host：CUtensorMap 构造 ─────────────────────────────────────────────
    // row-major matrix [outer][inner]：dim0(fastest)=inner，dim1=outer
    // stride 数组长度 = rank-1，单位字节，描述 dim>=1 的步长。
    inline CUresult make_tensor_map_2d(
        CUtensorMap&  out,
        const float*  base,
        std::uint64_t outer,
        std::uint64_t inner,
        std::uint32_t box_outer,
        std::uint32_t box_inner
    ) noexcept
    {
        cuuint64_t size[2]    = {inner, outer};
        cuuint64_t stride[1]  = {inner * sizeof(float)};
        cuuint32_t box[2]     = {box_inner, box_outer};
        cuuint32_t elem_st[2] = {1u, 1u};
        return cuTensorMapEncodeTiled(
            &out,
            CU_TENSOR_MAP_DATA_TYPE_FLOAT32,
            2,
            const_cast<float*>(base),
            size,
            stride,
            box,
            elem_st,
            CU_TENSOR_MAP_INTERLEAVE_NONE,
            CU_TENSOR_MAP_SWIZZLE_NONE,
            CU_TENSOR_MAP_L2_PROMOTION_L2_128B,
            CU_TENSOR_MAP_FLOAT_OOB_FILL_NONE
        );
    }

    inline bool aligned16(const void* p) noexcept
    {
        return (reinterpret_cast<std::uintptr_t>(p) & 0xF) == 0;
    }

    bool g_attr_set = false;

} // namespace

int ops_matmul_f32_tma_wmma(const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N, cudaStream_t stream) noexcept
{
    // 触发条件检查：尺寸对齐 + 指针 16B 对齐
    if ((M % BM) != 0 || (N % BN) != 0 || (K % BK) != 0) {
        return 1;
    }
    if (!aligned16(A) || !aligned16(B) || !aligned16(C)) {
        return 2;
    }
    if (M > INT32_MAX || N > INT32_MAX || K > INT32_MAX) {
        return 3;
    }

    CUtensorMap tmap_a, tmap_b;
    if (auto r = make_tensor_map_2d(tmap_a, static_cast<const float*>(A), static_cast<std::uint64_t>(M), static_cast<std::uint64_t>(K), BM, BK);
        r != CUDA_SUCCESS) {
        return static_cast<int>(cudaErrorInvalidValue);
    }
    if (auto r = make_tensor_map_2d(tmap_b, static_cast<const float*>(B), static_cast<std::uint64_t>(K), static_cast<std::uint64_t>(N), BK, BN);
        r != CUDA_SUCCESS) {
        return static_cast<int>(cudaErrorInvalidValue);
    }

    const int smem_bytes = STAGES * kStageBytes;

    if (!g_attr_set) {
        cudaError_t e = cudaFuncSetAttribute(gemm_tma_wmma_tf32_kernel, cudaFuncAttributeMaxDynamicSharedMemorySize, smem_bytes);
        if (e != cudaSuccess) {
            return static_cast<int>(e);
        }
        g_attr_set = true;
    }

    dim3 block(NUM_THREADS);
    dim3 grid(static_cast<unsigned>(N / BN), static_cast<unsigned>(M / BM));

    gemm_tma_wmma_tf32_kernel<<<grid, block, smem_bytes, stream>>>(
        tmap_a, tmap_b, static_cast<float*>(C), static_cast<int>(M), static_cast<int>(N), static_cast<int>(K)
    );

    return static_cast<int>(cudaGetLastError());
}

} // namespace bee::cuda::detail
