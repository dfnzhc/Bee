/**
 * @File Api.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA 组件对外 C++ 接口。
 *
 * 本头文件是 CUDA 组件的稳定公共入口，不向使用者暴露 cuda_runtime.h。
 * 调用方通过 Bee::Result 接收可恢复错误，具体 cudaError_t 由实现层转换为
 * Bee::Error 的 code 与中文诊断信息。
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "Base/Diagnostics/Error.hpp"

namespace bee::cuda
{

// ── 组件元信息 ──────────────────────────────────────────────────────────────

// 组件名（稳定字符串字面量）
[[nodiscard]] std::string_view component_name() noexcept;

// CUDA Toolkit 版本字符串；来自 CMake 配置阶段探测结果。
[[nodiscard]] std::string_view toolkit_version() noexcept;

// 当前构建是否检测到并启用了 CUTLASS 支持。
[[nodiscard]] bool has_cutlass() noexcept;

// CUDA 运行时桥接翻译单元是否由 nvcc 编译。
[[nodiscard]] bool compiled_with_nvcc() noexcept;

// ── 设备查询 ────────────────────────────────────────────────────────────────

// 设备数量；若驱动不可用或无设备则返回 0
[[nodiscard]] auto device_count() noexcept -> int;

// 将当前线程绑定到指定设备
[[nodiscard]] auto set_device(int device_index) -> Result<void>;

// 阻塞直到当前设备上所有已提交工作完成
[[nodiscard]] auto device_synchronize() -> Result<void>;

// ── 内存 ────────────────────────────────────────────────────────────────────
//
// 默认使用当前设备的 cudaMemPool_t 与 cudaStreamPerThread。allocate()
// 内部通过 cudaMallocFromPoolAsync 分配，并在返回前同步对应 stream，因此
// 对调用方表现为“返回后即可使用”的同步 API。alignment 参数保留给未来
// 特殊对齐需求；当前实现依赖 CUDA 分配器提供的设备指针对齐保证。

[[nodiscard]] auto allocate(std::size_t nbytes, std::size_t alignment) -> Result<void*>;

void deallocate(void* ptr, std::size_t nbytes, std::size_t alignment) noexcept;

// 分配页锁定主机内存，用于提升 H2D/D2H 传输吞吐。
[[nodiscard]] auto allocate_pinned_host(std::size_t nbytes) -> Result<void*>;
auto               free_pinned_host(void* ptr) noexcept -> void;

[[nodiscard]] auto memcpy_h2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;
[[nodiscard]] auto memcpy_d2h(void* dst, const void* src, std::size_t nbytes) -> Result<void>;
[[nodiscard]] auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>;

// 把设备内存 ptr 的前 nbytes 个字节置为 value 的低 8 位。
[[nodiscard]] auto memset(void* ptr, int value, std::size_t nbytes) -> Result<void>;

// ── 元素级算子 ──────────────────────────────────────────────────────────────
//
// 计算模型：输入/输出均为连续设备内存，CUDA 端不解释 Tensor 的广播或
// 非连续 stride 语义；Tensor 层负责在进入这里之前完成 contiguous() 或
// 形状检查。ScalarType 与 bee::DType 共享同一整型编码，便于桥接层直接
// 转换。

enum class ScalarType : std::uint8_t
{
    Bool = 0,
    U8   = 1,
    I32  = 2,
    I64  = 3,
    F32  = 4,
    F64  = 5,
    I8   = 6,
    F16  = 7,
    BF16 = 8,
};

// 二元元素级算子枚举。所有操作都按输入 dtype 的设备端语义执行；整数
// 除法与浮点特殊值处理遵循对应 CUDA C++ 运算符行为。
enum class BinaryOp : std::uint8_t
{
    Add,
    Sub,
    Mul,
    Div,
};

// 一元元素级算子枚举。仅对后端支持的 dtype 组合有效，不支持的组合会
// 通过 Result 返回可恢复错误。
enum class UnaryOp : std::uint8_t
{
    Neg     = 0,
    Abs     = 1,
    Sqrt    = 2,
    Exp     = 3,
    Log     = 4,
    Relu    = 5,
    Sigmoid = 6,
};

// 规约算子枚举。输出 dtype 由调用接口约定决定：Bee::CUDA 底层通常写回
// 同 dtype，Tensor 层可在需要时先转换到累加 dtype。
enum class ReduceOp : std::uint8_t
{
    Sum,
    Min,
    Max,
    Prod,
};

namespace ops
{

    // n 以元素为单位；a/b/out 指向同一 dtype 的连续缓冲。
    [[nodiscard]] auto binary(BinaryOp op, ScalarType dt, const void* a, const void* b, void* out, std::size_t n) -> Result<void>;

    // 一元算子；src/dst 同 dtype、同 shape、均连续。
    [[nodiscard]] auto unary(UnaryOp op, ScalarType dt, const void* src, void* dst, std::size_t n) -> Result<void>;

    // dtype 转换；src/dst 均连续且元素数相同。
    [[nodiscard]] auto cast(ScalarType src_dt, const void* src, ScalarType dst_dt, void* dst, std::size_t n) -> Result<void>;

    // 全局 reduce：src 为连续 n 元素缓冲，dst 指向 1 个同 dtype 标量。
    [[nodiscard]] auto reduce_global(ReduceOp op, ScalarType dt, const void* src, void* dst, std::size_t n) -> Result<void>;

    // 按轴 reduce：输入视为 [outer, axis, inner] 三维连续布局；输出 [outer, inner]。
    // src/dst 同 dtype、均连续；axis 必须 >=1。
    [[nodiscard]] auto reduce_axis(ReduceOp op, ScalarType dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner)
        -> Result<void>;

    // 稳定 softmax：输入视为 [outer, axis, inner] 连续布局；src/dst 同 dtype、同 shape。
    [[nodiscard]] auto softmax(ScalarType dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) -> Result<void>;

    // 原地缩放（dt ∈ {F32, F64}）：供 mean 实现。
    [[nodiscard]] auto scale_fp(ScalarType dt, void* buf, double factor, std::size_t n) -> Result<void>;

    // 2D tiled-shared matmul：C[M,N] = A[M,K] * B[K,N]；A/B/C 同 dtype、均连续。
    [[nodiscard]] auto matmul(ScalarType dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) -> Result<void>;

    // 低精度 GEMM：A/B 为 F16 或 BF16 连续设备缓冲，以 float 累加并写入
    // F32 输出 C[M,N]。
    // 仅接受 dt == F16 或 dt == BF16；有效尺寸下空指针返回错误。
    [[nodiscard]] auto matmul_lowp(ScalarType dt, const void* A, const void* B, float* C, std::size_t M, std::size_t K, std::size_t N)
        -> Result<void>;

    // 2D tiled-shared transpose：dst[i,j] = src[j,i]，src 为 [rows, cols] 连续。
    [[nodiscard]] auto transpose_2d(ScalarType dt, const void* src, void* dst, std::size_t rows, std::size_t cols) -> Result<void>;

    // 通用 strided copy：将 src storage（基地址 + offset_elements 元素偏移）按 shape/strides 物化到连续 dst。
    // shape/strides 为 host 侧指针，ndim 最多 8。
    [[nodiscard]] auto strided_copy(
        ScalarType     dt,
        const void*    src,
        void*          dst,
        const int64_t* shape,
        const int64_t* strides,
        int            ndim,
        int64_t        offset_elements,
        std::size_t    numel
    ) -> Result<void>;

    // ── 设备侧随机数 ───────────────────────────────────────────────────────────
    // Philox4x32-10 on-device：同一 seed 与 numel 下输出确定；dtype 仅支持 F32/F64。
    [[nodiscard]] auto random_uniform(ScalarType dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>;

    // 正态 N(0,1)；dtype 仅支持 F32/F64；内部以 Box-Muller 转换。
    [[nodiscard]] auto random_normal(ScalarType dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>;

    // 整数区间 [low, high)；dtype 支持 U8/I32/I64。
    [[nodiscard]] auto random_int(ScalarType dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) -> Result<void>;

    // ── AI 基础原语 ───────────────────────────────────────────────────────────
    //
    // RMSNorm：对最后一维做均方根归一化后乘以权重向量。
    //   x/w/out  均为 rows×dim 连续设备缓冲，dtype ∈ {F32, F64}。
    //   eps 须为有限正数。
    [[nodiscard]] auto rms_norm(ScalarType dt, const void* x, const void* w, void* out, std::size_t rows, std::size_t dim, double eps)
        -> Result<void>;

    // RoPE（split-half）：对 [n_batch, seq_len, dim] 布局的 token 序列施加旋转位置编码。
    //   dim 须为正偶数；base 须为有限正数；dtype ∈ {F32, F64}。
    [[nodiscard]] auto rope(
        ScalarType   dt,
        const void*  x,
        void*        out,
        std::size_t  n_batch,
        std::size_t  seq_len,
        std::size_t  dim,
        double       base,
        std::int64_t position_offset
    ) -> Result<void>;

    // Embedding：按 ids 从 weight 查表，写入 out[n_ids, hidden]。
    //   weight_dt ∈ {F32, F64}；ids_dt ∈ {I32, I64}；越界 id 返回错误。
    [[nodiscard]] auto embedding(
        ScalarType  weight_dt,
        ScalarType  ids_dt,
        const void* weight,
        const void* ids,
        void*       out,
        std::size_t n_ids,
        std::size_t hidden,
        std::size_t vocab
    ) -> Result<void>;

    // ── Matmul 后端切换 ─────────────────────────────────────────────────────────
    //
    // Auto：按当前构建与当前设备能力自动选择可用实现。
    // Wmma：通用 WMMA / baseline 路径，sm_70 及以上可用。
    // Cutlass：依赖 CUTLASS 且要求当前设备具备对应矩阵乘能力。
    // Native：当前指 Blackwell 专用的 TMA + WMMA F32 路径。
    enum class MatmulBackend : uint8_t
    {
        Auto    = 0,
        Wmma    = 1,
        Cutlass = 2,
        Native  = 3,
    };

    // 设置全局默认 matmul 后端并返回旧值。该设置保存在进程级 atomic 中，
    // 会影响后续未显式指定后端的 matmul 调用。
    auto set_matmul_backend(MatmulBackend backend) noexcept -> MatmulBackend;

    // 查询当前全局默认 matmul 后端。
    [[nodiscard]] auto get_matmul_backend() noexcept -> MatmulBackend;

    // 查询后端在“当前构建 + 当前设备”组合下是否可用。
    // 返回 false 代表当前构建未包含所需实现，或当前设备不具备该后端要求的硬件能力。
    [[nodiscard]] auto matmul_backend_available(MatmulBackend backend) noexcept -> bool;

} // namespace ops

// ── 异步运行时 API ──────────────────────────────────────────────────────────

// 将调用方传入的原生 cudaStream_t 句柄作为不拥有的 stream 视图使用。
// handle 必须来自同一进程中的 CUDA runtime；本接口不接管其生命周期。
[[nodiscard]] auto stream_from_handle(void* handle) -> Result<void*>;

// 异步内存拷贝。函数只负责把拷贝提交到用户提供的 stream；调用方负责
// 通过 event、stream 同步或更高层执行上下文管理可见性。
[[nodiscard]] auto memcpy_h2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>;
[[nodiscard]] auto memcpy_d2h_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>;
[[nodiscard]] auto memcpy_d2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>;

// 事件 API。
[[nodiscard]] auto create_event() -> Result<void*>;
[[nodiscard]] auto record_event(void* event_handle, void* stream) -> Result<void>;
[[nodiscard]] auto wait_event(void* event_handle, void* stream) -> Result<void>;

// workspace 管理。
// runtime-owned：返回的指针由运行时持有，调用方无需也不应 free。
// 容量足够时复用已有块（返回相同指针）；触发扩容时旧指针失效，
// 调用方不得跨增长边界缓存旧 workspace 指针。
[[nodiscard]] auto request_workspace(std::size_t nbytes, void* stream) -> Result<void*>;

} // namespace bee::cuda
