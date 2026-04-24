#include "Tensor/Ops/Random.hpp"
#include "Tensor/Cuda/Backend.hpp"

#include <format>
#include <random>
#include <cstdint>
#include <limits>

namespace bee
{

namespace
{

    auto make_engine(uint64_t seed) -> std::mt19937_64
    {
        if (seed == 0) {
            std::random_device rd;
            return std::mt19937_64{rd()};
        }
        return std::mt19937_64{seed};
    }

    // 对 seed=0 的 CUDA 路径派生非零种子。Philox 在 counter/key 全 0 时输出仍满足
    // 均匀性，但为与 CPU 行为一致（seed=0 表示"真随机"），派生一个随机 u64。
    auto resolve_cuda_seed(uint64_t seed) -> uint64_t
    {
        if (seed != 0)
            return seed;
        std::random_device rd;
        const uint64_t     hi = static_cast<uint64_t>(rd()) << 32;
        const uint64_t     lo = static_cast<uint64_t>(rd());
        const uint64_t     s  = hi | lo;
        return s == 0 ? 0x9E3779B97F4A7C15ull : s;
    }

} // namespace

auto rand(Shape shape, DType dtype, uint64_t seed, Device device) -> Result<Tensor>
{
    if (dtype != DType::F32 && dtype != DType::F64)
        return std::unexpected(make_error(std::format("rand: 不支持 DType::{}，仅允许 F32/F64", enum_to_name(dtype)), Severity::Recoverable));

    auto out_r = Tensor::empty(shape, dtype, device);
    if (!out_r)
        return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    const int64_t n = out.numel();
    if (n == 0)
        return out;

    if (device == Device::CUDA) {
        const uint64_t s = resolve_cuda_seed(seed);
        auto           r = tensor::cuda::random_uniform(static_cast<int>(dtype), out.data_ptr(), static_cast<std::size_t>(n), s);
        if (!r)
            return std::unexpected(std::move(r.error()));
        return out;
    }

    auto eng = make_engine(seed);

    if (dtype == DType::F32) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        auto*                                 ptr = static_cast<float*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        auto*                                  ptr = static_cast<double*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    }

    return out;
}

auto randn(Shape shape, DType dtype, uint64_t seed, Device device) -> Result<Tensor>
{
    if (dtype != DType::F32 && dtype != DType::F64)
        return std::unexpected(make_error(std::format("randn: 不支持 DType::{}，仅允许 F32/F64", enum_to_name(dtype)), Severity::Recoverable));

    auto out_r = Tensor::empty(shape, dtype, device);
    if (!out_r)
        return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    const int64_t n = out.numel();
    if (n == 0)
        return out;

    if (device == Device::CUDA) {
        const uint64_t s = resolve_cuda_seed(seed);
        auto           r = tensor::cuda::random_normal(static_cast<int>(dtype), out.data_ptr(), static_cast<std::size_t>(n), s);
        if (!r)
            return std::unexpected(std::move(r.error()));
        return out;
    }

    auto eng = make_engine(seed);

    if (dtype == DType::F32) {
        std::normal_distribution<float> dist(0.0f, 1.0f);
        auto*                           ptr = static_cast<float*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else {
        std::normal_distribution<double> dist(0.0, 1.0);
        auto*                            ptr = static_cast<double*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    }

    return out;
}

auto randint(int64_t low, int64_t high, Shape shape, DType dtype, uint64_t seed, Device device) -> Result<Tensor>
{
    if (low >= high)
        return std::unexpected(make_error(std::format("randint: low({}) >= high({})，区间为空", low, high), Severity::Recoverable));

    if (dtype != DType::I32 && dtype != DType::I64 && dtype != DType::U8)
        return std::unexpected(make_error(std::format("randint: 不支持 DType::{}，仅允许 I32/I64/U8", enum_to_name(dtype)), Severity::Recoverable));

    if (dtype == DType::U8 && low < 0)
        return std::unexpected(make_error(std::format("randint: dtype=U8 时要求 low >= 0，但 low={}", low), Severity::Recoverable));

    if (dtype == DType::I32) {
        constexpr int64_t kI32Min = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
        constexpr int64_t kI32Max = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
        if (low < kI32Min || high > (kI32Max + 1)) {
            return std::unexpected(
                make_error(std::format("randint: I32 范围要求 [{} , {}]（high 为开区间上界）", kI32Min, kI32Max + 1), Severity::Recoverable)
            );
        }
    }

    if (dtype == DType::U8) {
        if (high > 256) {
            return std::unexpected(make_error("randint: U8 范围要求 high <= 256（开区间上界）", Severity::Recoverable));
        }
    }

    auto out_r = Tensor::empty(shape, dtype, device);
    if (!out_r)
        return std::unexpected(std::move(out_r.error()));
    Tensor out = std::move(*out_r);

    const int64_t n = out.numel();
    if (n == 0)
        return out;

    if (device == Device::CUDA) {
        const uint64_t s = resolve_cuda_seed(seed);
        auto           r = tensor::cuda::random_int(static_cast<int>(dtype), out.data_ptr(), static_cast<std::size_t>(n), low, high, s);
        if (!r)
            return std::unexpected(std::move(r.error()));
        return out;
    }

    auto eng = make_engine(seed);

    if (dtype == DType::I32) {
        std::uniform_int_distribution<int32_t> dist(static_cast<int32_t>(low), static_cast<int32_t>(high) - 1);
        auto*                                  ptr = static_cast<int32_t*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else if (dtype == DType::I64) {
        std::uniform_int_distribution<int64_t> dist(low, high - 1);
        auto*                                  ptr = static_cast<int64_t*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = dist(eng);
    } else {
        std::uniform_int_distribution<uint16_t> dist(static_cast<uint16_t>(low), static_cast<uint16_t>(high) - 1u);
        auto*                                   ptr = static_cast<uint8_t*>(out.data_ptr());
        for (int64_t i = 0; i < n; ++i)
            ptr[i] = static_cast<uint8_t>(dist(eng));
    }

    return out;
}

} // namespace bee
