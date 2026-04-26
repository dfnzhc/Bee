/**
 * @File Ops/MatmulTmaWmma.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Native matmul 的 Blackwell 专用 TMA + WMMA F32 实现。
 *
 * 本文件只负责当前 Native 后端中依赖 TensorMap/TMA 的专用路径：
 * - 设备侧使用 `ArchDispatch.cuh` 的能力宏裁剪 Blackwell 专属代码；
 * - host 侧在当前设备不满足要求时返回明确错误；
 * - 对 sm_89 等缺少 TMA 的设备暂不补实现，只保留 TODO 说明原因。
 */

#include "CUDA/Core/ArchDispatch.cuh"
#include "CUDA/Core/Check.cuh"

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

    constexpr int FRAG_M = 16;
    constexpr int FRAG_N = 16;
    constexpr int FRAG_K = 8;

    [[maybe_unused]] constexpr int FM_TILES = WM / FRAG_M; // 4
    [[maybe_unused]] constexpr int FN_TILES = WN / FRAG_N; // 4
    [[maybe_unused]] constexpr int FK_TILES = BK / FRAG_K; // 4

    // 单个 stage 的 shared 布局：连续存放 A/B tile，不额外插入 padding。
    struct alignas(128) StageTile
    {
        float A[BM][BK];
        float B[BK][BN];
    };

    constexpr int kStageBytes = sizeof(StageTile); // 32 KB

    [[nodiscard]] auto current_device_supports_native_tma_wmma() noexcept -> bool
    {
        int dev = 0;
        if (cudaGetDevice(&dev) != cudaSuccess) {
            (void)cudaGetLastError();
            return false;
        }

        cudaDeviceProp prop{};
        if (cudaGetDeviceProperties(&prop, dev) != cudaSuccess) {
            (void)cudaGetLastError();
            return false;
        }

        return prop.major * 100 + prop.minor * 10 >= 1200;
    }

    __global__ __launch_bounds__(NUM_THREADS, 1) void gemm_tma_wmma_tf32_kernel(
        const __grid_constant__ CUtensorMap tmap_a,
        const __grid_constant__ CUtensorMap tmap_b,
        float* __restrict__ C,
        int M,
        int N,
        int K
    )
    {
#if BEE_CUDA_HAS_NATIVE_TMA_WMMA
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

        // arrival_token 不能默认构造，这里用原始字节缓冲延后原地构造。
        alignas(barrier::arrival_token) std::uint8_t token_buf[STAGES][sizeof(barrier::arrival_token)];

        auto token_at = [&](int s) -> barrier::arrival_token& {
            return *reinterpret_cast<barrier::arrival_token*>(&token_buf[s][0]);
        };

        auto issue_tma = [&](int kt, int stage) {
            // A: [M, K] row-major，TMA 坐标为 (k, y)。
            // B: [K, N] row-major，TMA 坐标为 (x, k)。
            if (threadIdx.x == 0) {
                cde::cp_async_bulk_tensor_2d_global_to_shared(&stages[stage].A[0][0], &tmap_a, kt * BK, tile_y, bars[stage]);
                cde::cp_async_bulk_tensor_2d_global_to_shared(&stages[stage].B[0][0], &tmap_b, tile_x, kt * BK, bars[stage]);
                ::new (&token_at(stage)) barrier::arrival_token(::cuda::device::barrier_arrive_tx(bars[stage], 1, kStageBytes));
            } else {
                ::new (&token_at(stage)) barrier::arrival_token(bars[stage].arrive());
            }
        };

        // Prologue：先填满流水线可容纳的 stage。
        const int prologue = K_TILES < STAGES ? K_TILES : STAGES;
        BEE_UNROLL
        for (int s = 0; s < STAGES; ++s) {
            if (s < prologue) {
                issue_tma(s, s);
            }
        }

        // 初始化每个 warp 负责的累加器分块。
        using namespace nvcuda::wmma;
        fragment<accumulator, FRAG_M, FRAG_N, FRAG_K, float> c_frag[FM_TILES][FN_TILES];
        BEE_UNROLL
        for (int i = 0; i < FM_TILES; ++i) {
            BEE_UNROLL
            for (int j = 0; j < FN_TILES; ++j) {
                fill_fragment(c_frag[i][j], 0.0f);
            }
        }

        // 主循环：等待当前 stage 完成，然后执行对应 K 分块的 WMMA 累加。
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

            // 维持固定深度流水线，按需把下一个 K 分块注入当前 stage。
            const int next_kt = kt + STAGES;
            if (next_kt < K_TILES) {
                issue_tma(next_kt, stage);
            }
        }

        // 写回当前 block 覆盖的 C 子块。
        BEE_UNROLL
        for (int i = 0; i < FM_TILES; ++i) {
            BEE_UNROLL
            for (int j = 0; j < FN_TILES; ++j) {
                const int crow = tile_y + warp_row * WM + i * FRAG_M;
                const int ccol = tile_x + warp_col * WN + j * FRAG_N;
                store_matrix_sync(&C[crow * N + ccol], c_frag[i][j], N, mem_row_major);
            }
        }
#else
        (void)tmap_a;
        (void)tmap_b;
        (void)C;
        (void)M;
        (void)N;
        (void)K;
#endif
    }

    // 构造二维 row-major TensorMap：
    // - dim0（最快变化维）对应 inner；
    // - dim1 对应 outer；
    // - stride 数组长度为 rank - 1，单位为字节。
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
    if (!current_device_supports_native_tma_wmma()) {
        // TODO(df): Ada(sm_89) 缺少当前实现依赖的 TensorMap/TMA 搬运能力。
        // 后续若补 Ada 专用 Native kernel，应在此处按能力继续细分。
        return static_cast<int>(cudaErrorNotSupported);
    }

    // 仅接受当前 Blackwell Native kernel 已验证的规整形状与对齐约束。
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
