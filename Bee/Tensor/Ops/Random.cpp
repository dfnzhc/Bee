#include "Tensor/Ops/Random.hpp"

#include <format>
#include <random>
#include <cstdint>

namespace bee
{

namespace
{

// 生成随机引擎：seed==0 时使用 std::random_device 产生不固定种子
auto make_engine(uint64_t seed) -> std::mt19937_64
{
    if (seed == 0) {
        std::random_device rd;
        return std::mt19937_64{rd()};
    }
    return std::mt19937_64{seed};
}

} // namespace

auto rand(Shape shape, DType dtype, uint64_t seed, Device device) -> Result<Tensor>
{
    // 仅支持浮点 dtype
    if (dtype != DType::F32 && dtype != DType::F64)
        return std::unexpected(make_error(
            std::format("rand: 不支持 DType::{}，仅允许 F32/F64", dtype_name(dtype)),
            Severity::Recoverable));

    // 分配 storage（CUDA 由 empty 挡住）
    auto out_r = Tensor::empty(shape, dtype, device);
    if (!out_r) return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    auto eng = make_engine(seed);

    const int64_t n = out.numel();

    if (dtype == DType::F32) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        auto* ptr = static_cast<float*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        auto* ptr = static_cast<double*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    }

    return out;
}

auto randn(Shape shape, DType dtype, uint64_t seed, Device device) -> Result<Tensor>
{
    // 仅支持浮点 dtype
    if (dtype != DType::F32 && dtype != DType::F64)
        return std::unexpected(make_error(
            std::format("randn: 不支持 DType::{}，仅允许 F32/F64", dtype_name(dtype)),
            Severity::Recoverable));

    auto out_r = Tensor::empty(shape, dtype, device);
    if (!out_r) return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    auto eng = make_engine(seed);

    const int64_t n = out.numel();

    if (dtype == DType::F32) {
        std::normal_distribution<float> dist(0.0f, 1.0f);
        auto* ptr = static_cast<float*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else {
        std::normal_distribution<double> dist(0.0, 1.0);
        auto* ptr = static_cast<double*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    }

    return out;
}

auto randint(int64_t low, int64_t high, Shape shape, DType dtype, uint64_t seed,
             Device device) -> Result<Tensor>
{
    // 范围校验
    if (low >= high)
        return std::unexpected(make_error(
            std::format("randint: low({}) >= high({})，区间为空", low, high),
            Severity::Recoverable));

    // dtype 校验
    if (dtype != DType::I32 && dtype != DType::I64 && dtype != DType::U8)
        return std::unexpected(make_error(
            std::format("randint: 不支持 DType::{}，仅允许 I32/I64/U8", dtype_name(dtype)),
            Severity::Recoverable));

    // U8 时要求 low >= 0
    if (dtype == DType::U8 && low < 0)
        return std::unexpected(make_error(
            std::format("randint: dtype=U8 时要求 low >= 0，但 low={}", low),
            Severity::Recoverable));

    auto out_r = Tensor::empty(shape, dtype, device);
    if (!out_r) return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    auto eng = make_engine(seed);

    const int64_t n = out.numel();

    // 分派不同整数类型的分布
    if (dtype == DType::I32) {
        std::uniform_int_distribution<int32_t> dist(
            static_cast<int32_t>(low), static_cast<int32_t>(high) - 1);
        auto* ptr = static_cast<int32_t*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else if (dtype == DType::I64) {
        std::uniform_int_distribution<int64_t> dist(low, high - 1);
        auto* ptr = static_cast<int64_t*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else {
        // U8
        std::uniform_int_distribution<uint16_t> dist(
            static_cast<uint16_t>(low), static_cast<uint16_t>(high) - 1u);
        auto* ptr = static_cast<uint8_t*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = static_cast<uint8_t>(dist(eng));
    }

    return out;
}

} // namespace bee
