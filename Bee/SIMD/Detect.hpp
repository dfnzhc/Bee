#pragma once

#include <cstdint>

namespace bee::simd
{

// 运行期 ISA 枚举。
//
// 枚举值按能力从低到高排列，便于调用方用大小关系判断“至少支持某个
// ISA”。这里描述的是 Bee::SIMD 当前选择使用的最高后端，不代表 CPU
// 支持的所有扩展集合；例如 SSE2 后端在实现上还依赖 SSE4.1 的部分指令。
enum class Isa : std::uint8_t
{
    Scalar = 0,
    Sse2   = 1,
    Avx2   = 2,
    Avx512 = 3,
};

// 返回枚举对应的人类可读名称，主要用于诊断日志与测试输出。
auto isa_name(Isa isa) noexcept -> const char*;

// 检测当前进程可安全使用的最高 SIMD 后端。
//
// 该函数同时考虑 CPU 指令集与操作系统保存寄存器状态的能力。首次调用
// 会执行 CPUID/XCR0 探测并缓存结果，后续调用直接返回缓存值；函数局部
// static 初始化保证多线程首次调用时的线程安全。
auto detect_isa() noexcept -> Isa;

// detect_isa() 的语义别名，用于强调“当前进程建议选择的最优后端”。
inline auto current_isa() noexcept -> Isa
{
    return detect_isa();
}

} // namespace bee::simd
