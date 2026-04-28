#include "Tensor/Ops/Norm.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <format>
#include <cmath>

namespace bee
{

// ─────────────────────────────────────────────────────────────────────────────
// 内部辅助
// ─────────────────────────────────────────────────────────────────────────────

namespace
{

    // CPU 端 RMSNorm 内核：对最后一维归一化，再乘以 weight
    //   n_rows ：x 沿前面所有维展平后的行数（= x.numel() / d）
    //   d      ：最后一维大小（= weight.numel()）
    template <typename T>
    auto rms_norm_cpu_impl(const T* x_ptr, const T* w_ptr, T* out_ptr, int64_t n_rows, int64_t d, double eps) -> void
    {
        for (int64_t r = 0; r < n_rows; ++r) {
            const T* row_in  = x_ptr + r * d;
            T*       row_out = out_ptr + r * d;

            // 计算均方值
            double sum2 = 0.0;
            for (int64_t i = 0; i < d; ++i) {
                const double v  = static_cast<double>(row_in[i]);
                sum2           += v * v;
            }
            const double rms_inv = 1.0 / std::sqrt(sum2 / static_cast<double>(d) + eps);

            // 归一化并乘权重
            for (int64_t i = 0; i < d; ++i) {
                row_out[i] = static_cast<T>(static_cast<double>(row_in[i]) * rms_inv * static_cast<double>(w_ptr[i]));
            }
        }
    }

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// 公开接口
// ─────────────────────────────────────────────────────────────────────────────

auto rms_norm(const Tensor& x, const Tensor& weight, double eps, const tensor::cuda::ExecContext* /*ctx*/) -> Result<Tensor>
{
    // ── 前置校验 ──────────────────────────────────────────────────────────────
    if (!x.defined())
        return std::unexpected(make_error("rms_norm: x 未定义", Severity::Recoverable));
    if (!weight.defined())
        return std::unexpected(make_error("rms_norm: weight 未定义", Severity::Recoverable));

    if (x.dtype() != DType::F32 && x.dtype() != DType::F64)
        return std::unexpected(make_error(std::format("rms_norm: x dtype 须为 F32 或 F64，当前 {}", enum_to_name(x.dtype())), Severity::Recoverable));

    if (weight.dtype() != x.dtype())
        return std::unexpected(make_error(
            std::format("rms_norm: weight dtype（{}）须与 x dtype（{}）一致", enum_to_name(weight.dtype()), enum_to_name(x.dtype())),
            Severity::Recoverable
        ));

    if (x.ndim() < 1)
        return std::unexpected(make_error("rms_norm: x 须为至少 1-D 张量", Severity::Recoverable));

    if (weight.ndim() != 1)
        return std::unexpected(make_error(std::format("rms_norm: weight 须为 1-D 张量，当前 ndim={}", weight.ndim()), Severity::Recoverable));

    const int64_t d = x.shape().back();
    if (d <= 0)
        return std::unexpected(make_error(std::format("rms_norm: x 最后维须 > 0，当前 d={}", d), Severity::Recoverable));

    if (weight.numel() != d)
        return std::unexpected(
            make_error(std::format("rms_norm: weight 大小（{}）须与 x 最后维（{}）一致", weight.numel(), d), Severity::Recoverable)
        );

    if (!std::isfinite(eps) || eps <= 0.0)
        return std::unexpected(make_error(std::format("rms_norm: eps 须为有限正数，当前 {}", eps), Severity::Recoverable));

    // ── 设备一致性检查 ────────────────────────────────────────────────────────
    if (x.device() != weight.device())
        return std::unexpected(make_error(
            std::format(
                "rms_norm: x 与 weight 须在同一设备（{} vs {}）",
                x.device() == Device::CPU ? "CPU" : "CUDA",
                weight.device() == Device::CPU ? "CPU" : "CUDA"
            ),
            Severity::Recoverable
        ));

    // ── CUDA 原生路径 ─────────────────────────────────────────────────────────
    if (x.device() == Device::CUDA) {
        // 确保连续
        auto x_cont_r = x.contiguous();
        if (!x_cont_r)
            return std::unexpected(std::move(x_cont_r.error()));
        auto w_cont_r = weight.contiguous();
        if (!w_cont_r)
            return std::unexpected(std::move(w_cont_r.error()));

        const auto&   x_cont = *x_cont_r;
        const auto&   w_cont = *w_cont_r;
        const int64_t n_rows = x_cont.numel() / d;

        auto out = Tensor::empty(x_cont.shape(), x_cont.dtype(), Device::CUDA);
        if (!out)
            return std::unexpected(std::move(out.error()));

        BEE_TRY(
            tensor::cuda::rms_norm(
                static_cast<int>(x_cont.dtype()),
                x_cont.data_ptr(),
                w_cont.data_ptr(),
                out->data_ptr(),
                static_cast<std::size_t>(n_rows),
                static_cast<std::size_t>(d),
                eps
            )
        );
        return *out;
    }

    // ── CPU 路径 ──────────────────────────────────────────────────────────────
    // 确保连续
    Tensor x_cont = x;
    Tensor w_cont = weight;
    if (!x_cont.is_contiguous()) {
        auto r = x_cont.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        x_cont = std::move(*r);
    }
    if (!w_cont.is_contiguous()) {
        auto r = w_cont.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        w_cont = std::move(*r);
    }

    const int64_t n_rows = x_cont.numel() / d;
    auto          out    = Tensor::empty(x_cont.shape(), x_cont.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));

    if (x_cont.dtype() == DType::F32) {
        rms_norm_cpu_impl<float>(
            static_cast<const float*>(x_cont.data_ptr()),
            static_cast<const float*>(w_cont.data_ptr()),
            static_cast<float*>(out->data_ptr()),
            n_rows,
            d,
            eps
        );
    } else {
        rms_norm_cpu_impl<double>(
            static_cast<const double*>(x_cont.data_ptr()),
            static_cast<const double*>(w_cont.data_ptr()),
            static_cast<double*>(out->data_ptr()),
            n_rows,
            d,
            eps
        );
    }

    return *out;
}

} // namespace bee
