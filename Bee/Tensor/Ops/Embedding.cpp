#include "Tensor/Ops/Embedding.hpp"
#include "Tensor/Cuda/Backend.hpp"

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
    auto embedding_cpu_impl(const T* w_ptr, const void* ids_ptr, DType ids_dtype, T* out_ptr, int64_t n_ids, int64_t hidden, int64_t vocab)
        -> Result<void>
    {
        for (int64_t i = 0; i < n_ids; ++i) {
            const int64_t id = read_id(ids_ptr, i, ids_dtype);
            if (id < 0 || id >= vocab)
                return std::unexpected(make_error(std::format("embedding: id={} 越界（vocab={}）", id, vocab), Severity::Recoverable));
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

    if (weight.ndim() != 2)
        return std::unexpected(
            make_error(std::format("embedding: weight 须为 2-D [vocab, hidden]，当前 ndim={}", weight.ndim()), Severity::Recoverable)
        );

    if (weight.dtype() != DType::F32 && weight.dtype() != DType::F64)
        return std::unexpected(
            make_error(std::format("embedding: weight dtype 须为 F32 或 F64，当前 {}", enum_to_name(weight.dtype())), Severity::Recoverable)
        );

    if (token_ids.dtype() != DType::I32 && token_ids.dtype() != DType::I64)
        return std::unexpected(
            make_error(std::format("embedding: token_ids dtype 须为 I32 或 I64，当前 {}", enum_to_name(token_ids.dtype())), Severity::Recoverable)
        );

    // ── 设备一致性检查 ────────────────────────────────────────────────────────
    if (weight.device() != token_ids.device())
        return std::unexpected(make_error(
            std::format(
                "embedding: weight 与 token_ids 须在同一设备（{} vs {}）",
                weight.device() == Device::CPU ? "CPU" : "CUDA",
                token_ids.device() == Device::CPU ? "CPU" : "CUDA"
            ),
            Severity::Recoverable
        ));

    // ── CUDA 原生路径 ─────────────────────────────────────────────────────────
    if (weight.device() == Device::CUDA) {
        auto w_cont_r = weight.contiguous();
        if (!w_cont_r)
            return std::unexpected(std::move(w_cont_r.error()));
        auto ids_cont_r = token_ids.contiguous();
        if (!ids_cont_r)
            return std::unexpected(std::move(ids_cont_r.error()));

        const auto&   w_cont   = *w_cont_r;
        const auto&   ids_cont = *ids_cont_r;
        const int64_t vocab    = w_cont.shape()[0];
        const int64_t hidden   = w_cont.shape()[1];
        const int64_t n_ids    = ids_cont.numel();

        // 构建输出形状：token_ids.shape + [hidden]
        Shape out_shape = ids_cont.shape();
        out_shape.push_back(hidden);

        auto out = Tensor::empty(out_shape, w_cont.dtype(), Device::CUDA);
        if (!out)
            return std::unexpected(std::move(out.error()));

        BEE_TRY(
            tensor::cuda::embedding(
                static_cast<int>(w_cont.dtype()),
                static_cast<int>(ids_cont.dtype()),
                w_cont.data_ptr(),
                ids_cont.data_ptr(),
                out->data_ptr(),
                static_cast<std::size_t>(n_ids),
                static_cast<std::size_t>(hidden),
                static_cast<std::size_t>(vocab)
            )
        );
        return *out;
    }

    // ── CPU 路径 ──────────────────────────────────────────────────────────────
    // 确保连续
    Tensor w_cpu   = weight;
    Tensor ids_cpu = token_ids;
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
            n_ids,
            hidden,
            vocab
        );
    } else {
        r = embedding_cpu_impl<double>(
            static_cast<const double*>(w_cpu.data_ptr()),
            ids_cpu.data_ptr(),
            ids_cpu.dtype(),
            static_cast<double*>(out->data_ptr()),
            n_ids,
            hidden,
            vocab
        );
    }

    if (!r)
        return std::unexpected(std::move(r.error()));

    return *out;
}

} // namespace bee
