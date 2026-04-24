#include "Tensor/Ops/Reduce.hpp"
#include "Tensor/Ops/Cast.hpp"
#include "Tensor/Cpu/ReduceCpu.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <format>

namespace bee
{

// ─────────────────────────────────────────────────────────────────────────────
// 内部辅助：dtype 校验
// ─────────────────────────────────────────────────────────────────────────────

namespace
{

    auto check_dtype_sum_prod(DType dt, std::string_view op) -> Result<void>
    {
        if (dt == DType::Bool || dt == DType::U8)
            return std::unexpected(make_error(std::format("{} 不支持 DType::{}", op, enum_to_name(dt)), Severity::Recoverable));
        return {};
    }

    auto check_dtype_minmax(DType dt, std::string_view op) -> Result<void>
    {
        if (dt == DType::Bool)
            return std::unexpected(make_error(std::format("{} 不支持 DType::Bool", op), Severity::Recoverable));
        return {};
    }

    auto check_dtype_mean(DType dt, std::string_view op) -> Result<void>
    {
        if (dt == DType::Bool || dt == DType::U8)
            return std::unexpected(make_error(std::format("{} 不支持 DType::{}", op, enum_to_name(dt)), Severity::Recoverable));
        return {};
    }

    auto mean_out_dtype(DType dt) -> DType
    {
        if (dt == DType::F32)
            return DType::F32;
        return DType::F64;
    }

    template <typename CheckFn>
    auto check_global_precond(const Tensor& a, std::string_view op, CheckFn dtype_check) -> Result<void>
    {
        if (!a.defined())
            return std::unexpected(make_error(std::format("{}: 输入 Tensor 未定义", op), Severity::Recoverable));
        return dtype_check(a.dtype(), op);
    }

    template <typename CheckFn>
    auto check_axis_precond(const Tensor& a, int dim, std::string_view op, CheckFn dtype_check) -> Result<int64_t>
    {
        if (!a.defined())
            return std::unexpected(make_error(std::format("{}: 输入 Tensor 未定义", op), Severity::Recoverable));
        {
            auto r = dtype_check(a.dtype(), op);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }

        const int64_t ndim = a.ndim();
        if (ndim == 0)
            return std::unexpected(make_error(std::format("{}: 0-rank 张量不支持按轴 reduce", op), Severity::Recoverable));

        int64_t d = static_cast<int64_t>(dim);
        if (d < 0)
            d += ndim;
        if (d < 0 || d >= ndim)
            return std::unexpected(make_error(std::format("{}: dim={} 越界（ndim={}）", op, dim, ndim), Severity::Recoverable));
        return d;
    }

    enum class RdOp
    {
        Sum,
        Min,
        Max,
        Prod
    };

    auto dispatch_global_cpu(RdOp op, const Tensor& a, Tensor& out) -> void
    {
        switch (op) {
        case RdOp::Sum: BEE_RT_DISPATCH(rd_sum_global, a, out);
        case RdOp::Min: BEE_RT_DISPATCH(rd_min_global, a, out);
        case RdOp::Max: BEE_RT_DISPATCH(rd_max_global, a, out);
        case RdOp::Prod: BEE_RT_DISPATCH(rd_prod_global, a, out);
        }
    }

    template <typename Op>
    auto dispatch_axis_cpu(const Tensor& a, int64_t dim, bool keepdim, Tensor& out) -> void
    {
        switch (a.dtype()) {
        case DType::F32: cpu::cpu_reduce_axis_dispatch<float, Op>(a, dim, keepdim, out); break;
        case DType::F64: cpu::cpu_reduce_axis_dispatch<double, Op>(a, dim, keepdim, out); break;
        case DType::I32: cpu::cpu_reduce_axis_dispatch<int32_t, Op>(a, dim, keepdim, out); break;
        case DType::I64: cpu::cpu_reduce_axis_dispatch<int64_t, Op>(a, dim, keepdim, out); break;
        case DType::U8: cpu::cpu_reduce_axis_dispatch<uint8_t, Op>(a, dim, keepdim, out); break;
        default: break;
        }
    }

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

    // ─────────────────────────────────────────────────────────────────────────
    // CUDA helpers
    // ─────────────────────────────────────────────────────────────────────────

    auto check_cuda_contig(const Tensor& a, std::string_view op) -> Result<void>
    {
        if (!a.is_contiguous())
            return std::unexpected(make_error(std::format("{}: CUDA 后端要求输入 contiguous", op), Severity::Recoverable));
        return {};
    }

    auto run_global_cuda(RdOp op, const Tensor& a, Tensor& out, std::string_view op_name) -> Result<void>
    {
        if (auto r = check_cuda_contig(a, op_name); !r)
            return std::unexpected(std::move(r.error()));
        return tensor::cuda::reduce_global(
            static_cast<int>(op), static_cast<int>(a.dtype()), a.data_ptr(), out.data_ptr(), static_cast<std::size_t>(a.numel())
        );
    }

    auto run_axis_cuda(RdOp op, const Tensor& a, int64_t dim, Tensor& out, std::string_view op_name) -> Result<void>
    {
        if (auto r = check_cuda_contig(a, op_name); !r)
            return std::unexpected(std::move(r.error()));
        std::size_t outer = 1, inner = 1;
        const auto& s = a.shape();
        for (int64_t i = 0; i < dim; ++i)
            outer *= static_cast<std::size_t>(s[static_cast<std::size_t>(i)]);
        for (int64_t i = dim + 1; i < static_cast<int64_t>(s.size()); ++i)
            inner *= static_cast<std::size_t>(s[static_cast<std::size_t>(i)]);
        const std::size_t axis_n = static_cast<std::size_t>(s[static_cast<std::size_t>(dim)]);
        return tensor::cuda::reduce_axis(static_cast<int>(op), static_cast<int>(a.dtype()), a.data_ptr(), out.data_ptr(), outer, axis_n, inner);
    }

    auto mean_not_impl_on_cuda_for_int(DType dt, std::string_view op) -> Result<void>
    {
        if (dt == DType::I32 || dt == DType::I64)
            return std::unexpected(make_error(std::format("{}: CUDA 后端暂未实现整数输入（I32/I64 → F64）", op), Severity::Recoverable));
        return {};
    }

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// 全局 reduce 实现
// ─────────────────────────────────────────────────────────────────────────────

auto sum(const Tensor& a) -> Result<Tensor>
{
    if (auto r = check_global_precond(a, "sum", check_dtype_sum_prod); !r)
        return std::unexpected(std::move(r.error()));

    // 低精度 F16/BF16：提升到 F32 后再 reduce，输出为 F32
    if (a.dtype() == DType::F16 || a.dtype() == DType::BF16) {
        auto f32 = cast(a, DType::F32);
        if (!f32)
            return std::unexpected(std::move(f32.error()));
        return sum(*f32);
    }

    auto out = Tensor::empty({}, a.dtype(), a.device());
    if (!out)
        return std::unexpected(std::move(out.error()));

    if (a.device() == Device::CUDA) {
        if (auto r = run_global_cuda(RdOp::Sum, a, *out, "sum"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_global_cpu(RdOp::Sum, a, *out);
    return *out;
}

auto mean(const Tensor& a) -> Result<Tensor>
{
    if (auto r = check_global_precond(a, "mean", check_dtype_mean); !r)
        return std::unexpected(std::move(r.error()));
    if (a.numel() == 0)
        return std::unexpected(make_error("mean: 不支持空张量（numel == 0）", Severity::Recoverable));

    const DType out_dt = mean_out_dtype(a.dtype());
    auto        out    = Tensor::empty({}, out_dt, a.device());
    if (!out)
        return std::unexpected(std::move(out.error()));

    if (a.device() == Device::CUDA) {
        if (auto r = mean_not_impl_on_cuda_for_int(a.dtype(), "mean"); !r)
            return std::unexpected(std::move(r.error()));
        if (auto r = run_global_cuda(RdOp::Sum, a, *out, "mean"); !r)
            return std::unexpected(std::move(r.error()));
        const double inv = 1.0 / static_cast<double>(a.numel());
        return tensor::cuda::scale_fp(static_cast<int>(a.dtype()), out->data_ptr(), inv, 1).transform([&] { return *out; });
    }

    if (a.dtype() == DType::F32) {
        dispatch_global_cpu(RdOp::Sum, a, *out);
        auto* p  = static_cast<float*>(out->data_ptr());
        p[0]    /= static_cast<float>(a.numel());
    } else if (a.dtype() == DType::F64) {
        dispatch_global_cpu(RdOp::Sum, a, *out);
        auto* p  = static_cast<double*>(out->data_ptr());
        p[0]    /= static_cast<double>(a.numel());
    } else if (a.dtype() == DType::I32) {
        cpu::cpu_reduce_mean_global_dispatch<int32_t, double>(a, *out);
    } else {
        cpu::cpu_reduce_mean_global_dispatch<int64_t, double>(a, *out);
    }
    return *out;
}

auto min(const Tensor& a) -> Result<Tensor>
{
    if (auto r = check_global_precond(a, "min", check_dtype_minmax); !r)
        return std::unexpected(std::move(r.error()));
    if (a.numel() == 0)
        return std::unexpected(make_error("min: 不支持空张量（numel == 0）", Severity::Recoverable));
    auto out = Tensor::empty({}, a.dtype(), a.device());
    if (!out)
        return std::unexpected(std::move(out.error()));
    if (a.device() == Device::CUDA) {
        if (auto r = run_global_cuda(RdOp::Min, a, *out, "min"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_global_cpu(RdOp::Min, a, *out);
    return *out;
}

auto max(const Tensor& a) -> Result<Tensor>
{
    if (auto r = check_global_precond(a, "max", check_dtype_minmax); !r)
        return std::unexpected(std::move(r.error()));
    if (a.numel() == 0)
        return std::unexpected(make_error("max: 不支持空张量（numel == 0）", Severity::Recoverable));
    auto out = Tensor::empty({}, a.dtype(), a.device());
    if (!out)
        return std::unexpected(std::move(out.error()));
    if (a.device() == Device::CUDA) {
        if (auto r = run_global_cuda(RdOp::Max, a, *out, "max"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_global_cpu(RdOp::Max, a, *out);
    return *out;
}

auto prod(const Tensor& a) -> Result<Tensor>
{
    if (auto r = check_global_precond(a, "prod", check_dtype_sum_prod); !r)
        return std::unexpected(std::move(r.error()));
    auto out = Tensor::empty({}, a.dtype(), a.device());
    if (!out)
        return std::unexpected(std::move(out.error()));
    if (a.device() == Device::CUDA) {
        if (auto r = run_global_cuda(RdOp::Prod, a, *out, "prod"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_global_cpu(RdOp::Prod, a, *out);
    return *out;
}

// ─────────────────────────────────────────────────────────────────────────────
// 按轴 reduce 实现
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
    auto make_axis_out(const Tensor& a, int64_t d, bool keepdim, DType out_dt) -> Result<Tensor>
    {
        return Tensor::empty(make_reduce_axis_shape(a.shape(), d, keepdim), out_dt, a.device());
    }
} // namespace

auto sum(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "sum", check_dtype_sum_prod);
    if (!dim_r)
        return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;

    auto out = make_axis_out(a, d, keepdim, a.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));

    if (a.device() == Device::CUDA) {
        if (auto r = run_axis_cuda(RdOp::Sum, a, d, *out, "sum"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_axis_cpu<cpu::OpReduceSum>(a, d, keepdim, *out);
    return *out;
}

auto mean(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "mean", check_dtype_mean);
    if (!dim_r)
        return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;
    const int64_t K = a.shape()[static_cast<std::size_t>(d)];
    if (K == 0)
        return std::unexpected(make_error("mean: 被 reduce 的维度大小为 0", Severity::Recoverable));

    const DType out_dt = mean_out_dtype(a.dtype());
    auto        out    = make_axis_out(a, d, keepdim, out_dt);
    if (!out)
        return std::unexpected(std::move(out.error()));

    if (a.device() == Device::CUDA) {
        if (auto r = mean_not_impl_on_cuda_for_int(a.dtype(), "mean"); !r)
            return std::unexpected(std::move(r.error()));
        if (auto r = run_axis_cuda(RdOp::Sum, a, d, *out, "mean"); !r)
            return std::unexpected(std::move(r.error()));
        const double inv = 1.0 / static_cast<double>(K);
        if (auto r = tensor::cuda::scale_fp(static_cast<int>(a.dtype()), out->data_ptr(), inv, static_cast<std::size_t>(out->numel())); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }

    if (a.dtype() == DType::F32) {
        cpu::cpu_reduce_axis_dispatch<float, cpu::OpReduceSum>(a, d, keepdim, *out);
        auto* p = static_cast<float*>(out->data_ptr());
        for (int64_t i = 0; i < out->numel(); ++i)
            p[i] /= static_cast<float>(K);
    } else if (a.dtype() == DType::F64) {
        cpu::cpu_reduce_axis_dispatch<double, cpu::OpReduceSum>(a, d, keepdim, *out);
        auto* p = static_cast<double*>(out->data_ptr());
        for (int64_t i = 0; i < out->numel(); ++i)
            p[i] /= static_cast<double>(K);
    } else if (a.dtype() == DType::I32) {
        cpu::cpu_reduce_mean_axis_dispatch<int32_t, double>(a, d, keepdim, *out);
    } else {
        cpu::cpu_reduce_mean_axis_dispatch<int64_t, double>(a, d, keepdim, *out);
    }
    return *out;
}

auto min(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "min", check_dtype_minmax);
    if (!dim_r)
        return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;
    const int64_t K = a.shape()[static_cast<std::size_t>(d)];

    if (K == 0)
        return std::unexpected(make_error("min: 被 reduce 的维度大小为 0", Severity::Recoverable));

    auto out = make_axis_out(a, d, keepdim, a.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));
    if (a.device() == Device::CUDA) {
        if (auto r = run_axis_cuda(RdOp::Min, a, d, *out, "min"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_axis_cpu<cpu::OpReduceMin>(a, d, keepdim, *out);
    return *out;
}

auto max(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "max", check_dtype_minmax);
    if (!dim_r)
        return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;
    const int64_t K = a.shape()[static_cast<std::size_t>(d)];

    if (K == 0)
        return std::unexpected(make_error("max: 被 reduce 的维度大小为 0", Severity::Recoverable));

    auto out = make_axis_out(a, d, keepdim, a.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));
    if (a.device() == Device::CUDA) {
        if (auto r = run_axis_cuda(RdOp::Max, a, d, *out, "max"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_axis_cpu<cpu::OpReduceMax>(a, d, keepdim, *out);
    return *out;
}

auto prod(const Tensor& a, int dim, bool keepdim) -> Result<Tensor>
{
    auto dim_r = check_axis_precond(a, dim, "prod", check_dtype_sum_prod);
    if (!dim_r)
        return std::unexpected(std::move(dim_r.error()));
    const int64_t d = *dim_r;

    auto out = make_axis_out(a, d, keepdim, a.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));
    if (a.device() == Device::CUDA) {
        if (auto r = run_axis_cuda(RdOp::Prod, a, d, *out, "prod"); !r)
            return std::unexpected(std::move(r.error()));
        return *out;
    }
    dispatch_axis_cpu<cpu::OpReduceProd>(a, d, keepdim, *out);
    return *out;
}

} // namespace bee
