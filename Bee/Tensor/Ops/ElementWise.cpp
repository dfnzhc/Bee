#include "Tensor/Ops/ElementWise.hpp"
#include "Tensor/Ops/Broadcast.hpp"
#include "Tensor/Cpu/ElementWiseCpu.hpp"

#include <format>

namespace bee
{

namespace
{

auto check_binary_device(const Tensor& a, const Tensor& b, std::string_view op) -> Result<void>
{
    if (!a.defined() || !b.defined())
        return std::unexpected(make_error(
            std::format("{}: 输入 Tensor 未定义", op), Severity::Recoverable));
    if (a.device() != b.device())
        return std::unexpected(make_error(
            std::format("{}: 两操作数 device 不同（{} vs {}）", op,
                        a.device() == Device::CPU ? "CPU" : "CUDA",
                        b.device() == Device::CPU ? "CPU" : "CUDA"),
            Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error(
            std::format("{}: CUDA 后端未实现", op), Severity::Recoverable));
    return {};
}

auto check_same_dtype(const Tensor& a, const Tensor& b, std::string_view op) -> Result<void>
{
    if (a.dtype() != b.dtype())
        return std::unexpected(make_error(
            std::format("{}: dtype 不匹配（{} vs {}）", op,
                        dtype_name(a.dtype()), dtype_name(b.dtype())),
            Severity::Recoverable));
    return {};
}

auto check_dtype_addsub(DType dt, std::string_view op) -> Result<void>
{
    if (dt == DType::Bool)
        return std::unexpected(make_error(
            std::format("{} 不支持 DType::Bool", op), Severity::Recoverable));
    return {};
}

auto check_dtype_muldiv(DType dt, std::string_view op) -> Result<void>
{
    if (dt == DType::Bool)
        return std::unexpected(make_error(
            std::format("{} 不支持 DType::Bool", op), Severity::Recoverable));
    if (dt == DType::U8)
        return std::unexpected(make_error(
            std::format("{} 不支持 DType::U8", op), Severity::Recoverable));
    return {};
}

auto check_dtype_negabs(DType dt, std::string_view op) -> Result<void>
{
    if (dt == DType::Bool || dt == DType::U8)
        return std::unexpected(make_error(
            std::format("{} 不支持 DType::{}", op, dtype_name(dt)), Severity::Recoverable));
    return {};
}

auto check_dtype_float(DType dt, std::string_view op) -> Result<void>
{
    if (dt != DType::F32 && dt != DType::F64)
        return std::unexpected(make_error(
            std::format("{} 仅支持 DType::F32 / DType::F64，当前 dtype 为 {}",
                        op, dtype_name(dt)),
            Severity::Recoverable));
    return {};
}

template <typename Op>
auto dispatch_binary_cpu(const Tensor& a, const Tensor& b, Tensor& out) -> void
{
    switch (out.dtype()) {
    case DType::F32: cpu::cpu_elementwise_binary<float,    Op>(a, b, out); break;
    case DType::F64: cpu::cpu_elementwise_binary<double,   Op>(a, b, out); break;
    case DType::I32: cpu::cpu_elementwise_binary<int32_t,  Op>(a, b, out); break;
    case DType::I64: cpu::cpu_elementwise_binary<int64_t,  Op>(a, b, out); break;
    case DType::U8:  cpu::cpu_elementwise_binary<uint8_t,  Op>(a, b, out); break;
    default: break;
    }
}

template <typename Op>
auto dispatch_unary_cpu_negabs(const Tensor& a, Tensor& out) -> void
{
    switch (out.dtype()) {
    case DType::F32: cpu::cpu_elementwise_unary<float,   Op>(a, out); break;
    case DType::F64: cpu::cpu_elementwise_unary<double,  Op>(a, out); break;
    case DType::I32: cpu::cpu_elementwise_unary<int32_t, Op>(a, out); break;
    case DType::I64: cpu::cpu_elementwise_unary<int64_t, Op>(a, out); break;
    default: break;
    }
}

template <typename Op>
auto dispatch_unary_cpu_float(const Tensor& a, Tensor& out) -> void
{
    switch (out.dtype()) {
    case DType::F32: cpu::cpu_elementwise_unary<float,  Op>(a, out); break;
    case DType::F64: cpu::cpu_elementwise_unary<double, Op>(a, out); break;
    default: break;
    }
}

template <typename Op, typename Fn>
auto binary_op_impl(const Tensor& a, const Tensor& b, std::string_view op_name,
                    Fn check_dtype_fn) -> Result<Tensor>
{
    {
        auto r = check_binary_device(a, b, op_name);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    {
        auto r = check_same_dtype(a, b, op_name);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    {
        auto r = check_dtype_fn(a.dtype(), op_name);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    auto bshape = compute_broadcast_shape(a.shape(), b.shape());
    if (!bshape) return std::unexpected(std::move(bshape.error()));

    auto out = Tensor::empty(*bshape, a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));

    dispatch_binary_cpu<Op>(a, b, *out);
    return *out;
}

template <typename Op, typename Fn>
auto inplace_binary_impl(Tensor& dst, const Tensor& src, std::string_view op_name,
                         Fn check_dtype_fn) -> Result<void>
{
    if (!dst.defined() || !src.defined())
        return std::unexpected(make_error(
            std::format("{}: Tensor 未定义", op_name), Severity::Recoverable));
    if (dst.device() != src.device())
        return std::unexpected(make_error(
            std::format("{}: device 不一致", op_name), Severity::Recoverable));
    if (dst.device() == Device::CUDA)
        return std::unexpected(make_error(
            std::format("{}: CUDA 后端未实现", op_name), Severity::Recoverable));

    {
        auto r = check_same_dtype(dst, src, op_name);
        if (!r) return std::unexpected(std::move(r.error()));
    }
    {
        auto r = check_dtype_fn(dst.dtype(), op_name);
        if (!r) return std::unexpected(std::move(r.error()));
    }

    auto bshape = compute_broadcast_shape(dst.shape(), src.shape());
    if (!bshape) return std::unexpected(std::move(bshape.error()));
    if (*bshape != dst.shape())
        return std::unexpected(make_error(
            std::format("{}: in-place 广播后 shape 不等于 dst.shape，"
                        "src 不能比 dst 大",
                        op_name),
            Severity::Recoverable));

    dispatch_binary_cpu<Op>(dst, src, dst);
    return {};
}

} // namespace

auto add(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<cpu::OpAdd>(a, b, "add",
        [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto sub(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<cpu::OpSub>(a, b, "sub",
        [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto mul(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<cpu::OpMul>(a, b, "mul",
        [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

auto div(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<cpu::OpDiv>(a, b, "div",
        [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

auto neg(const Tensor& a) -> Result<Tensor>
{
    if (!a.defined())
        return std::unexpected(make_error("neg: Tensor 未定义", Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error("neg: CUDA 后端未实现", Severity::Recoverable));
    {
        auto r = check_dtype_negabs(a.dtype(), "neg");
        if (!r) return std::unexpected(std::move(r.error()));
    }

    auto out = Tensor::empty(a.shape(), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));

    dispatch_unary_cpu_negabs<cpu::OpNeg>(a, *out);
    return *out;
}

auto abs(const Tensor& a) -> Result<Tensor>
{
    if (!a.defined())
        return std::unexpected(make_error("abs: Tensor 未定义", Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error("abs: CUDA 后端未实现", Severity::Recoverable));
    {
        auto r = check_dtype_negabs(a.dtype(), "abs");
        if (!r) return std::unexpected(std::move(r.error()));
    }

    auto out = Tensor::empty(a.shape(), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));

    dispatch_unary_cpu_negabs<cpu::OpAbs>(a, *out);
    return *out;
}

auto sqrt(const Tensor& a) -> Result<Tensor>
{
    if (!a.defined())
        return std::unexpected(make_error("sqrt: Tensor 未定义", Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error("sqrt: CUDA 后端未实现", Severity::Recoverable));
    {
        auto r = check_dtype_float(a.dtype(), "sqrt");
        if (!r) return std::unexpected(std::move(r.error()));
    }

    auto out = Tensor::empty(a.shape(), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));

    dispatch_unary_cpu_float<cpu::OpSqrt>(a, *out);
    return *out;
}

auto exp(const Tensor& a) -> Result<Tensor>
{
    if (!a.defined())
        return std::unexpected(make_error("exp: Tensor 未定义", Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error("exp: CUDA 后端未实现", Severity::Recoverable));
    {
        auto r = check_dtype_float(a.dtype(), "exp");
        if (!r) return std::unexpected(std::move(r.error()));
    }

    auto out = Tensor::empty(a.shape(), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));

    dispatch_unary_cpu_float<cpu::OpExp>(a, *out);
    return *out;
}

auto log(const Tensor& a) -> Result<Tensor>
{
    if (!a.defined())
        return std::unexpected(make_error("log: Tensor 未定义", Severity::Recoverable));
    if (a.device() == Device::CUDA)
        return std::unexpected(make_error("log: CUDA 后端未实现", Severity::Recoverable));
    {
        auto r = check_dtype_float(a.dtype(), "log");
        if (!r) return std::unexpected(std::move(r.error()));
    }

    auto out = Tensor::empty(a.shape(), a.dtype());
    if (!out) return std::unexpected(std::move(out.error()));

    dispatch_unary_cpu_float<cpu::OpLog>(a, *out);
    return *out;
}

auto add_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<cpu::OpAdd>(dst, src, "add_inplace",
        [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto sub_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<cpu::OpSub>(dst, src, "sub_inplace",
        [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto mul_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<cpu::OpMul>(dst, src, "mul_inplace",
        [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

auto div_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<cpu::OpDiv>(dst, src, "div_inplace",
        [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

namespace
{
auto inplace_unary_impl_negabs(Tensor& dst, std::string_view op_name) -> Result<void>
{
    if (!dst.defined())
        return std::unexpected(make_error(
            std::format("{}: Tensor 未定义", op_name), Severity::Recoverable));
    if (dst.device() == Device::CUDA)
        return std::unexpected(make_error(
            std::format("{}: CUDA 后端未实现", op_name), Severity::Recoverable));
    return check_dtype_negabs(dst.dtype(), op_name);
}

auto inplace_unary_impl_float(Tensor& dst, std::string_view op_name) -> Result<void>
{
    if (!dst.defined())
        return std::unexpected(make_error(
            std::format("{}: Tensor 未定义", op_name), Severity::Recoverable));
    if (dst.device() == Device::CUDA)
        return std::unexpected(make_error(
            std::format("{}: CUDA 后端未实现", op_name), Severity::Recoverable));
    return check_dtype_float(dst.dtype(), op_name);
}
} // namespace

auto neg_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_negabs(dst, "neg_inplace");
    if (!r) return r;
    dispatch_unary_cpu_negabs<cpu::OpNeg>(dst, dst);
    return {};
}

auto abs_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_negabs(dst, "abs_inplace");
    if (!r) return r;
    dispatch_unary_cpu_negabs<cpu::OpAbs>(dst, dst);
    return {};
}

auto sqrt_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_float(dst, "sqrt_inplace");
    if (!r) return r;
    dispatch_unary_cpu_float<cpu::OpSqrt>(dst, dst);
    return {};
}

auto exp_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_float(dst, "exp_inplace");
    if (!r) return r;
    dispatch_unary_cpu_float<cpu::OpExp>(dst, dst);
    return {};
}

auto log_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_float(dst, "log_inplace");
    if (!r) return r;
    dispatch_unary_cpu_float<cpu::OpLog>(dst, dst);
    return {};
}

} // namespace bee
