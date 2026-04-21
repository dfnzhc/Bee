#include "Tensor/Ops/Matmul.hpp"
#include "Tensor/Cpu/MatmulCpu.hpp"

#include <cstring>
#include <format>

namespace bee
{

namespace
{

// 分派到对应类型的 CPU 内核
auto dispatch_matmul_cpu(
    int64_t M, int64_t K, int64_t N,
    DType dtype,
    const void* A,
    const void* B,
    void*       C) -> void
{
    // 先将输出缓冲区置零
    const std::size_t nbytes = static_cast<std::size_t>(M * N) * dtype_size(dtype);
    std::memset(C, 0, nbytes);

    switch (dtype) {
    case DType::F32:
        cpu::cpu_matmul_kernel<float>(
            M, K, N,
            static_cast<const float*>(A),
            static_cast<const float*>(B),
            static_cast<float*>(C));
        break;
    case DType::F64:
        cpu::cpu_matmul_kernel<double>(
            M, K, N,
            static_cast<const double*>(A),
            static_cast<const double*>(B),
            static_cast<double*>(C));
        break;
    case DType::I32:
        cpu::cpu_matmul_kernel<int32_t>(
            M, K, N,
            static_cast<const int32_t*>(A),
            static_cast<const int32_t*>(B),
            static_cast<int32_t*>(C));
        break;
    case DType::I64:
        cpu::cpu_matmul_kernel<int64_t>(
            M, K, N,
            static_cast<const int64_t*>(A),
            static_cast<const int64_t*>(B),
            static_cast<int64_t*>(C));
        break;
    default:
        // 不应到达此处（调用方已校验 dtype）
        break;
    }
}

} // namespace

auto matmul(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    // ── 基本有效性检查 ────────────────────────────────────────────────────────
    if (!a.defined() || !b.defined())
        return std::unexpected(make_error(
            "matmul: 输入 Tensor 未定义", Severity::Recoverable));

    // ── device 检查 ───────────────────────────────────────────────────────────
    if (a.device() != b.device())
        return std::unexpected(make_error(
            std::format("matmul: 两操作数 device 不同（{} vs {}）",
                        a.device() == Device::CPU ? "CPU" : "CUDA",
                        b.device() == Device::CPU ? "CPU" : "CUDA"),
            Severity::Recoverable));

    if (a.device() == Device::CUDA)
        return std::unexpected(make_error(
            "matmul: CUDA 后端未实现", Severity::Recoverable));

    // ── dtype 检查 ────────────────────────────────────────────────────────────
    if (a.dtype() != b.dtype())
        return std::unexpected(make_error(
            std::format("matmul: dtype 不匹配（{} vs {}）",
                        dtype_name(a.dtype()), dtype_name(b.dtype())),
            Severity::Recoverable));

    const DType dt = a.dtype();
    if (dt == DType::Bool || dt == DType::U8)
        return std::unexpected(make_error(
            std::format("matmul: 不支持 DType::{}", dtype_name(dt)),
            Severity::Recoverable));

    // ── 维度检查：仅支持 2D × 2D ─────────────────────────────────────────────
    if (a.ndim() != 2)
        return std::unexpected(make_error(
            std::format("matmul: a 必须是 2D 张量，当前 ndim={}", a.ndim()),
            Severity::Recoverable));

    if (b.ndim() != 2)
        return std::unexpected(make_error(
            std::format("matmul: b 必须是 2D 张量，当前 ndim={}", b.ndim()),
            Severity::Recoverable));

    // ── shape 相容性检查 ──────────────────────────────────────────────────────
    const int64_t M  = a.shape()[0];
    const int64_t Ka = a.shape()[1];
    const int64_t Kb = b.shape()[0];
    const int64_t N  = b.shape()[1];

    if (Ka != Kb)
        return std::unexpected(make_error(
            std::format("matmul: 内维不匹配（a 列={}, b 行={}）", Ka, Kb),
            Severity::Recoverable));

    const int64_t K = Ka;

    // ── M 或 N 为 0：直接返回形状正确的零张量 ────────────────────────────────
    if (M == 0 || N == 0) {
        auto out = Tensor::zeros({M, N}, dt);
        if (!out) return std::unexpected(std::move(out.error()));
        return *out;
    }

    // ── 输出张量（全零，contiguous）──────────────────────────────────────────
    auto out = Tensor::zeros({M, N}, dt);
    if (!out) return std::unexpected(std::move(out.error()));

    // K==0：输出已经是全 0，直接返回
    if (K == 0)
        return *out;

    // ── 非 contiguous 输入自动整理 ────────────────────────────────────────────
    Tensor ca = a;
    if (!a.is_contiguous()) {
        auto r = a.contiguous();
        if (!r) return std::unexpected(std::move(r.error()));
        ca = *r;
    }

    Tensor cb = b;
    if (!b.is_contiguous()) {
        auto r = b.contiguous();
        if (!r) return std::unexpected(std::move(r.error()));
        cb = *r;
    }

    // ── 调用 CPU 内核 ─────────────────────────────────────────────────────────
    dispatch_matmul_cpu(
        M, K, N,
        dt,
        ca.data_ptr(),
        cb.data_ptr(),
        out->data_ptr());

    return *out;
}

} // namespace bee
