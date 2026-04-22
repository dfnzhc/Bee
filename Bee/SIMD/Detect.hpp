#pragma once

#include <cstdint>

namespace bee::simd
{

// 运行期 ISA 枚举：数值越大表示越宽
enum class Isa : std::uint8_t
{
    Scalar = 0,
    Sse2   = 1,
    Avx2   = 2,
    Avx512 = 3,
};

// 返回枚举对应的人类可读名字
auto isa_name(Isa isa) noexcept -> const char*;

// 检测当前 CPU 支持的最高 ISA（线程安全，仅在首次调用时执行 CPUID，
// 后续调用直接返回缓存值）
auto detect_isa() noexcept -> Isa;

// 等价于 detect_isa()，命名上更贴近"当前最优 ISA"的语义
inline auto current_isa() noexcept -> Isa
{
    return detect_isa();
}

} // namespace bee::simd
