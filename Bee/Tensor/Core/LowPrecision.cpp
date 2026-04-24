#include "Tensor/Core/LowPrecision.hpp"

#include <cstring>
#include <cstdint>

namespace bee
{

// ─── float → F16 ─────────────────────────────────────────────────────────────
// 按 IEEE 754 half-float 规范：1位符号 + 5位指数（偏置15）+ 10位尾数。
// 舍入策略：最近偶数（round-to-nearest-even）。
auto float_to_f16_bits(float value) noexcept -> std::uint16_t
{
    std::uint32_t f32;
    std::memcpy(&f32, &value, sizeof(f32));

    const std::uint32_t sign = (f32 >> 16) & 0x8000u;
    const std::uint32_t abs  = f32 & 0x7FFFFFFFu;

    // NaN → quiet NaN（保留符号位）
    if (abs > 0x7F800000u)
        return static_cast<std::uint16_t>(sign | 0x7E00u);

    // 无穷大或溢出 F16 最大值（65504）→ 无穷
    if (abs >= 0x47800000u)
        return static_cast<std::uint16_t>(sign | 0x7C00u);

    const int32_t  biased_exp = static_cast<int32_t>((f32 >> 23) & 0xFFu) - 127 + 15;
    const uint32_t mant       = f32 & 0x7FFFFFu;

    if (biased_exp <= 0) {
        // 次规格数或下溢
        if (biased_exp < -10)
            return static_cast<std::uint16_t>(sign); // 绝对值太小 → ±0
        // 次规格数：将隐含的 leading 1 加入尾数后右移
        const uint32_t shifted = (mant | 0x800000u) >> (1 - biased_exp);
        // 最近偶数舍入
        const uint32_t round_bit = (mant | 0x800000u) >> (-biased_exp) & 1u;
        const uint32_t sticky    = ((mant | 0x800000u) << (14 + biased_exp)) & 0x1FFFu;
        uint32_t       result    = (shifted >> 13);
        if (round_bit && (sticky || (result & 1u)))
            ++result;
        return static_cast<std::uint16_t>(sign | result);
    }

    // 规格数：截断 13 位尾数并最近偶数舍入
    const uint32_t truncated  = mant >> 13;
    const uint32_t round_half = mant & 0x1FFFu;
    uint32_t       exp_out    = static_cast<uint32_t>(biased_exp);
    uint32_t       mant_out   = truncated;

    if (round_half > 0x1000u || (round_half == 0x1000u && (truncated & 1u))) {
        ++mant_out;
        if (mant_out == 0x400u) {
            mant_out = 0;
            ++exp_out;
            if (exp_out >= 31u)
                return static_cast<std::uint16_t>(sign | 0x7C00u); // 溢出 → 无穷
        }
    }

    return static_cast<std::uint16_t>(sign | (exp_out << 10) | mant_out);
}

// ─── float → BF16 ────────────────────────────────────────────────────────────
// BF16 与 F32 共享指数位宽（8位），只截断低 16 位尾数，最近偶数舍入。
auto float_to_bf16_bits(float value) noexcept -> std::uint16_t
{
    std::uint32_t f32;
    std::memcpy(&f32, &value, sizeof(f32));

    // NaN → quiet NaN（置 mantissa bit 6）
    if ((f32 & 0x7FFFFFFFu) > 0x7F800000u)
        return static_cast<std::uint16_t>((f32 >> 16) | 0x0040u);

    // 最近偶数舍入：偏置 = 0x7FFF + 当前 bit16（即被舍去部分的最高位）
    const std::uint32_t rounding_bias = 0x7FFFu + ((f32 >> 16) & 1u);
    return static_cast<std::uint16_t>((f32 + rounding_bias) >> 16);
}

// ─── F16 → float ─────────────────────────────────────────────────────────────
auto f16_bits_to_float(std::uint16_t bits) noexcept -> float
{
    const std::uint32_t sign = static_cast<std::uint32_t>(bits & 0x8000u) << 16;
    const std::uint32_t exp  = (bits >> 10) & 0x1Fu;
    const std::uint32_t mant = bits & 0x3FFu;

    std::uint32_t f32;
    if (exp == 0u) {
        if (mant == 0u) {
            // 零（保留符号位）
            f32 = sign;
        } else {
            // 次规格数：规格化为 F32 规格数
            std::uint32_t m  = mant;
            std::uint32_t e  = 1u;
            while (!(m & 0x400u)) {
                m <<= 1;
                --e;
            }
            m &= 0x3FFu;
            f32 = sign | ((e + 127u - 15u) << 23) | (m << 13);
        }
    } else if (exp == 31u) {
        // 无穷大或 NaN
        f32 = sign | 0x7F800000u | (mant << 13);
    } else {
        // 规格数
        f32 = sign | ((exp + 127u - 15u) << 23) | (mant << 13);
    }

    float result;
    std::memcpy(&result, &f32, sizeof(result));
    return result;
}

// ─── BF16 → float ────────────────────────────────────────────────────────────
// 只需把 16 位值放到 F32 高 16 位，低 16 位补零。
auto bf16_bits_to_float(std::uint16_t bits) noexcept -> float
{
    const std::uint32_t f32 = static_cast<std::uint32_t>(bits) << 16;
    float               result;
    std::memcpy(&result, &f32, sizeof(result));
    return result;
}

} // namespace bee
