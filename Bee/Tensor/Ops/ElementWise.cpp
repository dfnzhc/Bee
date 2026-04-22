#include "Tensor/Ops/ElementWise.hpp"
#include "Tensor/Ops/Broadcast.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <format>

namespace bee
{

namespace
{

    auto check_binary_device(const Tensor& a, const Tensor& b, std::string_view op) -> Result<void>
    {
        if (!a.defined() || !b.defined())
            return std::unexpected(make_error(std::format("{}: 输入 Tensor 未定义", op), Severity::Recoverable));
        if (a.device() != b.device())
            return std::unexpected(make_error(
                std::format(
                    "{}: 两操作数 device 不同（{} vs {}）", op, a.device() == Device::CPU ? "CPU" : "CUDA", b.device() == Device::CPU ? "CPU" : "CUDA"
                ),
                Severity::Recoverable
            ));
        return {};
    }

    auto check_same_dtype(const Tensor& a, const Tensor& b, std::string_view op) -> Result<void>
    {
        if (a.dtype() != b.dtype())
            return std::unexpected(
                make_error(std::format("{}: dtype 不匹配（{} vs {}）", op, dtype_name(a.dtype()), dtype_name(b.dtype())), Severity::Recoverable)
            );
        return {};
    }

    auto check_dtype_addsub(DType dt, std::string_view op) -> Result<void>
    {
        if (dt == DType::Bool)
            return std::unexpected(make_error(std::format("{} 不支持 DType::Bool", op), Severity::Recoverable));
        return {};
    }

    auto check_dtype_muldiv(DType dt, std::string_view op) -> Result<void>
    {
        if (dt == DType::Bool)
            return std::unexpected(make_error(std::format("{} 不支持 DType::Bool", op), Severity::Recoverable));
        if (dt == DType::U8)
            return std::unexpected(make_error(std::format("{} 不支持 DType::U8", op), Severity::Recoverable));
        return {};
    }

    auto check_dtype_negabs(DType dt, std::string_view op) -> Result<void>
    {
        if (dt == DType::Bool || dt == DType::U8)
            return std::unexpected(make_error(std::format("{} 不支持 DType::{}", op, dtype_name(dt)), Severity::Recoverable));
        return {};
    }

    auto check_dtype_float(DType dt, std::string_view op) -> Result<void>
    {
        if (dt != DType::F32 && dt != DType::F64)
            return std::unexpected(
                make_error(std::format("{} 仅支持 DType::F32 / DType::F64，当前 dtype 为 {}", op, dtype_name(dt)), Severity::Recoverable)
            );
        return {};
    }

    // 运行期分派到 ISA 特化 namespace
    enum class BinOp
    {
        Add,
        Sub,
        Mul,
        Div
    };
    enum class UnOp
    {
        Neg,
        Abs,
        Sqrt,
        Exp,
        Log
    };

    auto dispatch_binary_cpu(BinOp op, const Tensor& a, const Tensor& b, Tensor& out) -> void
    {
        switch (op) {
        case BinOp::Add: BEE_RT_DISPATCH(ew_add, a, b, out);
        case BinOp::Sub: BEE_RT_DISPATCH(ew_sub, a, b, out);
        case BinOp::Mul: BEE_RT_DISPATCH(ew_mul, a, b, out);
        case BinOp::Div: BEE_RT_DISPATCH(ew_div, a, b, out);
        }
    }

    auto dispatch_unary_cpu(UnOp op, const Tensor& a, Tensor& out) -> void
    {
        switch (op) {
        case UnOp::Neg: BEE_RT_DISPATCH(ew_neg, a, out);
        case UnOp::Abs: BEE_RT_DISPATCH(ew_abs, a, out);
        case UnOp::Sqrt: BEE_RT_DISPATCH(ew_sqrt, a, out);
        case UnOp::Exp: BEE_RT_DISPATCH(ew_exp, a, out);
        case UnOp::Log: BEE_RT_DISPATCH(ew_log, a, out);
        }
    }

    // CUDA 端二元算子：要求 a、b、out 形状一致且均连续（无广播）。
    auto run_binary_cuda(BinOp op, const Tensor& a, const Tensor& b, Tensor& out, std::string_view op_name) -> Result<void>
    {
        if (a.shape() != b.shape())
            return std::unexpected(make_error(
                std::format("{}: CUDA 后端暂不支持广播，两操作数 shape 必须一致", op_name),
                Severity::Recoverable
            ));
        if (!a.is_contiguous() || !b.is_contiguous() || !out.is_contiguous())
            return std::unexpected(make_error(
                std::format("{}: CUDA 后端要求所有张量均 contiguous", op_name),
                Severity::Recoverable
            ));
        return tensor::cuda::ew_binary(
            static_cast<int>(op),
            static_cast<int>(a.dtype()),
            a.data_ptr(), b.data_ptr(), out.data_ptr(),
            static_cast<std::size_t>(a.numel())
        );
    }

    auto run_unary_cuda(UnOp op, const Tensor& a, Tensor& out, std::string_view op_name) -> Result<void>
    {
        if (!a.is_contiguous() || !out.is_contiguous())
            return std::unexpected(make_error(
                std::format("{}: CUDA 后端要求所有张量均 contiguous", op_name),
                Severity::Recoverable
            ));
        return tensor::cuda::ew_unary(
            static_cast<int>(op),
            static_cast<int>(a.dtype()),
            a.data_ptr(), out.data_ptr(),
            static_cast<std::size_t>(a.numel())
        );
    }

    template <BinOp Op, typename Fn>
    auto binary_op_impl(const Tensor& a, const Tensor& b, std::string_view op_name, Fn check_dtype_fn) -> Result<Tensor>
    {
        {
            auto r = check_binary_device(a, b, op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }
        {
            auto r = check_same_dtype(a, b, op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }
        {
            auto r = check_dtype_fn(a.dtype(), op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }
        auto bshape = compute_broadcast_shape(a.shape(), b.shape());
        if (!bshape)
            return std::unexpected(std::move(bshape.error()));

        auto out = Tensor::empty(*bshape, a.dtype(), a.device());
        if (!out)
            return std::unexpected(std::move(out.error()));

        if (a.device() == Device::CUDA) {
            auto r = run_binary_cuda(Op, a, b, *out, op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
            return *out;
        }

        dispatch_binary_cpu(Op, a, b, *out);
        return *out;
    }

    template <BinOp Op, typename Fn>
    auto inplace_binary_impl(Tensor& dst, const Tensor& src, std::string_view op_name, Fn check_dtype_fn) -> Result<void>
    {
        if (!dst.defined() || !src.defined())
            return std::unexpected(make_error(std::format("{}: Tensor 未定义", op_name), Severity::Recoverable));
        if (dst.device() != src.device())
            return std::unexpected(make_error(std::format("{}: device 不一致", op_name), Severity::Recoverable));

        {
            auto r = check_same_dtype(dst, src, op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }
        {
            auto r = check_dtype_fn(dst.dtype(), op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }

        auto bshape = compute_broadcast_shape(dst.shape(), src.shape());
        if (!bshape)
            return std::unexpected(std::move(bshape.error()));
        if (*bshape != dst.shape())
            return std::unexpected(make_error(
                std::format(
                    "{}: in-place 广播后 shape 不等于 dst.shape，"
                    "src 不能比 dst 大",
                    op_name
                ),
                Severity::Recoverable
            ));

        if (dst.device() == Device::CUDA) {
            return run_binary_cuda(Op, dst, src, dst, op_name);
        }

        dispatch_binary_cpu(Op, dst, src, dst);
        return {};
    }

} // namespace

auto add(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<BinOp::Add>(a, b, "add", [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto sub(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<BinOp::Sub>(a, b, "sub", [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto mul(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<BinOp::Mul>(a, b, "mul", [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

auto div(const Tensor& a, const Tensor& b) -> Result<Tensor>
{
    return binary_op_impl<BinOp::Div>(a, b, "div", [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

namespace
{
    template <UnOp Op, typename Fn>
    auto unary_op_impl(const Tensor& a, std::string_view op_name, Fn check_dtype_fn) -> Result<Tensor>
    {
        if (!a.defined())
            return std::unexpected(make_error(std::format("{}: Tensor 未定义", op_name), Severity::Recoverable));
        {
            auto r = check_dtype_fn(a.dtype(), op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
        }

        auto out = Tensor::empty(a.shape(), a.dtype(), a.device());
        if (!out)
            return std::unexpected(std::move(out.error()));

        if (a.device() == Device::CUDA) {
            auto r = run_unary_cuda(Op, a, *out, op_name);
            if (!r)
                return std::unexpected(std::move(r.error()));
            return *out;
        }

        dispatch_unary_cpu(Op, a, *out);
        return *out;
    }
} // namespace

auto neg(const Tensor& a) -> Result<Tensor>
{
    return unary_op_impl<UnOp::Neg>(a, "neg", [](DType dt, std::string_view op) { return check_dtype_negabs(dt, op); });
}

auto abs(const Tensor& a) -> Result<Tensor>
{
    return unary_op_impl<UnOp::Abs>(a, "abs", [](DType dt, std::string_view op) { return check_dtype_negabs(dt, op); });
}

auto sqrt(const Tensor& a) -> Result<Tensor>
{
    return unary_op_impl<UnOp::Sqrt>(a, "sqrt", [](DType dt, std::string_view op) { return check_dtype_float(dt, op); });
}

auto exp(const Tensor& a) -> Result<Tensor>
{
    return unary_op_impl<UnOp::Exp>(a, "exp", [](DType dt, std::string_view op) { return check_dtype_float(dt, op); });
}

auto log(const Tensor& a) -> Result<Tensor>
{
    return unary_op_impl<UnOp::Log>(a, "log", [](DType dt, std::string_view op) { return check_dtype_float(dt, op); });
}

auto add_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<BinOp::Add>(dst, src, "add_inplace", [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto sub_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<BinOp::Sub>(dst, src, "sub_inplace", [](DType dt, std::string_view op) { return check_dtype_addsub(dt, op); });
}

auto mul_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<BinOp::Mul>(dst, src, "mul_inplace", [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

auto div_inplace(Tensor& dst, const Tensor& src) -> Result<void>
{
    return inplace_binary_impl<BinOp::Div>(dst, src, "div_inplace", [](DType dt, std::string_view op) { return check_dtype_muldiv(dt, op); });
}

namespace
{
    auto inplace_unary_impl_negabs(Tensor& dst, std::string_view op_name) -> Result<void>
    {
        if (!dst.defined())
            return std::unexpected(make_error(std::format("{}: Tensor 未定义", op_name), Severity::Recoverable));
        return check_dtype_negabs(dst.dtype(), op_name);
    }

    auto inplace_unary_impl_float(Tensor& dst, std::string_view op_name) -> Result<void>
    {
        if (!dst.defined())
            return std::unexpected(make_error(std::format("{}: Tensor 未定义", op_name), Severity::Recoverable));
        return check_dtype_float(dst.dtype(), op_name);
    }

    auto run_inplace_unary(UnOp op, Tensor& dst, std::string_view op_name) -> Result<void>
    {
        if (dst.device() == Device::CUDA)
            return run_unary_cuda(op, dst, dst, op_name);
        dispatch_unary_cpu(op, dst, dst);
        return {};
    }
} // namespace

auto neg_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_negabs(dst, "neg_inplace");
    if (!r)
        return r;
    return run_inplace_unary(UnOp::Neg, dst, "neg_inplace");
}

auto abs_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_negabs(dst, "abs_inplace");
    if (!r)
        return r;
    return run_inplace_unary(UnOp::Abs, dst, "abs_inplace");
}

auto sqrt_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_float(dst, "sqrt_inplace");
    if (!r)
        return r;
    return run_inplace_unary(UnOp::Sqrt, dst, "sqrt_inplace");
}

auto exp_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_float(dst, "exp_inplace");
    if (!r)
        return r;
    return run_inplace_unary(UnOp::Exp, dst, "exp_inplace");
}

auto log_inplace(Tensor& dst) -> Result<void>
{
    auto r = inplace_unary_impl_float(dst, "log_inplace");
    if (!r)
        return r;
    return run_inplace_unary(UnOp::Log, dst, "log_inplace");
}

} // namespace bee
