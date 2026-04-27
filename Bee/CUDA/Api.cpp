/**
 * @File Api.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief Bee::CUDA 组件对外 API 的 host 端实现。
 *
 * 本 TU 由 MSVC 以 C++23 编译，可自由使用 Result<T>/std::format。
 * 所有 CUDA runtime 调用通过 detail::runtime_* 经由 Api.cu 转发。
 */

#include "CUDA/Api.hpp"
#include "CUDA/Runtime.hpp"
#include "CUDA/Core/Check.hpp"
#include "CUDA/Core/Stream.hpp"
#include "CUDA/Mem/MemoryPool.hpp"
#include "CUDA/Ops/OpsBridge.hpp"

#include <cuda_runtime.h>

#include <array>
#include <atomic>
#include <cmath>
#include <format>
#include <mutex>

namespace bee::cuda
{

// ── 组件元信息 ──────────────────────────────────────────────────────────────

auto component_name() noexcept -> std::string_view
{
    return "CUDA";
}

auto toolkit_version() noexcept -> std::string_view
{
    return BEE_CUDA_VERSION_STRING;
}

auto has_cutlass() noexcept -> bool
{
    return BEE_HAS_CUTLASS != 0;
}

auto compiled_with_nvcc() noexcept -> bool
{
    return detail::runtime_compiled_with_nvcc();
}

// ── 设备查询 ────────────────────────────────────────────────────────────────

auto device_count() noexcept -> int
{
    int n = 0;
    (void)detail::runtime_get_device_count(&n);
    return n;
}

namespace
{

    [[nodiscard]] auto cuda_err_to_error(int err, std::string_view op) -> Error
    {
        return make_error(
            std::format("{} 失败: {} ({})", op, detail::runtime_error_name(err), detail::runtime_error_string(err)), Severity::Recoverable, err
        );
    }

    [[nodiscard]] auto current_device_arch_code() noexcept -> int
    {
        int count = 0;
        if (detail::runtime_get_device_count(&count) != 0 || count <= 0)
            return 0;

        int dev = 0;
        if (cudaGetDevice(&dev) != cudaSuccess) {
            (void)cudaGetLastError();
            return 0;
        }

        cudaDeviceProp prop{};
        if (cudaGetDeviceProperties(&prop, dev) != cudaSuccess) {
            (void)cudaGetLastError();
            return 0;
        }

        return prop.major * 100 + prop.minor * 10;
    }

    [[nodiscard]] auto current_device_has_wmma() noexcept -> bool
    {
        return current_device_arch_code() >= 700;
    }

    [[nodiscard]] auto current_device_has_native_tma_wmma() noexcept -> bool
    {
        return current_device_arch_code() >= 1200;
    }

} // namespace

auto set_device(int device_index) -> Result<void>
{
    const int err = detail::runtime_set_device(device_index);
    if (err != 0)
        return std::unexpected(cuda_err_to_error(err, std::format("cudaSetDevice({})", device_index)));
    return {};
}

auto device_synchronize() -> Result<void>
{
    const int err = detail::runtime_device_synchronize();
    if (err != 0)
        return std::unexpected(cuda_err_to_error(err, "cudaDeviceSynchronize"));
    return {};
}

// ── 内存通路（M2 实装） ─────────────────────────────────────────────────────

auto allocate(std::size_t nbytes, std::size_t /*alignment*/) -> Result<void*>
{
    if (nbytes == 0)
        return static_cast<void*>(nullptr);

    int dev = 0;
    BEE_CUDA_CHECK(cudaGetDevice(&dev));

    MemoryPool* pool = nullptr;
    BEE_TRY_ASSIGN(pool, MemoryPool::get_default(dev));

    const auto stream = StreamView::per_thread();
    void*      ptr    = nullptr;
    BEE_TRY_ASSIGN(ptr, pool->allocate_async(nbytes, stream));
    // 分配后同步：返回的指针立即可用（plan-cuda §5 同步 API 语义）。
    BEE_CUDA_CHECK(cudaStreamSynchronize(stream.native_handle()));
    return ptr;
}

void deallocate(void* ptr, std::size_t /*nbytes*/, std::size_t /*alignment*/) noexcept
{
    if (!ptr)
        return;
    int dev = 0;
    if (cudaGetDevice(&dev) != cudaSuccess) {
        (void)cudaGetLastError();
        return;
    }
    auto pool_r = MemoryPool::get_default(dev);
    if (!pool_r)
        return;
    const auto stream = StreamView::per_thread();
    pool_r.value()->deallocate_async(ptr, stream);
    // 同步，确保 free 生效后再继续（与 allocate 的同步语义对称）。
    (void)cudaStreamSynchronize(stream.native_handle());
}

auto allocate_pinned_host(std::size_t nbytes) -> Result<void*>
{
    if (nbytes == 0)
        return static_cast<void*>(nullptr);

    void* ptr = nullptr;
    BEE_CUDA_CHECK(cudaMallocHost(&ptr, nbytes));
    return ptr;
}

auto free_pinned_host(void* ptr) noexcept -> void
{
    if (!ptr)
        return;
    (void)cudaFreeHost(ptr);
}

namespace
{

    [[nodiscard]] auto sync_memcpy(void* dst, const void* src, std::size_t nbytes, cudaMemcpyKind kind) -> Result<void>
    {
        if (nbytes == 0)
            return {};
        const auto stream = StreamView::per_thread();
        BEE_CUDA_CHECK(cudaMemcpyAsync(dst, src, nbytes, kind, stream.native_handle()));
        BEE_CUDA_CHECK(cudaStreamSynchronize(stream.native_handle()));
        return {};
    }

} // namespace

auto memcpy_h2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return sync_memcpy(dst, src, nbytes, cudaMemcpyHostToDevice);
}

auto memcpy_d2h(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return sync_memcpy(dst, src, nbytes, cudaMemcpyDeviceToHost);
}

auto memcpy_d2d(void* dst, const void* src, std::size_t nbytes) -> Result<void>
{
    return sync_memcpy(dst, src, nbytes, cudaMemcpyDeviceToDevice);
}

auto memset(void* ptr, int value, std::size_t nbytes) -> Result<void>
{
    if (nbytes == 0)
        return {};
    const auto stream = StreamView::per_thread();
    BEE_CUDA_CHECK(cudaMemsetAsync(ptr, value, nbytes, stream.native_handle()));
    BEE_CUDA_CHECK(cudaStreamSynchronize(stream.native_handle()));
    return {};
}

// ── 元素级算子（M3 实装） ──────────────────────────────────────────────────

namespace ops
{

    namespace
    {

        [[nodiscard]] auto wrap(int err, std::string_view op) -> Result<void>
        {
            if (err == 0)
                return {};
            return std::unexpected(cuda_err_to_error(err, op));
        }

    } // namespace

    auto binary(BinaryOp op, ScalarType dt, const void* a, const void* b, void* out, std::size_t n) -> Result<void>
    {
        if (n == 0)
            return {};
        const int err = detail::ops_binary(static_cast<int>(op), static_cast<int>(dt), a, b, out, n);
        return wrap(err, "cuda::ops::binary");
    }

    auto unary(UnaryOp op, ScalarType dt, const void* src, void* dst, std::size_t n) -> Result<void>
    {
        if (n == 0)
            return {};
        const int err = detail::ops_unary(static_cast<int>(op), static_cast<int>(dt), src, dst, n);
        return wrap(err, "cuda::ops::unary");
    }

    auto cast(ScalarType src_dt, const void* src, ScalarType dst_dt, void* dst, std::size_t n) -> Result<void>
    {
        if (n == 0)
            return {};
        const int err = detail::ops_cast(static_cast<int>(src_dt), src, static_cast<int>(dst_dt), dst, n);
        return wrap(err, "cuda::ops::cast");
    }

    auto reduce_global(ReduceOp op, ScalarType dt, const void* src, void* dst, std::size_t n) -> Result<void>
    {
        if (n == 0)
            return std::unexpected(make_error("cuda::ops::reduce_global: n == 0", Severity::Recoverable));
        const int err = detail::ops_reduce_global(static_cast<int>(op), static_cast<int>(dt), src, dst, n);
        return wrap(err, "cuda::ops::reduce_global");
    }

    auto reduce_axis(ReduceOp op, ScalarType dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) -> Result<void>
    {
        if (outer == 0 || inner == 0 || axis == 0)
            return std::unexpected(make_error("cuda::ops::reduce_axis: 维度含 0", Severity::Recoverable));
        const int err = detail::ops_reduce_axis(static_cast<int>(op), static_cast<int>(dt), src, dst, outer, axis, inner);
        return wrap(err, "cuda::ops::reduce_axis");
    }

    auto softmax(ScalarType dt, const void* src, void* dst, std::size_t outer, std::size_t axis, std::size_t inner) -> Result<void>
    {
        if (outer == 0 || inner == 0 || axis == 0)
            return std::unexpected(make_error("cuda::ops::softmax: 维度含 0", Severity::Recoverable));
        const int err = detail::ops_softmax(static_cast<int>(dt), src, dst, outer, axis, inner);
        return wrap(err, "cuda::ops::softmax");
    }

    auto scale_fp(ScalarType dt, void* buf, double factor, std::size_t n) -> Result<void>
    {
        if (n == 0)
            return {};
        const int err = detail::ops_scale_fp(static_cast<int>(dt), buf, factor, n);
        return wrap(err, "cuda::ops::scale_fp");
    }

    auto matmul(ScalarType dt, const void* A, const void* B, void* C, std::size_t M, std::size_t K, std::size_t N) -> Result<void>
    {
        if (M == 0 || N == 0)
            return {};

        // 按当前全局后端设置分派：
        //  - Cutlass：未独立暴露（Auto/Wmma 在 detail::ops_matmul 内部已优先选择 CUTLASS）
        //  - Native：B10 手写 TMA + WMMA TF32 路径（要求严格对齐，否则返回错误）
        //  - Auto/Wmma：内部启发式（CUTLASS 或 native tile）
        const auto backend = get_matmul_backend();
        switch (backend) {
        case MatmulBackend::Cutlass:
            return std::unexpected(make_error("cuda::ops::matmul: Cutlass 后端尚未暴露（Auto 已自动启用）", Severity::Recoverable));
        case MatmulBackend::Native: {
            if (!current_device_has_native_tma_wmma())
                return std::unexpected(make_error(
                    "cuda::ops::matmul[Native:TMA+WMMA]: 当前设备不支持该后端", Severity::Recoverable, static_cast<int>(cudaErrorNotSupported)
                ));
            const int err = detail::ops_matmul_force_tma_wmma(static_cast<int>(dt), A, B, C, M, K, N);
            return wrap(err, "cuda::ops::matmul[Native:TMA+WMMA]");
        }
        case MatmulBackend::Auto:
        case MatmulBackend::Wmma:
        default: break;
        }

        const int err = detail::ops_matmul(static_cast<int>(dt), A, B, C, M, K, N);
        return wrap(err, "cuda::ops::matmul");
    }

    auto matmul_lowp(ScalarType dt, const void* A, const void* B, float* C, std::size_t M, std::size_t K, std::size_t N) -> Result<void>
    {
        // 仅接受 F16 或 BF16
        if (dt != ScalarType::F16 && dt != ScalarType::BF16)
            return std::unexpected(make_error(
                std::format("cuda::ops::matmul_lowp: 不支持的 dtype={}，仅接受 F16/BF16", static_cast<int>(dt)),
                Severity::Recoverable
            ));

        // M==0 或 N==0：无输出，直接成功
        if (M == 0 || N == 0)
            return {};

        // K==0：输出全零由调用方保证，API 层直接成功
        if (K == 0)
            return {};

        // 有效尺寸下指针不得为空
        if (!A || !B || !C)
            return std::unexpected(make_error("cuda::ops::matmul_lowp: 空指针（A/B/C 均须有效）", Severity::Recoverable));

        const int err = detail::ops_matmul_lowp(static_cast<int>(dt), A, B, C, M, K, N);
        return wrap(err, "cuda::ops::matmul_lowp");
    }

    auto transpose_2d(ScalarType dt, const void* src, void* dst, std::size_t rows, std::size_t cols) -> Result<void>
    {
        if (rows == 0 || cols == 0)
            return {};
        const int err = detail::ops_transpose_2d(static_cast<int>(dt), src, dst, rows, cols);
        return wrap(err, "cuda::ops::transpose_2d");
    }

    auto strided_copy(
        ScalarType     dt,
        const void*    src,
        void*          dst,
        const int64_t* shape,
        const int64_t* strides,
        int            ndim,
        int64_t        offset_elements,
        std::size_t    numel
    ) -> Result<void>
    {
        if (numel == 0)
            return {};
        const int err = detail::ops_strided_copy(static_cast<int>(dt), src, dst, shape, strides, ndim, offset_elements, numel);
        return wrap(err, "cuda::ops::strided_copy");
    }

    auto random_uniform(ScalarType dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>
    {
        if (n == 0)
            return {};
        const int err = detail::ops_random_uniform(static_cast<int>(dt), dst, n, seed);
        return wrap(err, "cuda::ops::random_uniform");
    }

    auto random_normal(ScalarType dt, void* dst, std::size_t n, std::uint64_t seed) -> Result<void>
    {
        if (n == 0)
            return {};
        const int err = detail::ops_random_normal(static_cast<int>(dt), dst, n, seed);
        return wrap(err, "cuda::ops::random_normal");
    }

    auto random_int(ScalarType dt, void* dst, std::size_t n, std::int64_t low, std::int64_t high, std::uint64_t seed) -> Result<void>
    {
        if (n == 0)
            return {};
        const int err = detail::ops_random_int(static_cast<int>(dt), dst, n, low, high, seed);
        return wrap(err, "cuda::ops::random_int");
    }

    auto rms_norm(ScalarType dt, const void* x, const void* w, void* out,
                  std::size_t rows, std::size_t dim, double eps) -> Result<void>
    {
        if (!x || !w || !out)
            return std::unexpected(make_error("cuda::ops::rms_norm: 空指针", Severity::Recoverable));
        if (rows == 0 || dim == 0)
            return std::unexpected(make_error("cuda::ops::rms_norm: 维度含 0", Severity::Recoverable));
        if (rows > 2147483647ull)
            return std::unexpected(make_error("cuda::ops::rms_norm: rows 超过 CUDA grid.x 上限", Severity::Recoverable));
        if (!std::isfinite(eps) || eps <= 0.0)
            return std::unexpected(make_error(std::format("cuda::ops::rms_norm: eps 须为有限正数，当前 {}", eps), Severity::Recoverable));
        const int err = detail::ops_rms_norm(static_cast<int>(dt), x, w, out, rows, dim, eps);
        return wrap(err, "cuda::ops::rms_norm");
    }

    auto rope(ScalarType dt, const void* x, void* out,
              std::size_t n_batch, std::size_t seq_len, std::size_t dim,
              double base, std::int64_t position_offset) -> Result<void>
    {
        if (!x || !out)
            return std::unexpected(make_error("cuda::ops::rope: 空指针", Severity::Recoverable));
        if (n_batch == 0 || seq_len == 0 || dim == 0)
            return std::unexpected(make_error("cuda::ops::rope: 维度含 0", Severity::Recoverable));
        if (dim % 2 != 0)
            return std::unexpected(make_error(std::format("cuda::ops::rope: dim 须为偶数，当前 {}", dim), Severity::Recoverable));
        if (!std::isfinite(base) || base <= 0.0)
            return std::unexpected(make_error(std::format("cuda::ops::rope: base 须为有限正数，当前 {}", base), Severity::Recoverable));
        const int err = detail::ops_rope(static_cast<int>(dt), x, out, n_batch, seq_len, dim, base, position_offset);
        return wrap(err, "cuda::ops::rope");
    }

    auto embedding(ScalarType weight_dt, ScalarType ids_dt,
                   const void* weight, const void* ids, void* out,
                   std::size_t n_ids, std::size_t hidden, std::size_t vocab) -> Result<void>
    {
        if (!weight || !ids || !out)
            return std::unexpected(make_error("cuda::ops::embedding: 空指针", Severity::Recoverable));
        if (n_ids == 0 || hidden == 0)
            return std::unexpected(make_error("cuda::ops::embedding: 维度含 0", Severity::Recoverable));
        if (vocab == 0)
            return std::unexpected(make_error("cuda::ops::embedding: vocab 须 > 0", Severity::Recoverable));
        const int err = detail::ops_embedding(
            static_cast<int>(weight_dt), static_cast<int>(ids_dt), weight, ids, out, n_ids, hidden, vocab
        );
        return wrap(err, "cuda::ops::embedding");
    }

    // ── Matmul 后端切换（M6/M7 脚手架） ─────────────────────────────────────────

    namespace
    {

        // std::atomic<uint8_t>：线程安全的全局后端设置。
        std::atomic<std::uint8_t>& matmul_backend_storage() noexcept
        {
            static std::atomic<std::uint8_t> g_backend{static_cast<std::uint8_t>(MatmulBackend::Auto)};
            return g_backend;
        }

    } // namespace

    auto set_matmul_backend(MatmulBackend backend) noexcept -> MatmulBackend
    {
        const auto old = matmul_backend_storage().exchange(static_cast<std::uint8_t>(backend), std::memory_order_acq_rel);
        return static_cast<MatmulBackend>(old);
    }

    auto get_matmul_backend() noexcept -> MatmulBackend
    {
        return static_cast<MatmulBackend>(matmul_backend_storage().load(std::memory_order_acquire));
    }

    auto matmul_backend_available(MatmulBackend backend) noexcept -> bool
    {
        switch (backend) {
        case MatmulBackend::Auto: return true;
        case MatmulBackend::Wmma: return current_device_has_wmma();
        case MatmulBackend::Cutlass: return has_cutlass() && current_device_has_wmma();
        case MatmulBackend::Native: return current_device_has_native_tma_wmma();
        }
        return false;
    }

} // namespace ops

// ── 异步运行时 API 实现 ─────────────────────────────────────────────────────

auto stream_from_handle(void* handle) -> Result<void*>
{
    // 骨架阶段直接传递 handle。
    return handle;
}

auto memcpy_h2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>
{
    if (nbytes == 0)
        return {};
    auto cuda_stream = stream ? reinterpret_cast<cudaStream_t>(stream) : StreamView::per_thread().native_handle();
    BEE_CUDA_CHECK(cudaMemcpyAsync(dst, src, nbytes, cudaMemcpyHostToDevice, cuda_stream));
    return {};
}

auto memcpy_d2h_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>
{
    if (nbytes == 0)
        return {};
    auto cuda_stream = stream ? reinterpret_cast<cudaStream_t>(stream) : StreamView::per_thread().native_handle();
    BEE_CUDA_CHECK(cudaMemcpyAsync(dst, src, nbytes, cudaMemcpyDeviceToHost, cuda_stream));
    return {};
}

auto memcpy_d2d_async(void* dst, const void* src, std::size_t nbytes, void* stream) -> Result<void>
{
    if (nbytes == 0)
        return {};
    auto cuda_stream = stream ? reinterpret_cast<cudaStream_t>(stream) : StreamView::per_thread().native_handle();
    BEE_CUDA_CHECK(cudaMemcpyAsync(dst, src, nbytes, cudaMemcpyDeviceToDevice, cuda_stream));
    return {};
}

auto create_event() -> Result<void*>
{
    cudaEvent_t event;
    BEE_CUDA_CHECK(cudaEventCreate(&event));
    return reinterpret_cast<void*>(event);
}

auto record_event(void* event_handle, void* stream) -> Result<void>
{
    auto event       = reinterpret_cast<cudaEvent_t>(event_handle);
    auto cuda_stream = stream ? reinterpret_cast<cudaStream_t>(stream) : StreamView::per_thread().native_handle();
    BEE_CUDA_CHECK(cudaEventRecord(event, cuda_stream));
    return {};
}

auto wait_event(void* event_handle, void* stream) -> Result<void>
{
    auto event       = reinterpret_cast<cudaEvent_t>(event_handle);
    auto cuda_stream = stream ? reinterpret_cast<cudaStream_t>(stream) : StreamView::per_thread().native_handle();
    BEE_CUDA_CHECK(cudaStreamWaitEvent(cuda_stream, event, 0));
    return {};
}

namespace
{
    // 最小 workspace registry：按 device 持有单一缓存块。
    // 所有权：runtime-owned，调用方不负责 free。
    // 生命周期：静态对象析构时释放；进程期间可复用。
    // 线程安全：mtx 保护所有 slot 的并发读写，防止双重 free / 丢失更新。
    struct WorkspaceRegistry
    {
        struct Slot
        {
            void*       ptr      = nullptr;
            std::size_t capacity = 0;
        };

        std::array<Slot, 16> slots{}; // 最多支持 16 个 device
        std::mutex           mtx;     // 保护 slots 并发访问

        ~WorkspaceRegistry()
        {
            // 析构在进程退出时调用，无需加锁（此时不再有并发访问）
            for (auto& slot : slots) {
                if (slot.ptr) {
                    (void)cudaFree(slot.ptr);
                    slot.ptr = nullptr;
                }
            }
        }

        [[nodiscard]] auto get_or_grow(int device, std::size_t nbytes) -> Result<void*>
        {
            if (device < 0 || device >= static_cast<int>(slots.size()))
                return std::unexpected(make_error("workspace: device index out of range", Severity::Recoverable));

            std::lock_guard lock{mtx};
            auto&           slot = slots[static_cast<std::size_t>(device)];

            // 容量足够，直接复用
            if (slot.ptr && slot.capacity >= nbytes)
                return slot.ptr;

            // 容量不足或首次分配：释放旧缓存，重新分配更大块
            // 注意：旧指针在此处失效，调用方不得跨增长边界缓存旧 workspace 指针
            if (slot.ptr) {
                (void)cudaFree(slot.ptr);
                slot.ptr      = nullptr;
                slot.capacity = 0;
            }

            void* new_ptr = nullptr;
            BEE_CUDA_CHECK(cudaMalloc(&new_ptr, nbytes));
            slot.ptr      = new_ptr;
            slot.capacity = nbytes;
            return new_ptr;
        }
    };

    WorkspaceRegistry& get_workspace_registry() noexcept
    {
        static WorkspaceRegistry reg;
        return reg;
    }

} // namespace

auto request_workspace(std::size_t nbytes, void* stream) -> Result<void*>
{
    // runtime-owned workspace：返回的指针由 runtime 持有，调用方无需也不应 free。
    // 生命周期契约：
    // - 同一 device 上容量足够时，直接复用已有块，返回相同指针。
    // - 当后续请求的 nbytes 超过已有容量时，runtime 会释放旧块并重新分配更大块；
    //   旧指针随即失效，调用方不得跨增长边界缓存旧 workspace 指针。
    if (nbytes == 0)
        return static_cast<void*>(nullptr);

    (void)stream; // 当前暂不使用 stream 参数

    int dev = 0;
    BEE_CUDA_CHECK(cudaGetDevice(&dev));

    return get_workspace_registry().get_or_grow(dev, nbytes);
}

} // namespace bee::cuda
