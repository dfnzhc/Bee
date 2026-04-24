#pragma once

// 低精度（F16/BF16）位编码/解码辅助函数声明。
// - F16：IEEE 754 半精度（1位符号 + 5位指数 + 10位尾数）。
// - BF16：Brain Float 16（1位符号 + 8位指数 + 7位尾数），与 F32 共享指数范围。
// - 所有函数均为 CPU 参考实现，用于 cast 与 full() 的 F16/BF16 编码支持。

#include <cstdint>

namespace bee
{

// float → F16 位编码（IEEE 754 half-float，最近偶数舍入）
[[nodiscard]] auto float_to_f16_bits(float value) noexcept -> std::uint16_t;

// float → BF16 位编码（取 F32 高 16 位，最近偶数舍入）
[[nodiscard]] auto float_to_bf16_bits(float value) noexcept -> std::uint16_t;

// F16 位编码 → float
[[nodiscard]] auto f16_bits_to_float(std::uint16_t bits) noexcept -> float;

// BF16 位编码 → float
[[nodiscard]] auto bf16_bits_to_float(std::uint16_t bits) noexcept -> float;

} // namespace bee
