#include "Tensor/Ops/Softmax.hpp"
#include "Tensor/Ops/Cast.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <format>
#include <algorithm>
#include <cmath>
#include <limits>

namespace bee
{

namespace
{

    auto normalize_dim(int dim, int64_t ndim) -> Result<int64_t>
    {
        int64_t d = static_cast<int64_t>(dim);
        if (d < 0)
            d += ndim;
        if (d < 0 || d >= ndim)
            return std::unexpected(make_error(std::format("softmax: dim={} 越界（ndim={}）", dim, ndim), Severity::Recoverable));
        return d;
    }

    auto check_softmax_dtype(DType dt) -> Result<void>
    {
        if (dt == DType::F32 || dt == DType::F64 || dt == DType::F16 || dt == DType::BF16)
            return {};
        return std::unexpected(make_error(std::format("softmax 不支持 DType::{}", enum_to_name(dt)), Severity::Recoverable));
    }

    template <typename T>
    auto softmax_cpu_impl(const Tensor& src, int64_t dim) -> Result<Tensor>
    {
        const auto&   shape = src.shape();
        const int64_t ndim  = src.ndim();

        auto out_r = Tensor::empty(shape, src.dtype(), Device::CPU);
        if (!out_r)
            return std::unexpected(std::move(out_r.error()));
        Tensor out = std::move(*out_r);

        const T* src_ptr = static_cast<const T*>(src.data_ptr());
        T*       out_ptr = static_cast<T*>(out.data_ptr());

        // 计算维度信息
        int64_t outer = 1;
        for (int64_t i = 0; i < dim; ++i)
            outer *= shape[static_cast<std::size_t>(i)];

        const int64_t axis_size = shape[static_cast<std::size_t>(dim)];

        int64_t inner = 1;
        for (int64_t i = dim + 1; i < ndim; ++i)
            inner *= shape[static_cast<std::size_t>(i)];

        // 对每个独立的切片执行 softmax
        for (int64_t o = 0; o < outer; ++o) {
            for (int64_t i = 0; i < inner; ++i) {
                const int64_t base = o * axis_size * inner + i;

                // 第一遍：找最大值
                T max_val = std::numeric_limits<T>::lowest();
                for (int64_t a = 0; a < axis_size; ++a) {
                    const int64_t idx = base + a * inner;
                    max_val           = std::max(max_val, src_ptr[idx]);
                }

                // 第二遍：计算 exp(x - max) 的和
                long double sum = 0.0L;
                for (int64_t a = 0; a < axis_size; ++a) {
                    const int64_t     idx      = base + a * inner;
                    const T           shifted  = src_ptr[idx] - max_val;
                    const long double exp_val  = std::exp(static_cast<long double>(shifted));
                    out_ptr[idx]               = static_cast<T>(exp_val);
                    sum                       += exp_val;
                }

                // 第三遍：归一化
                const T norm = static_cast<T>(sum);
                for (int64_t a = 0; a < axis_size; ++a) {
                    const int64_t idx  = base + a * inner;
                    out_ptr[idx]      /= norm;
                }
            }
        }

        return out;
    }

} // anonymous namespace

auto softmax(const Tensor& x, int dim, const tensor::cuda::ExecContext* ctx) -> Result<Tensor>
{
    // 前置校验
    if (!x.defined())
        return std::unexpected(make_error("softmax: 输入 Tensor 未定义", Severity::Recoverable));

    const int64_t ndim = x.ndim();
    if (ndim == 0)
        return std::unexpected(make_error("softmax: 输入必须至少为 1-D", Severity::Recoverable));

    // 归一化维度索引
    auto dim_r = normalize_dim(dim, ndim);
    if (!dim_r)
        return std::unexpected(std::move(dim_r.error()));
    const int64_t normalized_dim = *dim_r;

    // 检查 axis 维度大小
    const int64_t axis_size = x.shape()[static_cast<std::size_t>(normalized_dim)];
    if (axis_size <= 0)
        return std::unexpected(make_error(std::format("softmax: dim {} 的大小必须 > 0", dim), Severity::Recoverable));

    // dtype 检查
    auto dtype_r = check_softmax_dtype(x.dtype());
    if (!dtype_r)
        return std::unexpected(std::move(dtype_r.error()));

    // F16/BF16 提升为 F32
    if (x.dtype() == DType::F16 || x.dtype() == DType::BF16) {
        auto f32_r = cast(x, DType::F32, ctx);
        if (!f32_r)
            return std::unexpected(std::move(f32_r.error()));
        return softmax(*f32_r, dim, ctx);
    }

    // 非连续输入先整理为连续
    Tensor cont;
    if (!x.is_contiguous()) {
        auto r = x.contiguous(ctx);
        if (!r)
            return std::unexpected(std::move(r.error()));
        cont = std::move(*r);
    } else {
        cont = x;
    }

    if (cont.device() == Device::CUDA) {
        auto out_r = Tensor::empty(cont.shape(), cont.dtype(), Device::CUDA);
        if (!out_r)
            return std::unexpected(std::move(out_r.error()));
        Tensor out = std::move(*out_r);

        std::size_t outer = 1;
        for (int64_t i = 0; i < normalized_dim; ++i)
            outer *= static_cast<std::size_t>(cont.shape()[static_cast<std::size_t>(i)]);

        const std::size_t axis = static_cast<std::size_t>(axis_size);

        std::size_t inner = 1;
        for (int64_t i = normalized_dim + 1; i < ndim; ++i)
            inner *= static_cast<std::size_t>(cont.shape()[static_cast<std::size_t>(i)]);

        auto r = tensor::cuda::softmax(static_cast<int>(cont.dtype()), cont.data_ptr(), out.data_ptr(), outer, axis, inner);
        if (!r)
            return std::unexpected(std::move(r.error()));
        return out;
    }

    // CPU 实现：F32/F64
    if (cont.dtype() == DType::F32)
        return softmax_cpu_impl<float>(cont, normalized_dim);

    if (cont.dtype() == DType::F64)
        return softmax_cpu_impl<double>(cont, normalized_dim);

    return std::unexpected(make_error(std::format("softmax: 不支持的 dtype {}", enum_to_name(cont.dtype())), Severity::Recoverable));
}

} // namespace bee
