#include "Tensor/Ops/Matmul.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cuda/Backend.hpp"
#include "Tensor/Core/DType.hpp"
#include "Tensor/Ops/Broadcast.hpp"

#include <cstring>
#include <format>
#include <vector>

namespace bee
{

namespace
{

    // 分派到对应类型的 CPU 内核（运行期 ISA 分派）
    // in_dtype：输入 A/B 的 dtype；out_dtype：输出 C 的 dtype（通常同 in_dtype，
    // 但 I8×I8→I32 时不同）
    auto dispatch_matmul_cpu(int64_t M, int64_t K, int64_t N, DType in_dtype, DType out_dtype, const void* A, const void* B, void* C) -> void
    {
        // 输出缓冲区按 out_dtype 大小置零
        const std::size_t nbytes = static_cast<std::size_t>(M * N) * dtype_size(out_dtype);
        std::memset(C, 0, nbytes);

        switch (in_dtype) {
        case DType::F32: BEE_RT_DISPATCH(mm_f32, M, K, N, static_cast<const float*>(A), static_cast<const float*>(B), static_cast<float*>(C));
        case DType::F64: BEE_RT_DISPATCH(mm_f64, M, K, N, static_cast<const double*>(A), static_cast<const double*>(B), static_cast<double*>(C));
        case DType::I32: BEE_RT_DISPATCH(mm_i32, M, K, N, static_cast<const int32_t*>(A), static_cast<const int32_t*>(B), static_cast<int32_t*>(C));
        case DType::I64: BEE_RT_DISPATCH(mm_i64, M, K, N, static_cast<const int64_t*>(A), static_cast<const int64_t*>(B), static_cast<int64_t*>(C));
        case DType::I8: BEE_RT_DISPATCH(mm_i8, M, K, N, static_cast<const int8_t*>(A), static_cast<const int8_t*>(B), static_cast<int32_t*>(C));
        default: break;
        }
    }

    // 推导 batched matmul 输出形状
    // a: [..., M, K]，b: [..., K, N] → [..., M, N]，batch 维按广播规则合并
    auto infer_batched_matmul_shape(const Shape& a, const Shape& b) -> Result<Shape>
    {
        if (a.size() < 2)
            return std::unexpected(make_error(std::format("matmul: a 维度必须 >= 2，当前 ndim={}", a.size()), Severity::Recoverable));
        if (b.size() < 2)
            return std::unexpected(make_error(std::format("matmul: b 维度必须 >= 2，当前 ndim={}", b.size()), Severity::Recoverable));

        const int64_t Ka = a[a.size() - 1];
        const int64_t Kb = b[b.size() - 2];
        if (Ka != Kb)
            return std::unexpected(make_error(std::format("matmul: 内维不匹配（a 列={}, b 行={}）", Ka, Kb), Severity::Recoverable));

        // 分别提取 batch 形状（去掉最后两个矩阵维度）
        const Shape a_batch(a.begin(), a.end() - 2);
        const Shape b_batch(b.begin(), b.end() - 2);

        auto batch_r = compute_broadcast_shape(a_batch, b_batch);
        if (!batch_r)
            return std::unexpected(std::move(batch_r.error()));

        Shape out = *batch_r;
        out.push_back(a[a.size() - 2]); // M
        out.push_back(b[b.size() - 1]); // N
        return out;
    }

    // 提取 contiguous 后的 2D slice（沿 dim=0 连续取一项并 squeeze）
    auto extract_2d_slice(const Tensor& t, const std::vector<int64_t>& multi_idx, int64_t batch_ndim) -> Result<Tensor>
    {
        Tensor cur = t;
        // 将 t 填充到拥有 batch_ndim 个批次维（在最高维插入 1）
        const int64_t t_batch_ndim = cur.ndim() - 2;
        for (int64_t i = t_batch_ndim; i < batch_ndim; ++i) {
            auto r = cur.unsqueeze(0);
            if (!r)
                return std::unexpected(std::move(r.error()));
            cur = *r;
        }
        // 逐个批次维取切片
        for (int64_t d = 0; d < batch_ndim; ++d) {
            const int64_t size = cur.shape()[0]; // 始终处理 dim=0
            const int64_t idx  = (size == 1) ? 0 : multi_idx[static_cast<std::size_t>(d)];
            auto r1 = cur.slice(0, idx, idx + 1);
            if (!r1)
                return std::unexpected(std::move(r1.error()));
            auto r2 = r1->squeeze(0);
            if (!r2)
                return std::unexpected(std::move(r2.error()));
            cur = *r2;
        }
        return cur;
    }

    // CPU 批次 matmul（逐批次调用 2D 内核）
    auto matmul_batched_cpu(const Tensor& a, const Tensor& b, const Shape& out_shape, DType dt, DType out_dt) -> Result<Tensor>
    {
        const int64_t M = out_shape[out_shape.size() - 2];
        const int64_t N = out_shape[out_shape.size() - 1];
        const int64_t K = a.shape()[a.shape().size() - 1];

        const int64_t batch_ndim = static_cast<int64_t>(out_shape.size()) - 2;
        int64_t       total_batch = 1;
        for (int64_t d = 0; d < batch_ndim; ++d)
            total_batch *= out_shape[static_cast<std::size_t>(d)];

        auto out = Tensor::zeros(out_shape, out_dt);
        if (!out)
            return std::unexpected(std::move(out.error()));
        if (M == 0 || N == 0 || total_batch == 0)
            return *out;
        if (K == 0)
            return *out;

        auto*             out_data   = static_cast<char*>(out->data_ptr());
        const std::size_t elem_size  = dtype_size(out_dt);
        const std::size_t slice_bytes = static_cast<std::size_t>(M * N) * elem_size;

        std::vector<int64_t> multi_idx(static_cast<std::size_t>(batch_ndim));

        for (int64_t flat = 0; flat < total_batch; ++flat) {
            // 将线性批次索引转换为多维索引
            int64_t rem = flat;
            for (int64_t d = batch_ndim - 1; d >= 0; --d) {
                multi_idx[static_cast<std::size_t>(d)] = rem % out_shape[static_cast<std::size_t>(d)];
                rem /= out_shape[static_cast<std::size_t>(d)];
            }

            // 提取 2D 切片
            auto a_slice_r = extract_2d_slice(a, multi_idx, batch_ndim);
            if (!a_slice_r)
                return std::unexpected(std::move(a_slice_r.error()));
            auto b_slice_r = extract_2d_slice(b, multi_idx, batch_ndim);
            if (!b_slice_r)
                return std::unexpected(std::move(b_slice_r.error()));

            // 确保连续
            Tensor ca = *a_slice_r;
            if (!ca.is_contiguous()) {
                auto r = ca.contiguous();
                if (!r)
                    return std::unexpected(std::move(r.error()));
                ca = *r;
            }
            Tensor cb = *b_slice_r;
            if (!cb.is_contiguous()) {
                auto r = cb.contiguous();
                if (!r)
                    return std::unexpected(std::move(r.error()));
                cb = *r;
            }

            // 直接写入输出缓冲区的对应偏移位置
            dispatch_matmul_cpu(M, K, N, dt, out_dt, ca.data_ptr(), cb.data_ptr(), out_data + static_cast<std::size_t>(flat) * slice_bytes);
        }

        return *out;
    }

    // 计算 contiguous 输入中某一批次切片对应的平坦偏移（以元素为单位）
    // 形参 batch_ndim：输出批次维数
    // 形参 tensor_shape：含矩阵维的完整形状（ndim >= 2）
    auto compute_batch_flat(const Shape& tensor_shape, const std::vector<int64_t>& multi_idx, int64_t batch_ndim) -> std::size_t
    {
        const int64_t t_batch_ndim = static_cast<int64_t>(tensor_shape.size()) - 2;
        // 右对齐：tensor batch 维 j 对应输出 batch 维 (batch_ndim - t_batch_ndim + j)
        std::size_t flat   = 0;
        std::size_t stride = 1;
        // 从右往左遍历 tensor 的批次维
        for (int64_t j = t_batch_ndim - 1; j >= 0; --j) {
            const int64_t out_d   = batch_ndim - t_batch_ndim + j;
            const int64_t t_size  = tensor_shape[static_cast<std::size_t>(j)];
            const int64_t idx     = (t_size == 1) ? 0 : multi_idx[static_cast<std::size_t>(out_d)];
            flat   += static_cast<std::size_t>(idx) * stride;
            stride *= static_cast<std::size_t>(t_size);
        }
        return flat;
    }

    // CUDA 批次 matmul（逐批次以指针偏移调用底层 2D kernel；输入已 contiguous）
    auto matmul_batched_cuda(const Tensor& a_cont, const Tensor& b_cont, const Shape& out_shape, DType dt) -> Result<Tensor>
    {
        const int64_t M = out_shape[out_shape.size() - 2];
        const int64_t N = out_shape[out_shape.size() - 1];
        const int64_t K = a_cont.shape()[a_cont.shape().size() - 1];

        const int64_t batch_ndim = static_cast<int64_t>(out_shape.size()) - 2;
        int64_t       total_batch = 1;
        for (int64_t d = 0; d < batch_ndim; ++d)
            total_batch *= out_shape[static_cast<std::size_t>(d)];

        auto out = Tensor::zeros(out_shape, dt, Device::CUDA);
        if (!out)
            return std::unexpected(std::move(out.error()));
        if (M == 0 || N == 0 || total_batch == 0 || K == 0)
            return *out;

        const std::size_t elem_size    = dtype_size(dt);
        const std::size_t a_slice_elems = static_cast<std::size_t>(M * K);
        const std::size_t b_slice_elems = static_cast<std::size_t>(K * N);
        const std::size_t o_slice_elems = static_cast<std::size_t>(M * N);

        const auto* a_ptr  = static_cast<const char*>(a_cont.data_ptr());
        const auto* b_ptr  = static_cast<const char*>(b_cont.data_ptr());
        auto*       out_ptr = static_cast<char*>(out->data_ptr());

        std::vector<int64_t> multi_idx(static_cast<std::size_t>(batch_ndim));

        for (int64_t flat = 0; flat < total_batch; ++flat) {
            // 将线性批次索引转换为多维索引
            int64_t rem = flat;
            for (int64_t d = batch_ndim - 1; d >= 0; --d) {
                multi_idx[static_cast<std::size_t>(d)] = rem % out_shape[static_cast<std::size_t>(d)];
                rem /= out_shape[static_cast<std::size_t>(d)];
            }

            const std::size_t a_flat = compute_batch_flat(a_cont.shape(), multi_idx, batch_ndim);
            const std::size_t b_flat = compute_batch_flat(b_cont.shape(), multi_idx, batch_ndim);

            auto rc = tensor::cuda::matmul(
                static_cast<int>(dt),
                a_ptr + a_flat * a_slice_elems * elem_size,
                b_ptr + b_flat * b_slice_elems * elem_size,
                out_ptr + static_cast<std::size_t>(flat) * o_slice_elems * elem_size,
                static_cast<std::size_t>(M),
                static_cast<std::size_t>(K),
                static_cast<std::size_t>(N)
            );
            if (!rc)
                return std::unexpected(std::move(rc.error()));
        }

        return *out;
    }

} // namespace

auto matmul(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    // ── 基本有效性检查 ────────────────────────────────────────────────────────
    if (!a.defined() || !b.defined())
        return std::unexpected(make_error("matmul: 输入 Tensor 未定义", Severity::Recoverable));

    // ── 维度检查：要求 ndim >= 2 ──────────────────────────────────────────────
    if (a.ndim() < 2)
        return std::unexpected(make_error(std::format("matmul: a 维度必须 >= 2，当前 ndim={}", a.ndim()), Severity::Recoverable));
    if (b.ndim() < 2)
        return std::unexpected(make_error(std::format("matmul: b 维度必须 >= 2，当前 ndim={}", b.ndim()), Severity::Recoverable));

    // ── device 检查 ───────────────────────────────────────────────────────────
    if (a.device() != b.device())
        return std::unexpected(make_error(
            std::format(
                "matmul: 两操作数 device 不同（{} vs {}）", a.device() == Device::CPU ? "CPU" : "CUDA", b.device() == Device::CPU ? "CPU" : "CUDA"
            ),
            Severity::Recoverable
        ));

    // ── dtype 检查 ────────────────────────────────────────────────────────────
    if (a.dtype() != b.dtype())
        return std::unexpected(
            make_error(std::format("matmul: dtype 不匹配（{} vs {}）", enum_to_name(a.dtype()), enum_to_name(b.dtype())), Severity::Recoverable)
        );

    const DType dt = a.dtype();
    if (dt == DType::Bool || dt == DType::U8)
        return std::unexpected(make_error(std::format("matmul: 不支持 DType::{}", enum_to_name(dt)), Severity::Recoverable));

    // ── 推导输出形状（含内维匹配校验）────────────────────────────────────────
    auto out_shape_r = infer_batched_matmul_shape(a.shape(), b.shape());
    if (!out_shape_r)
        return std::unexpected(std::move(out_shape_r.error()));
    const Shape& out_shape = *out_shape_r;

    const int64_t M = out_shape[out_shape.size() - 2];
    const int64_t K = a.shape()[static_cast<std::size_t>(a.ndim() - 1)];
    const int64_t N = out_shape[out_shape.size() - 1];

    // I8 输入 → I32 输出（累加到更宽类型避免溢出）
    const DType out_dt = (dt == DType::I8) ? DType::I32 : dt;

    const bool is_batched = (out_shape.size() > 2);

    if (a.device() == Device::CUDA) {
        // ── CUDA 路径 ──────────────────────────────────────────────────────────
        if (dt == DType::Bool || dt == DType::U8)
            return std::unexpected(make_error(std::format("matmul: CUDA 不支持 DType::{}", enum_to_name(dt)), Severity::Recoverable));

        // M 或 N 为 0：直接返回形状正确的零张量
        if (M == 0 || N == 0) {
            auto out = Tensor::zeros(out_shape, dt, Device::CUDA);
            if (!out)
                return std::unexpected(std::move(out.error()));
            return *out;
        }

        // 确保连续
        Tensor ca = a;
        if (!a.is_contiguous()) {
            auto r = a.contiguous();
            if (!r)
                return std::unexpected(std::move(r.error()));
            ca = *r;
        }
        Tensor cb = b;
        if (!b.is_contiguous()) {
            auto r = b.contiguous();
            if (!r)
                return std::unexpected(std::move(r.error()));
            cb = *r;
        }

        if (!is_batched) {
            // 2D 路径
            auto out_r = Tensor::zeros(out_shape, dt, Device::CUDA);
            if (!out_r)
                return std::unexpected(std::move(out_r.error()));
            if (K == 0)
                return *out_r;
            auto rc = tensor::cuda::matmul(
                static_cast<int>(dt),
                ca.data_ptr(),
                cb.data_ptr(),
                out_r->data_ptr(),
                static_cast<std::size_t>(M),
                static_cast<std::size_t>(K),
                static_cast<std::size_t>(N)
            );
            if (!rc)
                return std::unexpected(std::move(rc.error()));
            return *out_r;
        }

        // 批次 CUDA 路径（逐批次调用底层 2D kernel）
        return matmul_batched_cuda(ca, cb, out_shape, dt);
    }

    // ── CPU 路径 ───────────────────────────────────────────────────────────────

    // M 或 N 为 0：直接返回形状正确的零张量
    if (M == 0 || N == 0) {
        auto out = Tensor::zeros(out_shape, out_dt);
        if (!out)
            return std::unexpected(std::move(out.error()));
        return *out;
    }

    if (is_batched) {
        // 批次 CPU 路径
        return matmul_batched_cpu(a, b, out_shape, dt, out_dt);
    }

    // ── 2D CPU 路径 ────────────────────────────────────────────────────────────
    auto out = Tensor::zeros(out_shape, out_dt);
    if (!out)
        return std::unexpected(std::move(out.error()));

    if (K == 0)
        return *out;

    Tensor ca = a;
    if (!a.is_contiguous()) {
        auto r = a.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        ca = *r;
    }
    Tensor cb = b;
    if (!b.is_contiguous()) {
        auto r = b.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        cb = *r;
    }

    dispatch_matmul_cpu(M, K, N, dt, out_dt, ca.data_ptr(), cb.data_ptr(), out->data_ptr());
    return *out;
}

} // namespace bee
