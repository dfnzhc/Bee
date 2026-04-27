/**
 * @File Ops/AiPrimitives.cu
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 原生 CUDA RMSNorm、RoPE、Embedding kernel 实现。
 *        支持 F32/F64 数据类型；Embedding ids 支持 I32/I64。
 *        所有入口均在返回前同步，对调用方表现为同步 API。
 */

#include "CUDA/Ops/OpsBridge.hpp"
#include "CUDA/Core/Launch.cuh"

#include <cuda_runtime.h>

#include <cmath>
#include <cstdint>

namespace
{

// dtype 编码（与 Api.hpp ScalarType 整型编码一致）
constexpr int kDtI32 = 2;
constexpr int kDtI64 = 3;
constexpr int kDtF32 = 4;
constexpr int kDtF64 = 5;

constexpr std::size_t kMaxGridX = 2147483647ull;

// ─── RMSNorm kernel ───────────────────────────────────────────────────────────
//
// 每个 block 负责一行数据；使用 double 精度共享内存做平方和规约。
// blockDim.x 固定为 256，通过 stride 循环处理 dim > 256 的情况。

template <typename T>
__global__ void rms_norm_kernel(
    const T* __restrict__ x,
    const T* __restrict__ w,
    T* __restrict__       out,
    std::size_t           rows,
    std::size_t           dim,
    double                eps
)
{
    // 每个 block 处理行 blockIdx.x
    const std::size_t row = static_cast<std::size_t>(blockIdx.x);
    if (row >= rows)
        return;

    const T* x_row = x   + row * dim;
    T*       o_row = out + row * dim;

    // 共享内存用于块内 double 规约（调用方负责传入 blockDim.x * sizeof(double) 字节）
    extern __shared__ double smem[];

    // 每线程累加本线程负责的元素平方和
    double local_sum2 = 0.0;
    for (std::size_t i = static_cast<std::size_t>(threadIdx.x); i < dim; i += static_cast<std::size_t>(blockDim.x)) {
        const double v  = static_cast<double>(x_row[i]);
        local_sum2     += v * v;
    }
    smem[threadIdx.x] = local_sum2;
    __syncthreads();

    // 块内二分规约（blockDim.x 必须为 2 的幂次）
    for (unsigned s = blockDim.x >> 1u; s > 0u; s >>= 1u) {
        if (threadIdx.x < s)
            smem[threadIdx.x] += smem[threadIdx.x + s];
        __syncthreads();
    }

    // thread 0 写入 rms_inv，其余线程通过共享内存读取
    if (threadIdx.x == 0)
        smem[0] = 1.0 / sqrt(smem[0] / static_cast<double>(dim) + eps);
    __syncthreads();

    const double rms_inv = smem[0];

    // 写归一化输出
    for (std::size_t i = static_cast<std::size_t>(threadIdx.x); i < dim; i += static_cast<std::size_t>(blockDim.x))
        o_row[i] = static_cast<T>(static_cast<double>(x_row[i]) * rms_inv * static_cast<double>(w[i]));
}

template <typename T>
auto launch_rms_norm(
    const void*  x,
    const void*  w,
    void*        out,
    std::size_t  rows,
    std::size_t  dim,
    double       eps,
    cudaStream_t stream
) -> int
{
    // blockDim 固定为 256（2 的幂次，stride 循环覆盖任意 dim）
    constexpr unsigned block = 256u;
    const std::size_t  smem  = block * sizeof(double);
    const unsigned     grid  = static_cast<unsigned>(rows);

    rms_norm_kernel<T><<<grid, block, smem, stream>>>(
        static_cast<const T*>(x), static_cast<const T*>(w), static_cast<T*>(out), rows, dim, eps
    );
    return static_cast<int>(cudaGetLastError());
}

// ─── RoPE kernel ──────────────────────────────────────────────────────────────
//
// 每线程处理一个 (token, pair_i) 对，实现 split-half 旋转：
//   (x[i], x[i + dim/2]) → 旋转后的 (out[i], out[i + dim/2])
// 总任务数 = n_batch * seq_len * (dim/2)。

template <typename T>
__global__ void rope_kernel(
    const T* __restrict__ x,
    T* __restrict__       out,
    std::size_t           n_batch,
    std::size_t           seq_len,
    std::size_t           dim,
    double                base,
    std::int64_t          position_offset
)
{
    const std::size_t half_dim    = dim >> 1u;
    const std::size_t total_pairs = n_batch * seq_len * half_dim;
    const std::size_t tid         = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (tid >= total_pairs)
        return;

    // 解码：pair_i ∈ [0, half_dim)，token 为展平后的 token 索引
    const std::size_t pair_i = tid % half_dim;
    const std::size_t token  = tid / half_dim;
    const std::size_t s      = token % seq_len; // 在序列内的位置

    const double pos   = static_cast<double>(position_offset + static_cast<std::int64_t>(s));
    const double theta = pos / pow(base, 2.0 * static_cast<double>(pair_i) / static_cast<double>(dim));

    const double cos_t = cos(theta);
    const double sin_t = sin(theta);

    // split-half 配对：(x[pair_i], x[pair_i + half_dim])
    const std::size_t row_offset = token * dim;
    const double      x0         = static_cast<double>(x[row_offset + pair_i]);
    const double      x1         = static_cast<double>(x[row_offset + pair_i + half_dim]);

    // 旋转变换：(x0, x1) → (x0·cos - x1·sin, x0·sin + x1·cos)
    out[row_offset + pair_i]           = static_cast<T>(x0 * cos_t - x1 * sin_t);
    out[row_offset + pair_i + half_dim] = static_cast<T>(x0 * sin_t + x1 * cos_t);
}

template <typename T>
auto launch_rope(
    const void*  x,
    void*        out,
    std::size_t  n_batch,
    std::size_t  seq_len,
    std::size_t  dim,
    double       base,
    std::int64_t position_offset,
    cudaStream_t stream
) -> int
{
    const std::size_t half_dim    = dim / 2;
    const std::size_t total_pairs = n_batch * seq_len * half_dim;
    const unsigned    block       = bee::cuda::kDefaultBlockSize;
    const unsigned    grid        = bee::cuda::compute_grid_1d(total_pairs, block);

    rope_kernel<T><<<grid, block, 0, stream>>>(
        static_cast<const T*>(x), static_cast<T*>(out), n_batch, seq_len, dim, base, position_offset
    );
    return static_cast<int>(cudaGetLastError());
}

// ─── Embedding kernel ─────────────────────────────────────────────────────────
//
// 每线程处理一个输出元素 (id_idx, h)；越界 id 通过原子标志上报错误，
// 同步后由 host 检查标志并返回 cudaErrorInvalidValue。

template <typename T, typename IdT>
__global__ void embedding_kernel(
    const T* __restrict__   weight,
    const IdT* __restrict__ ids,
    T* __restrict__         out,
    std::size_t             n_ids,
    std::size_t             hidden,
    std::int64_t            vocab,
    int* __restrict__       error_flag
)
{
    const std::size_t tid = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (tid >= n_ids * hidden)
        return;

    const std::size_t  id_idx = tid / hidden;
    const std::size_t  h      = tid % hidden;
    const std::int64_t id     = static_cast<std::int64_t>(ids[id_idx]);

    if (id < 0 || id >= vocab) {
        // 设备侧越界标志，原子写入保证无竞态
        atomicExch(error_flag, 1);
        return;
    }

    out[tid] = weight[static_cast<std::size_t>(id) * hidden + h];
}

template <typename T, typename IdT>
auto launch_embedding_typed(
    const void*  weight,
    const void*  ids,
    void*        out,
    std::size_t  n_ids,
    std::size_t  hidden,
    std::size_t  vocab,
    cudaStream_t stream
) -> int
{
    // 分配设备侧越界标志（单个 int）
    int*        d_flag   = nullptr;
    cudaError_t alloc_err = cudaMalloc(&d_flag, sizeof(int));
    if (alloc_err != cudaSuccess)
        return static_cast<int>(alloc_err);

    // 初始化标志为 0
    cudaError_t memset_err = cudaMemsetAsync(d_flag, 0, sizeof(int), stream);
    if (memset_err != cudaSuccess) {
        (void)cudaFree(d_flag);
        return static_cast<int>(memset_err);
    }

    const std::size_t total = n_ids * hidden;
    const unsigned    block = bee::cuda::kDefaultBlockSize;
    const unsigned    grid  = bee::cuda::compute_grid_1d(total, block);

    embedding_kernel<T, IdT><<<grid, block, 0, stream>>>(
        static_cast<const T*>(weight),
        static_cast<const IdT*>(ids),
        static_cast<T*>(out),
        n_ids,
        hidden,
        static_cast<std::int64_t>(vocab),
        d_flag
    );

    // 检查启动错误
    const cudaError_t launch_err = cudaGetLastError();
    if (launch_err != cudaSuccess) {
        (void)cudaFree(d_flag);
        return static_cast<int>(launch_err);
    }

    // 同步后才能安全读取越界标志
    const cudaError_t sync_err = cudaStreamSynchronize(stream);
    if (sync_err != cudaSuccess) {
        (void)cudaFree(d_flag);
        return static_cast<int>(sync_err);
    }

    // 从设备读取越界标志
    int         flag_val = 0;
    cudaError_t copy_err = cudaMemcpy(&flag_val, d_flag, sizeof(int), cudaMemcpyDeviceToHost);
    (void)cudaFree(d_flag);

    if (copy_err != cudaSuccess)
        return static_cast<int>(copy_err);
    if (flag_val != 0)
        return static_cast<int>(cudaErrorInvalidValue);

    return 0;
}

} // 匿名命名空间

namespace bee::cuda::detail
{

// ─── ops_rms_norm ─────────────────────────────────────────────────────────────

int ops_rms_norm(int dt, const void* x, const void* w, void* out,
                 std::size_t rows, std::size_t dim, double eps) noexcept
{
    if (!x || !w || !out || rows == 0 || dim == 0 || !std::isfinite(eps) || eps <= 0.0)
        return static_cast<int>(cudaErrorInvalidValue);
    if (rows > kMaxGridX)
        return static_cast<int>(cudaErrorInvalidValue);

    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    switch (dt) {
    case kDtF32: err = launch_rms_norm<float> (x, w, out, rows, dim, eps, stream); break;
    case kDtF64: err = launch_rms_norm<double>(x, w, out, rows, dim, eps, stream); break;
    default:     return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

// ─── ops_rope ─────────────────────────────────────────────────────────────────

int ops_rope(int dt, const void* x, void* out,
             std::size_t n_batch, std::size_t seq_len, std::size_t dim,
             double base, std::int64_t position_offset) noexcept
{
    if (!x || !out || n_batch == 0 || seq_len == 0 || dim == 0 || dim % 2 != 0)
        return static_cast<int>(cudaErrorInvalidValue);
    if (!std::isfinite(base) || base <= 0.0)
        return static_cast<int>(cudaErrorInvalidValue);

    cudaStream_t stream = cudaStreamPerThread;
    int          err    = 0;

    switch (dt) {
    case kDtF32: err = launch_rope<float> (x, out, n_batch, seq_len, dim, base, position_offset, stream); break;
    case kDtF64: err = launch_rope<double>(x, out, n_batch, seq_len, dim, base, position_offset, stream); break;
    default:     return static_cast<int>(cudaErrorInvalidValue);
    }

    if (err != 0)
        return err;
    return static_cast<int>(cudaStreamSynchronize(stream));
}

// ─── ops_embedding ────────────────────────────────────────────────────────────

int ops_embedding(int weight_dt, int ids_dt,
                  const void* weight, const void* ids, void* out,
                  std::size_t n_ids, std::size_t hidden, std::size_t vocab) noexcept
{
    if (!weight || !ids || !out || n_ids == 0 || hidden == 0 || vocab == 0)
        return static_cast<int>(cudaErrorInvalidValue);

    cudaStream_t stream = cudaStreamPerThread;

    // 按 weight_dt × ids_dt 组合分派（launch_embedding_typed 内部含同步）
    if (weight_dt == kDtF32 && ids_dt == kDtI32)
        return launch_embedding_typed<float,  std::int32_t>(weight, ids, out, n_ids, hidden, vocab, stream);
    if (weight_dt == kDtF32 && ids_dt == kDtI64)
        return launch_embedding_typed<float,  std::int64_t>(weight, ids, out, n_ids, hidden, vocab, stream);
    if (weight_dt == kDtF64 && ids_dt == kDtI32)
        return launch_embedding_typed<double, std::int32_t>(weight, ids, out, n_ids, hidden, vocab, stream);
    if (weight_dt == kDtF64 && ids_dt == kDtI64)
        return launch_embedding_typed<double, std::int64_t>(weight, ids, out, n_ids, hidden, vocab, stream);

    return static_cast<int>(cudaErrorInvalidValue);
}

} // namespace bee::cuda::detail
