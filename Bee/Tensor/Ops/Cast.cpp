#include "Tensor/Ops/Cast.hpp"
#include "Tensor/Core/LowPrecision.hpp"
#include "Tensor/Cpu/Dispatch/Dispatch.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <format>
#include <cstdint>

namespace bee
{

auto cast(const Tensor& src, DType dst_dtype, const tensor::cuda::ExecContext* /*ctx*/) -> Result<Tensor>
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

    // CPU F16/BF16 互转路径（不走 ISA 分派，直接用位编码辅助函数）
    const int64_t     n       = cont.numel();
    const DType       src_dt  = cont.dtype();
    const void*       src_ptr = cont.data_ptr();
    void*             dst_ptr = out.data_ptr();

    if (src_dt == DType::F32 && dst_dtype == DType::F16) {
        const auto* s = static_cast<const float*>(src_ptr);
        auto*       d = static_cast<std::uint16_t*>(dst_ptr);
        for (int64_t i = 0; i < n; ++i)
            d[i] = float_to_f16_bits(s[i]);
        return out;
    }
    if (src_dt == DType::F16 && dst_dtype == DType::F32) {
        const auto* s = static_cast<const std::uint16_t*>(src_ptr);
        auto*       d = static_cast<float*>(dst_ptr);
        for (int64_t i = 0; i < n; ++i)
            d[i] = f16_bits_to_float(s[i]);
        return out;
    }
    if (src_dt == DType::F32 && dst_dtype == DType::BF16) {
        const auto* s = static_cast<const float*>(src_ptr);
        auto*       d = static_cast<std::uint16_t*>(dst_ptr);
        for (int64_t i = 0; i < n; ++i)
            d[i] = float_to_bf16_bits(s[i]);
        return out;
    }
    if (src_dt == DType::BF16 && dst_dtype == DType::F32) {
        const auto* s = static_cast<const std::uint16_t*>(src_ptr);
        auto*       d = static_cast<float*>(dst_ptr);
        for (int64_t i = 0; i < n; ++i)
            d[i] = bf16_bits_to_float(s[i]);
        return out;
    }

    // CPU 路径：通过运行期 ISA 分派进入 B11 的 SIMD + parallel_for cast 内核。
    BEE_RT_DISPATCH_STMT(ct_cast, cont.dtype(), dst_dtype, cont.data_ptr(), out.data_ptr(), cont.numel());

    return out;
}

} // namespace bee
