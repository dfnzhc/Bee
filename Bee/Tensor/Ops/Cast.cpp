#include "Tensor/Ops/Cast.hpp"

#include <format>
#include <cstdint>

namespace bee
{

namespace
{

// CPU 类型转换内核：将 src 的 n 个 Src 类型元素逐一 static_cast 为 Dst 后写入 dst
template <typename Src, typename Dst>
auto cpu_cast_kernel(const void* src_data, void* dst_data, int64_t n) -> void
{
    const auto* s = static_cast<const Src*>(src_data);
    auto*       d = static_cast<Dst*>(dst_data);

    for (int64_t i = 0; i < n; ++i) {
        if constexpr (std::is_same_v<Dst, bool>) {
            // 任意类型转 Bool：非零为 true
            d[i] = (s[i] != Src{0});
        } else if constexpr (std::is_same_v<Src, bool>) {
            // Bool 转其它类型：true→1，false→0
            d[i] = static_cast<Dst>(s[i] ? Src{1} : Src{0});
        } else {
            d[i] = static_cast<Dst>(s[i]);
        }
    }
}

// 内层分派：已知 Src 类型，根据 dst_dtype 选择 Dst 并调用内核
template <typename Src>
auto dispatch_dst(DType dst_dtype, const void* src_data, void* dst_data, int64_t n) -> void
{
    switch (dst_dtype) {
    case DType::Bool: cpu_cast_kernel<Src, bool>    (src_data, dst_data, n); break;
    case DType::U8:   cpu_cast_kernel<Src, uint8_t> (src_data, dst_data, n); break;
    case DType::I32:  cpu_cast_kernel<Src, int32_t> (src_data, dst_data, n); break;
    case DType::I64:  cpu_cast_kernel<Src, int64_t> (src_data, dst_data, n); break;
    case DType::F32:  cpu_cast_kernel<Src, float>   (src_data, dst_data, n); break;
    case DType::F64:  cpu_cast_kernel<Src, double>  (src_data, dst_data, n); break;
    default: break;
    }
}

// 外层分派：根据 src_dtype 选择 Src，再调用内层分派
auto dispatch_cast(DType src_dtype, DType dst_dtype,
                   const void* src_data, void* dst_data, int64_t n) -> void
{
    switch (src_dtype) {
    case DType::Bool: dispatch_dst<bool>    (dst_dtype, src_data, dst_data, n); break;
    case DType::U8:   dispatch_dst<uint8_t> (dst_dtype, src_data, dst_data, n); break;
    case DType::I32:  dispatch_dst<int32_t> (dst_dtype, src_data, dst_data, n); break;
    case DType::I64:  dispatch_dst<int64_t> (dst_dtype, src_data, dst_data, n); break;
    case DType::F32:  dispatch_dst<float>   (dst_dtype, src_data, dst_data, n); break;
    case DType::F64:  dispatch_dst<double>  (dst_dtype, src_data, dst_data, n); break;
    default: break;
    }
}

} // namespace

auto cast(const Tensor& src, DType dst_dtype) -> Result<Tensor>
{
    // 前置校验
    if (!src.defined())
        return std::unexpected(make_error("cast: 输入 Tensor 未定义", Severity::Recoverable));
    if (src.device() == Device::CUDA)
        return std::unexpected(make_error("cast: CUDA 后端未实现", Severity::Recoverable));

    // 相同 dtype：返回独立 clone
    if (src.dtype() == dst_dtype)
        return src.clone();

    // 非连续时先整理为连续
    Tensor cont;
    if (!src.is_contiguous()) {
        auto r = src.contiguous();
        if (!r) return std::unexpected(std::move(r.error()));
        cont = std::move(*r);
    } else {
        cont = src;
    }

    // 分配目标 storage
    auto out_r = Tensor::empty(cont.shape(), dst_dtype, cont.device());
    if (!out_r) return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    // 执行类型转换
    dispatch_cast(cont.dtype(), dst_dtype,
                  cont.data_ptr(), out.data_ptr(),
                  cont.numel());

    return out;
}

} // namespace bee
