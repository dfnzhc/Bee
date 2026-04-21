#include "Tensor/Ops/Reduce.hpp"
#include "Tensor/Cpu/ReduceCpu.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"

#include <format>

namespace bee
{

// ─────────────────────────────────────────────────────────────────────────────
// 内部辅助：dtype 校验
// ─────────────────────────────────────────────────────────────────────────────

namespace
{

// sum/prod 支持 F32/F64/I32/I64；Bool 与 U8 不支持
auto check_dtype_sum_prod(DType dt, std::string_view op) -> Result<void>
{
    if (dt == DType::Bool || dt == DType::U8)
        return std::unexpected(make_error(
            std::format("{} 不支持 DType::{}", op, dtype_name(dt)),
            Severity::Recoverable));
    return {};
}

// min/max 支持 F32/F64/I32/I64/U8；Bool 不支持
auto check_dtype_minmax(DType dt, std::string_view op) -> Result<void>
{
    if (dt == DType::Bool)
        return std::unexpected(make_error(
            std::format("{} 不支持 DType::Bool", op),
            Severity::Recoverable));
    return {};
}

// mean 支持 F32/F64/I32/I64；Bool 与 U8 不支持
auto check_dtype_mean(DType dt, std::string_view op) -> Result<void>
{
    if (dt == DType::Bool || dt == DType::U8)
        return std::unexpected(make_error(
            std::format("{} 不支持 DType::{}", op, dtype_name(dt)),
            Severity::Recoverable));
    return {};
}

// mean 输出 dtype：F32→F32, F64→F64, I32/I64→F64
auto mean_out_dtype(DType dt) -> DType
{
    if (dt == DType::F32)
        return DType::F32;
    return DType::F64; // F64/I32/I64 均输出 F64
}

// 公共前置校验（全局 reduce）：defined + CPU + dtype
template <typename CheckFn>
auto check_global_precond(
    const Tensor& a, std::string_view op, CheckFn dtype_check) -> Result<void>
{
    if (!a.defined())
        return std::unexpected(make_error(
            std::format("{}: 输入 Tensor 未定义", op), Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error(
            std::format("{}: CUDA 后端未实现", op), Severity::Recoverable));
    return dtype_check(a.dtype(), op);
}

// 公共前置校验（按轴 reduce）：defined + CPU + dtype + ndim + dim 范围
// 成功时返回经规范化的正数 dim（已消除负索引）
template <typename CheckFn>
auto check_axis_precond(
    const Tensor& a, int dim, std::string_view op, CheckFn dtype_check)
    -> Result<int64_t>
{
    if (!a.defined())
        return std::unexpected(make_error(
            std::format("{}: 输入 Tensor 未定义", op), Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error(
            std::format("{}: CUDA 后端未实现", op), Severity::Recoverable));
    {
        auto r = dtype_check(a.dtype(), op);
        if (!r) return std::unexpected(std::move(r.error()));
    }

    const int64_t ndim = a.ndim();
    if (ndim == 0)
        return std::unexpected(make_error(
            std::format("{}: 0-rank 张量不支持按轴 reduce", op),
            Severity::Recoverable));

    // 规范化负 dim
    int64_t d = static_cast<int64_t>(dim);
    if (d < 0)
        d += ndim;
    if (d < 0 || d >= ndim)
        return std::unexpected(make_error(
            std::format("{}: dim={} 越界（ndim={}）", op, dim, ndim),
            Severity::Recoverable));
    return d;
}

// ─────────────────────────────────────────────────────────────────────────────
// 全局 reduce dispatch：运行期分派到 ISA 特化 namespace
// ─────────────────────────────────────────────────────────────────────────────

enum class RdOp { Sum, Min, Max, Prod };

auto dispatch_global_cpu(RdOp op, const Tensor& a, Tensor& out) -> void
{
    switch (op) {
    case RdOp::Sum:  BEE_RT_DISPATCH(rd_sum_global,  a, out);
    case RdOp::Min:  BEE_RT_DISPATCH(rd_min_global,  a, out);
    case RdOp::Max:  BEE_RT_DISPATCH(rd_max_global,  a, out);
    case RdOp::Prod: BEE_RT_DISPATCH(rd_prod_global, a, out);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 按轴 reduce dispatch：根据 dtype 调用对应类型的 CPU 内核
// ─────────────────────────────────────────────────────────────────────────────

template <typename Op>
auto dispatch_axis_cpu(const Tensor& a, int64_t dim, bool keepdim, Tensor& out) -> void
{
    switch (a.dtype()) {
    case DType::F32: cpu::cpu_reduce_axis_dispatch<float,   Op>(a, dim, keepdim, out); break;
    case DType::F64: cpu::cpu_reduce_axis_dispatch<double,  Op>(a, dim, keepdim, out); break;
    case DType::I32: cpu::cpu_reduce_axis_dispatch<int32_t, Op>(a, dim, keepdim, out); break;
    case DType::I64: cpu::cpu_reduce_axis_dispatch<int64_t, Op>(a, dim, keepdim, out); break;
    case DType::U8:  cpu::cpu_reduce_axis_dispatch<uint8_t, Op>(a, dim, keepdim, out); break;
    default:         break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 辅助：构建按轴 reduce 的输出 shape
// ─────────────────────────────────────────────────────────────────────────────

auto make_reduce_axis_shape(const Shape& in_shape, int64_t dim, bool keepdim) -> Shape
{
    Shape out;
    out.reserve(in_shape.size());
    for (int64_t d = 0; d < static_cast<int64_t>(in_shape.size()); ++d) {
        if (d == dim)
            keepdim ? out.push_back(1) : (void)0;
        else
            out.push_back(in_shape[static_cast<std::size_t>(d)]);
    }
    return out;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// 全局 reduce 实现
// ─────────────────────────────────────────────────────────────────────────────

auto sum(const Tensor& a) -> Result<Tensor>
{
    {
        auto r = check_global_precond(a, "sum", check_dtype_sum_prod);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    // 输出为 0-rank 标量张量（shape={}，numel=1）
    auto out = Tensor::empty({}, a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_global_cpu(RdOp::Sum, a, *out);
    return *out;
}

auto mean(const Tensor& a) -> Result<Tensor>
{
    {
        auto r = check_global_precond(a, "mean", check_dtype_mean);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    const DType out_dt = mean_out_dtype(a.dtype());
    auto out = Tensor::empty({}, out_dt);
    if (!out) return std::unexpected(std::move(out.error()));

    if (a.dtype() == DType::F32) {
        // F32 → F32：复用 sum 核心再除以 n
        dispatch_global_cpu(RdOp::Sum, a, *out);
        auto* p = static_cast<float*>(out->data_ptr());
        p[0] /= static_cast<float>(a.numel());
    } else if (a.dtype() == DType::F64) {
        // F64 → F64
        dispatch_global_cpu(RdOp::Sum, a, *out);
        auto* p = static_cast<double*>(out->data_ptr());
        p[0] /= static_cast<double>(a.numel());
    } else if (a.dtype() == DType::I32) {
        // I32 → F64：以 double 精度累加
        cpu::cpu_reduce_mean_global_dispatch<int32_t, double>(a, *out);
    } else {
        // I64 → F64
        cpu::cpu_reduce_mean_global_dispatch<int64_t, double>(a, *out);
    }
    return *out;
}

auto min(const Tensor& a) -> Result<Tensor>
{
    {
        auto r = check_global_precond(a, "min", check_dtype_minmax);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    if (a.numel() == 0)
    {
        return std::unexpected(make_error(
            "min: 不支持空张量（numel == 0）",
            Severity::Recoverable));
    }
    auto out = Tensor::empty({}, a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_global_cpu(RdOp::Min, a, *out);
    return *out;
}

auto max(const Tensor& a) -> Result<Tensor>
{
    {
        auto r = check_global_precond(a, "max", check_dtype_minmax);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    if (a.numel() == 0)
    {
        return std::unexpected(make_error(
            "max: 不支持空张量（numel == 0）",
            Severity::Recoverable));
    }
    auto out = Tensor::empty({}, a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_global_cpu(RdOp::Max, a, *out);
    return *out;
}

auto prod(const Tensor& a) -> Result<Tensor>
{
    {
        auto r = check_global_precond(a, "prod", check_dtype_sum_prod);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    auto out = Tensor::empty({}, a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_global_cpu(RdOp::Prod, a, *out);
    return *out;
}

// ─────────────────────────────────────────────────────────────────────────────
// 按轴 reduce 实现
// ─────────────────────────────────────────────────────────────────────────────

auto sum(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "sum", check_dtype_sum_prod);
    if (!dim_r) return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;

    auto out = Tensor::empty(make_reduce_axis_shape(a.shape(), d, keepdim), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_axis_cpu<cpu::OpReduceSum>(a, d, keepdim, *out);
    return *out;
}

auto mean(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "mean", check_dtype_mean);
    if (!dim_r) return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;

    const DType out_dt = mean_out_dtype(a.dtype());
    auto out = Tensor::empty(make_reduce_axis_shape(a.shape(), d, keepdim), out_dt);
    if (!out) return std::unexpected(std::move(out.error()));

    if (a.dtype() == DType::F32) {
        // F32 → F32：累加后除以轴长
        cpu::cpu_reduce_axis_dispatch<float, cpu::OpReduceSum>(a, d, keepdim, *out);
        const int64_t K = a.shape()[static_cast<std::size_t>(d)];
        auto* p = static_cast<float*>(out->data_ptr());
        for (int64_t i = 0; i < out->numel(); ++i)
            p[i] /= static_cast<float>(K);
    } else if (a.dtype() == DType::F64) {
        cpu::cpu_reduce_axis_dispatch<double, cpu::OpReduceSum>(a, d, keepdim, *out);
        const int64_t K = a.shape()[static_cast<std::size_t>(d)];
        auto* p = static_cast<double*>(out->data_ptr());
        for (int64_t i = 0; i < out->numel(); ++i)
            p[i] /= static_cast<double>(K);
    } else if (a.dtype() == DType::I32) {
        // I32 → F64：通过 double 精度累加并直接除以 K
        cpu::cpu_reduce_mean_axis_dispatch<int32_t, double>(a, d, keepdim, *out);
    } else {
        // I64 → F64
        cpu::cpu_reduce_mean_axis_dispatch<int64_t, double>(a, d, keepdim, *out);
    }
    return *out;
}

auto min(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "min", check_dtype_minmax);
    if (!dim_r) return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;
    const int64_t K = a.shape()[static_cast<std::size_t>(d)];

    if (K == 0)
    {
        return std::unexpected(make_error(
            "min: 被 reduce 的维度大小为 0",
            Severity::Recoverable));
    }

    auto out = Tensor::empty(make_reduce_axis_shape(a.shape(), d, keepdim), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_axis_cpu<cpu::OpReduceMin>(a, d, keepdim, *out);
    return *out;
}

auto max(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "max", check_dtype_minmax);
    if (!dim_r) return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;
    const int64_t K = a.shape()[static_cast<std::size_t>(d)];

    if (K == 0)
    {
        return std::unexpected(make_error(
            "max: 被 reduce 的维度大小为 0",
            Severity::Recoverable));
    }

    auto out = Tensor::empty(make_reduce_axis_shape(a.shape(), d, keepdim), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_axis_cpu<cpu::OpReduceMax>(a, d, keepdim, *out);
    return *out;
}

auto prod(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "prod", check_dtype_sum_prod);
    if (!dim_r) return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;

    auto out = Tensor::empty(make_reduce_axis_shape(a.shape(), d, keepdim), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));
    dispatch_axis_cpu<cpu::OpReduceProd>(a, d, keepdim, *out);
    return *out;
}

} // namespace bee
