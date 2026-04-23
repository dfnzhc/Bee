#include "Tensor/Ops/Cast.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <format>
#include <cstdint>

namespace bee
{

auto cast(const Tensor& src, DType dst_dtype) -> Result<Tensor>
{
    // 前置校验
    if (!src.defined())
        return std::unexpected(make_error("cast: 输入 Tensor 未定义", Severity::Recoverable));

    // 相同 dtype：返回独立 clone
    if (src.dtype() == dst_dtype)
        return src.clone();

    // 非连续时先整理为连续
    Tensor cont;
    if (!src.is_contiguous()) {
        auto r = src.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        cont = std::move(*r);
    } else {
        cont = src;
    }

    // 分配目标 storage
    auto out_r = Tensor::empty(cont.shape(), dst_dtype, cont.device());
    if (!out_r)
        return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    if (cont.device() == Device::CUDA) {
        auto r = tensor::cuda::ew_cast(
            static_cast<int>(cont.dtype()), cont.data_ptr(), static_cast<int>(dst_dtype), out.data_ptr(), static_cast<std::size_t>(cont.numel())
        );
        if (!r)
            return std::unexpected(std::move(r.error()));
        return out;
    }

    // 执行类型转换（B11：通过 ISA 运行期分派走 SIMD + parallel_for）
    // 执行类型转换（B11：通过 ISA 运行期分派走 SIMD + parallel_for）
    BEE_RT_DISPATCH_STMT(ct_cast, cont.dtype(), dst_dtype, cont.data_ptr(), out.data_ptr(), cont.numel());

    return out;
}

} // namespace bee
