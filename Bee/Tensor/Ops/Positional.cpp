#include "Tensor/Ops/Positional.hpp"

#include <format>
#include <cmath>

namespace bee
{

// ─────────────────────────────────────────────────────────────────────────────
// 内部辅助
// ─────────────────────────────────────────────────────────────────────────────

namespace
{

    // CPU 端 RoPE 内核
    //   x_ptr          ：输入数据指针（连续，形状 [..., seq_len, dim]）
    //   out_ptr        ：输出数据指针（同 shape）
    //   n_batch        ：批次大小（展平 [...] 维度后的数量）
    //   seq_len        ：倒数第二维大小
    //   dim            ：最后维大小（须为偶数）
    //   base           ：RoPE 基数（通常 10000.0）
    //   position_offset：序列偏移
    template <typename T>
    auto apply_rope_cpu_impl(
        const T* x_ptr,
        T*       out_ptr,
        int64_t  n_batch,
        int64_t  seq_len,
        int64_t  dim,
        double   base,
        int64_t  position_offset
    ) -> void
    {
        const int64_t half_dim = dim / 2;

        for (int64_t b = 0; b < n_batch; ++b) {
            for (int64_t s = 0; s < seq_len; ++s) {
                // 当前 token 的绝对位置
                const double pos = static_cast<double>(position_offset + s);

                // 指向当前行的指针
                const T* in_row  = x_ptr   + (b * seq_len + s) * dim;
                T*       out_row = out_ptr + (b * seq_len + s) * dim;

                for (int64_t i = 0; i < half_dim; ++i) {
                    // theta_i = pos / base^(2i / dim)
                    const double theta = pos / std::pow(base, 2.0 * static_cast<double>(i) / static_cast<double>(dim));

                    const double cos_t = std::cos(theta);
                    const double sin_t = std::sin(theta);

                    const double x0 = static_cast<double>(in_row[i]);
                    const double x1 = static_cast<double>(in_row[i + half_dim]);

                    // 旋转变换：(x0, x1) → (x0*cos - x1*sin, x0*sin + x1*cos)
                    out_row[i]            = static_cast<T>(x0 * cos_t - x1 * sin_t);
                    out_row[i + half_dim] = static_cast<T>(x0 * sin_t + x1 * cos_t);
                }
            }
        }
    }

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// 公开接口
// ─────────────────────────────────────────────────────────────────────────────

auto apply_rope(const Tensor& x, double base, int64_t position_offset, const tensor::cuda::ExecContext* /*ctx*/) -> Result<Tensor>
{
    // ── 前置校验 ──────────────────────────────────────────────────────────────
    if (!x.defined())
        return std::unexpected(make_error("apply_rope: x 未定义", Severity::Recoverable));

    if (x.dtype() != DType::F32 && x.dtype() != DType::F64)
        return std::unexpected(make_error(
            std::format("apply_rope: dtype 须为 F32 或 F64，当前 {}", enum_to_name(x.dtype())),
            Severity::Recoverable
        ));

    if (x.ndim() < 2)
        return std::unexpected(make_error(
            std::format("apply_rope: x 须至少为 2-D，当前 ndim={}", x.ndim()),
            Severity::Recoverable
        ));

    const int64_t dim = x.shape().back();
    if (dim % 2 != 0)
        return std::unexpected(make_error(
            std::format("apply_rope: 最后维须为偶数，当前 dim={}", dim),
            Severity::Recoverable
        ));

    if (base <= 0.0)
        return std::unexpected(make_error(
            std::format("apply_rope: base 须 > 0，当前 {}", base),
            Severity::Recoverable
        ));

    // ── CUDA 过渡路径 ─────────────────────────────────────────────────────────
    const Device orig_device = x.device();
    Tensor       x_cpu       = x;

    if (orig_device == Device::CUDA) {
        auto r = x.to(Device::CPU);
        if (!r)
            return std::unexpected(std::move(r.error()));
        x_cpu = std::move(*r);
    }

    // ── 确保连续 ──────────────────────────────────────────────────────────────
    if (!x_cpu.is_contiguous()) {
        auto r = x_cpu.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        x_cpu = std::move(*r);
    }

    // ── 计算维度参数 ──────────────────────────────────────────────────────────
    const int64_t seq_len = x_cpu.shape()[static_cast<std::size_t>(x_cpu.ndim() - 2)];
    // n_batch = numel / (seq_len * dim)
    const int64_t n_batch = x_cpu.numel() / (seq_len * dim);

    auto out = Tensor::empty(x_cpu.shape(), x_cpu.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));

    // ── 执行 CPU 内核 ─────────────────────────────────────────────────────────
    if (x_cpu.dtype() == DType::F32) {
        apply_rope_cpu_impl<float>(
            static_cast<const float*>(x_cpu.data_ptr()),
            static_cast<float*>(out->data_ptr()),
            n_batch, seq_len, dim, base, position_offset
        );
    } else {
        apply_rope_cpu_impl<double>(
            static_cast<const double*>(x_cpu.data_ptr()),
            static_cast<double*>(out->data_ptr()),
            n_batch, seq_len, dim, base, position_offset
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
