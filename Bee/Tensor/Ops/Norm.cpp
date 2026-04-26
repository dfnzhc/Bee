#include "Tensor/Ops/Norm.hpp"

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

    // ── CUDA 过渡路径 ─────────────────────────────────────────────────────────
    const Device orig_device = x.device();
    Tensor       x_cpu       = x;
    Tensor       w_cpu       = weight;

    if (orig_device == Device::CUDA) {
        auto xr = x.to(Device::CPU);
        if (!xr)
            return std::unexpected(std::move(xr.error()));
        x_cpu = std::move(*xr);

        auto wr = weight.to(Device::CPU);
        if (!wr)
            return std::unexpected(std::move(wr.error()));
        w_cpu = std::move(*wr);
    }

    // ── 确保连续 ──────────────────────────────────────────────────────────────
    if (!x_cpu.is_contiguous()) {
        auto r = x_cpu.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        x_cpu = std::move(*r);
    }
    if (!w_cpu.is_contiguous()) {
        auto r = w_cpu.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        w_cpu = std::move(*r);
    }

    const int64_t n_rows = x_cpu.numel() / d;

    auto out = Tensor::empty(x_cpu.shape(), x_cpu.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));

    // ── 执行 CPU 内核 ─────────────────────────────────────────────────────────
    if (x_cpu.dtype() == DType::F32) {
        rms_norm_cpu_impl<float>(
            static_cast<const float*>(x_cpu.data_ptr()),
            static_cast<const float*>(w_cpu.data_ptr()),
            static_cast<float*>(out->data_ptr()),
            n_rows,
            d,
            eps
        );
    } else {
        rms_norm_cpu_impl<double>(
            static_cast<const double*>(x_cpu.data_ptr()),
            static_cast<const double*>(w_cpu.data_ptr()),
            static_cast<double*>(out->data_ptr()),
            n_rows,
            d,
            eps
        );
    }

    // ── 若原设备为 CUDA，将结果迁回 ──────────────────────────────────────────
    if (orig_device == Device::CUDA) {
        auto back = out->to(Device::CUDA);
        if (!back)
            return std::unexpected(std::move(back.error()));
        return *back;
    }

    return *out;
}

} // namespace bee
