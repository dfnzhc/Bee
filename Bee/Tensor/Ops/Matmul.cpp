#include "Tensor/Ops/Matmul.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cuda/Backend.hpp"
#include "Tensor/Core/DType.hpp"

#include <cstring>
#include <format>

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

} // namespace

auto matmul(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    // ── 基本有效性检查 ────────────────────────────────────────────────────────
    if (!a.defined() || !b.defined())
        return std::unexpected(make_error("matmul: 输入 Tensor 未定义", Severity::Recoverable));

    // ── device 检查 ───────────────────────────────────────────────────────────
    if (a.device() != b.device())
        return std::unexpected(make_error(
            std::format(
                "matmul: 两操作数 device 不同（{} vs {}）", a.device() == Device::CPU ? "CPU" : "CUDA", b.device() == Device::CPU ? "CPU" : "CUDA"
            ),
            Severity::Recoverable
        ));

    if (a.device() == Device::CUDA) {
        // CUDA 路径：前置校验完成后转发给 Bee::CUDA 的 matmul 后端选择层。
        // 当前默认会在 CUTLASS / baseline tile / Native(TMA+WMMA) 之间选择或回退。
        if (a.dtype() != b.dtype())
            return std::unexpected(
                make_error(std::format("matmul: dtype 不匹配（{} vs {}）", enum_to_name(a.dtype()), enum_to_name(b.dtype())), Severity::Recoverable)
            );

        const DType dt_cu = a.dtype();
        if (dt_cu == DType::Bool || dt_cu == DType::U8)
            return std::unexpected(make_error(std::format("matmul: CUDA 不支持 DType::{}", enum_to_name(dt_cu)), Severity::Recoverable));

        if (a.ndim() != 2 || b.ndim() != 2)
            return std::unexpected(make_error("matmul: 仅支持 2D × 2D", Severity::Recoverable));

        const int64_t M  = a.shape()[0];
        const int64_t Ka = a.shape()[1];
        const int64_t Kb = b.shape()[0];
        const int64_t N  = b.shape()[1];
        if (Ka != Kb)
            return std::unexpected(make_error(std::format("matmul: 内维不匹配（a 列={}, b 行={}）", Ka, Kb), Severity::Recoverable));

        auto out_r = Tensor::zeros({M, N}, dt_cu, Device::CUDA);
        if (!out_r)
            return std::unexpected(std::move(out_r.error()));
        if (M == 0 || N == 0 || Ka == 0)
            return *out_r;

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

        auto rc = tensor::cuda::matmul(
            static_cast<int>(dt_cu),
            ca.data_ptr(),
            cb.data_ptr(),
            out_r->data_ptr(),
            static_cast<std::size_t>(M),
            static_cast<std::size_t>(Ka),
            static_cast<std::size_t>(N)
        );
        if (!rc)
            return std::unexpected(std::move(rc.error()));
        return *out_r;
    }

    // ── dtype 检查 ────────────────────────────────────────────────────────────
    if (a.dtype() != b.dtype())
        return std::unexpected(
            make_error(std::format("matmul: dtype 不匹配（{} vs {}）", enum_to_name(a.dtype()), enum_to_name(b.dtype())), Severity::Recoverable)
        );

    const DType dt = a.dtype();
    if (dt == DType::Bool || dt == DType::U8)
        return std::unexpected(make_error(std::format("matmul: 不支持 DType::{}", enum_to_name(dt)), Severity::Recoverable));

    // I8 输入 → I32 输出（累加到更宽类型避免溢出）
    const DType out_dt = (dt == DType::I8) ? DType::I32 : dt;

    // ── 维度检查：仅支持 2D × 2D ─────────────────────────────────────────────
    if (a.ndim() != 2)
        return std::unexpected(make_error(std::format("matmul: a 必须是 2D 张量，当前 ndim={}", a.ndim()), Severity::Recoverable));

    if (b.ndim() != 2)
        return std::unexpected(make_error(std::format("matmul: b 必须是 2D 张量，当前 ndim={}", b.ndim()), Severity::Recoverable));

    // ── shape 相容性检查 ──────────────────────────────────────────────────────
    const int64_t M  = a.shape()[0];
    const int64_t Ka = a.shape()[1];
    const int64_t Kb = b.shape()[0];
    const int64_t N  = b.shape()[1];

    if (Ka != Kb)
        return std::unexpected(make_error(std::format("matmul: 内维不匹配（a 列={}, b 行={}）", Ka, Kb), Severity::Recoverable));

    const int64_t K = Ka;

    // ── M 或 N 为 0：直接返回形状正确的零张量 ────────────────────────────────
    if (M == 0 || N == 0) {
        auto out = Tensor::zeros({M, N}, out_dt);
        if (!out)
            return std::unexpected(std::move(out.error()));
        return *out;
    }

    // ── 输出张量（全零，contiguous）──────────────────────────────────────────
    auto out = Tensor::zeros({M, N}, out_dt);
    if (!out)
        return std::unexpected(std::move(out.error()));

    // K==0：输出已经是全 0，直接返回
    if (K == 0)
        return *out;

    // ── 非 contiguous 输入自动整理 ────────────────────────────────────────────
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

    // ── 调用 CPU 内核 ─────────────────────────────────────────────────────────
    // 这里继续下沉到运行期 ISA 分派；F32/F64/I32 走 GEMM driver，I64 走模板核，
    // I8 则提升到 I32 输出。
    dispatch_matmul_cpu(M, K, N, dt, out_dt, ca.data_ptr(), cb.data_ptr(), out->data_ptr());

    return *out;
}

} // namespace bee
