#include "Tensor/Ops/Loss.hpp"

#include "Base/Reflection/Enum.hpp"
#include "Tensor/Core/Tensor.hpp"
#include "Tensor/Ops/Cast.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>

namespace bee
{

namespace
{

    // 模板辅助函数：计算交叉熵
    template <typename FloatT, typename IntT>
    auto compute_cross_entropy_impl(const FloatT* logits_data, const IntT* target_data, size_t batch_size, size_t num_classes) -> Result<long double>
    {
        long double total_loss = 0.0L;

        for (size_t i = 0; i < batch_size; ++i) {
            const IntT label = target_data[i];

            // 验证标签范围
            if (label < 0 || static_cast<size_t>(label) >= num_classes) {
                return std::unexpected(
                    make_error(std::format("cross_entropy: target[{}] = {} 超出类别范围 [0, {})", i, label, num_classes), Severity::Recoverable)
                );
            }

            // 计算该样本的 max
            FloatT max_val = logits_data[i * num_classes];
            for (size_t j = 1; j < num_classes; ++j) {
                max_val = std::max(max_val, logits_data[i * num_classes + j]);
            }

            // 计算 log_sum_exp
            FloatT sum_exp = FloatT(0);
            for (size_t j = 0; j < num_classes; ++j) {
                sum_exp += std::exp(logits_data[i * num_classes + j] - max_val);
            }

            const FloatT loss_i  = std::log(sum_exp) + max_val - logits_data[i * num_classes + static_cast<size_t>(label)];
            total_loss          += static_cast<long double>(loss_i);
        }

        return total_loss;
    }

} // anonymous namespace

auto cross_entropy(const Tensor& logits, const Tensor& target, const tensor::cuda::ExecContext* ctx) -> Result<Tensor>
{
    // F16/BF16 提升为 F32
    if (logits.dtype() == DType::F16 || logits.dtype() == DType::BF16) {
        auto promoted = cast(logits, DType::F32, ctx);
        if (!promoted)
            return std::unexpected(promoted.error());
        return cross_entropy(promoted.value(), target, ctx);
    }

    // 验证 logits dtype
    if (logits.dtype() != DType::F32 && logits.dtype() != DType::F64) {
        return std::unexpected(
            make_error(std::format("cross_entropy: 不支持的 logits dtype: {}", enum_to_name(logits.dtype())), Severity::Recoverable)
        );
    }

    // 验证 target dtype
    if (target.dtype() != DType::I32 && target.dtype() != DType::I64) {
        return std::unexpected(
            make_error(std::format("cross_entropy: 不支持的 target dtype: {}", enum_to_name(target.dtype())), Severity::Recoverable)
        );
    }

    // 验证 shape
    if (logits.ndim() != 2) {
        return std::unexpected(make_error(std::format("cross_entropy: logits 必须是 2D，实际维度: {}", logits.ndim()), Severity::Recoverable));
    }

    if (target.ndim() != 1) {
        return std::unexpected(make_error(std::format("cross_entropy: target 必须是 1D，实际维度: {}", target.ndim()), Severity::Recoverable));
    }

    const auto N        = logits.shape()[0];
    const auto C        = logits.shape()[1];
    const auto target_N = target.shape()[0];

    if (N != target_N) {
        return std::unexpected(
            make_error(std::format("cross_entropy: logits 与 target batch 大小不匹配: {} vs {}", N, target_N), Severity::Recoverable)
        );
    }

    if (N == 0 || C == 0) {
        return std::unexpected(make_error("cross_entropy: N 和 C 必须大于 0", Severity::Recoverable));
    }

    // 验证设备
    if (logits.device() != target.device()) {
        const auto logits_dev = device_name(logits.device());
        const auto target_dev = device_name(target.device());
        return std::unexpected(
            make_error(std::format("cross_entropy: logits 与 target 设备不匹配: {} vs {}", logits_dev, target_dev), Severity::Recoverable)
        );
    }

    // CUDA 输入走 CPU bridge
    if (logits.device() == Device::CUDA) {
        auto logits_cpu = logits.to(Device::CPU, ctx);
        if (!logits_cpu)
            return std::unexpected(logits_cpu.error());

        auto target_cpu = target.to(Device::CPU, ctx);
        if (!target_cpu)
            return std::unexpected(target_cpu.error());

        auto loss_cpu = cross_entropy(logits_cpu.value(), target_cpu.value(), ctx);
        if (!loss_cpu)
            return std::unexpected(loss_cpu.error());

        return loss_cpu.value().to(Device::CUDA, ctx);
    }

    // 确保连续
    Tensor logits_contig = logits;
    if (!logits.is_contiguous()) {
        auto r = logits.contiguous();
        if (!r)
            return std::unexpected(r.error());
        logits_contig = r.value();
    }

    Tensor target_contig = target;
    if (!target.is_contiguous()) {
        auto r = target.contiguous();
        if (!r)
            return std::unexpected(r.error());
        target_contig = r.value();
    }

    // 计算 loss
    Result<long double> total_loss_result{std::unexpected(make_error("cross_entropy: 未初始化", Severity::Bug))};

    if (logits_contig.dtype() == DType::F32) {
        const auto* logits_data = static_cast<const float*>(logits_contig.data_ptr());

        if (target_contig.dtype() == DType::I64) {
            const auto* target_data = static_cast<const int64_t*>(target_contig.data_ptr());
            total_loss_result       = compute_cross_entropy_impl(logits_data, target_data, N, C);
        } else { // I32
            const auto* target_data = static_cast<const int32_t*>(target_contig.data_ptr());
            total_loss_result       = compute_cross_entropy_impl(logits_data, target_data, N, C);
        }

        if (!total_loss_result)
            return std::unexpected(total_loss_result.error());

        const float mean_loss = static_cast<float>(total_loss_result.value() / static_cast<long double>(N));
        return Tensor::full({}, DType::F32, mean_loss);

    } else { // F64
        const auto* logits_data = static_cast<const double*>(logits_contig.data_ptr());

        if (target_contig.dtype() == DType::I64) {
            const auto* target_data = static_cast<const int64_t*>(target_contig.data_ptr());
            total_loss_result       = compute_cross_entropy_impl(logits_data, target_data, N, C);
        } else { // I32
            const auto* target_data = static_cast<const int32_t*>(target_contig.data_ptr());
            total_loss_result       = compute_cross_entropy_impl(logits_data, target_data, N, C);
        }

        if (!total_loss_result)
            return std::unexpected(total_loss_result.error());

        const double mean_loss = static_cast<double>(total_loss_result.value() / static_cast<long double>(N));
        return Tensor::full({}, DType::F64, mean_loss);
    }
}

} // namespace bee
