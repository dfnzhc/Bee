#include "Tensor/Ops/Embedding.hpp"

#include <format>
#include <cstring>
#include <cstdint>

namespace bee
{

// ─────────────────────────────────────────────────────────────────────────────
// 内部辅助
// ─────────────────────────────────────────────────────────────────────────────

namespace
{

    // 将任意整数张量中的元素以 int64_t 读取（支持 I32 / I64）
    auto read_id(const void* base_ptr, int64_t idx, DType dt) -> int64_t
    {
        if (dt == DType::I64)
            return static_cast<const int64_t*>(base_ptr)[idx];
        // I32
        return static_cast<int64_t>(static_cast<const int32_t*>(base_ptr)[idx]);
    }

    // CPU 端 embedding 核心：将 ids 对应的 weight 行拷贝到 out
    template <typename T>
    auto embedding_cpu_impl(
        const T*    w_ptr,
        const void* ids_ptr,
        DType       ids_dtype,
        T*          out_ptr,
        int64_t     n_ids,
        int64_t     hidden,
        int64_t     vocab
    ) -> Result<void>
    {
        for (int64_t i = 0; i < n_ids; ++i) {
            const int64_t id = read_id(ids_ptr, i, ids_dtype);
            if (id < 0 || id >= vocab)
                return std::unexpected(make_error(
                    std::format("embedding: id={} 越界（vocab={}）", id, vocab),
                    Severity::Recoverable
                ));
            std::memcpy(out_ptr + i * hidden, w_ptr + id * hidden, static_cast<std::size_t>(hidden) * sizeof(T));
        }
        return {};
    }

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// 公开接口
// ─────────────────────────────────────────────────────────────────────────────

auto embedding(const Tensor& weight, const Tensor& token_ids, const tensor::cuda::ExecContext* /*ctx*/) -> Result<Tensor>
{
    // ── 前置校验 ──────────────────────────────────────────────────────────────
    if (!weight.defined())
        return std::unexpected(make_error("embedding: weight 未定义", Severity::Recoverable));
    if (!token_ids.defined())
        return std::unexpected(make_error("embedding: token_ids 未定义", Severity::Recoverable));

    if (weight.ndim() < 2)
        return std::unexpected(make_error(
            std::format("embedding: weight 须至少为 2-D，当前 ndim={}", weight.ndim()),
            Severity::Recoverable
        ));

    if (weight.dtype() != DType::F32 && weight.dtype() != DType::F64)
        return std::unexpected(make_error(
            std::format("embedding: weight dtype 须为 F32 或 F64，当前 {}", enum_to_name(weight.dtype())),
            Severity::Recoverable
        ));

    if (token_ids.dtype() != DType::I32 && token_ids.dtype() != DType::I64)
        return std::unexpected(make_error(
            std::format("embedding: token_ids dtype 须为 I32 或 I64，当前 {}", enum_to_name(token_ids.dtype())),
            Severity::Recoverable
        ));

    // ── CUDA 过渡路径：迁移至 CPU 计算 ───────────────────────────────────────
    const Device orig_device = weight.device();
    Tensor       w_cpu       = weight;
    Tensor       ids_cpu     = token_ids;

    if (orig_device == Device::CUDA || token_ids.device() == Device::CUDA) {
        auto wr = weight.to(Device::CPU);
        if (!wr)
            return std::unexpected(std::move(wr.error()));
        w_cpu = std::move(*wr);

        auto ir = token_ids.to(Device::CPU);
        if (!ir)
            return std::unexpected(std::move(ir.error()));
        ids_cpu = std::move(*ir);
    }

    // ── 确保连续 ──────────────────────────────────────────────────────────────
    if (!w_cpu.is_contiguous()) {
        auto r = w_cpu.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        w_cpu = std::move(*r);
    }
    if (!ids_cpu.is_contiguous()) {
        auto r = ids_cpu.contiguous();
        if (!r)
            return std::unexpected(std::move(r.error()));
        ids_cpu = std::move(*r);
    }

    const int64_t vocab  = w_cpu.shape()[0];
    const int64_t hidden = w_cpu.shape()[1];
    const int64_t n_ids  = ids_cpu.numel();

    // ── 构建输出形状：token_ids.shape + [hidden] ──────────────────────────────
    Shape out_shape = ids_cpu.shape();
    out_shape.push_back(hidden);

    auto out = Tensor::empty(out_shape, w_cpu.dtype());
    if (!out)
        return std::unexpected(std::move(out.error()));

    // ── 执行 CPU 内核 ─────────────────────────────────────────────────────────
    Result<void> r;
    if (w_cpu.dtype() == DType::F32) {
        r = embedding_cpu_impl<float>(
            static_cast<const float*>(w_cpu.data_ptr()),
            ids_cpu.data_ptr(),
            ids_cpu.dtype(),
            static_cast<float*>(out->data_ptr()),
            n_ids, hidden, vocab
        );
    } else {
        r = embedding_cpu_impl<double>(
            static_cast<const double*>(w_cpu.data_ptr()),
            ids_cpu.data_ptr(),
            ids_cpu.dtype(),
            static_cast<double*>(out->data_ptr()),
            n_ids, hidden, vocab
        );
    }

    if (!r)
        return std::unexpected(std::move(r.error()));

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
