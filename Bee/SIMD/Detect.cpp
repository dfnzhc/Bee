// SIMD 组件实现：组件名 + 运行期 CPUID 检测
//
// 本文件**不**使用任何 SIMD intrinsics（只调用 CPUID / xgetbv），
// 因此**不需要**附加 ISA 编译标志，可以在任何目标机上安全执行。

#include "Detect.hpp"

#include <atomic>

#if defined(_MSC_VER)
#  include <intrin.h>
#  include <immintrin.h>
#endif

namespace bee::simd
{

namespace
{

#if defined(_MSC_VER)

// MSVC：用 __cpuid / __cpuidex + _xgetbv 读取
struct CpuidOut
{
    int eax;
    int ebx;
    int ecx;
    int edx;
};

auto call_cpuid(int leaf, int sub = 0) -> CpuidOut
{
    CpuidOut r{};
    int info[4] = {0, 0, 0, 0};
    __cpuidex(info, leaf, sub);
    r.eax = info[0];
    r.ebx = info[1];
    r.ecx = info[2];
    r.edx = info[3];
    return r;
}

auto xgetbv0() -> std::uint64_t
{
    return static_cast<std::uint64_t>(_xgetbv(0));
}

auto detect_impl() -> Isa
{
    // Leaf 0：最大可支持 leaf
    const auto leaf0 = call_cpuid(0);
    const int max_leaf = leaf0.eax;
    if (max_leaf < 1)
        return Isa::Scalar;

    const auto leaf1 = call_cpuid(1);
    const bool has_sse2   = (leaf1.edx & (1 << 26)) != 0;
    const bool has_sse41  = (leaf1.ecx & (1 << 19)) != 0;
    const bool has_osxsave = (leaf1.ecx & (1 << 27)) != 0;
    const bool has_avx    = (leaf1.ecx & (1 << 28)) != 0;

    // SSE2+SSE4.1 是 Bee::SIMD SSE2 后端的最低门槛
    const bool sse2_ok = has_sse2 && has_sse41;

    // 是否已由 OS 启用 AVX 的 YMM 状态（XCR0 bits 1 & 2）
    bool ymm_enabled = false;
    bool zmm_enabled = false;
    if (has_osxsave && has_avx) {
        const auto xcr0 = xgetbv0();
        ymm_enabled = (xcr0 & 0x6ULL) == 0x6ULL;
        // OPMASK (bit 5) + ZMM_Hi256 (bit 6) + Hi16_ZMM (bit 7)
        zmm_enabled = (xcr0 & 0xE6ULL) == 0xE6ULL;
    }

    bool has_avx2    = false;
    bool has_avx512f = false;
    bool has_avx512bw = false;
    if (max_leaf >= 7) {
        const auto leaf7 = call_cpuid(7, 0);
        has_avx2     = (leaf7.ebx & (1 << 5))  != 0;
        has_avx512f  = (leaf7.ebx & (1 << 16)) != 0;
        has_avx512bw = (leaf7.ebx & (1 << 30)) != 0;
    }

    if (has_avx512f && has_avx512bw && zmm_enabled)
        return Isa::Avx512;
    if (has_avx2 && ymm_enabled)
        return Isa::Avx2;
    if (sse2_ok)
        return Isa::Sse2;
    return Isa::Scalar;
}

#elif defined(__GNUC__) || defined(__clang__)

auto detect_impl() -> Isa
{
    __builtin_cpu_init();
    // AVX-512：同时需要 F + BW；编译器的 __builtin_cpu_supports 已包含 OS/XCR0 检查
    if (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw"))
        return Isa::Avx512;
    if (__builtin_cpu_supports("avx2"))
        return Isa::Avx2;
    if (__builtin_cpu_supports("sse4.1"))
        return Isa::Sse2;
    return Isa::Scalar;
}

#else

auto detect_impl() -> Isa
{
    return Isa::Scalar;
}

#endif

} // namespace

auto isa_name(Isa isa) noexcept -> const char*
{
    switch (isa) {
    case Isa::Avx512: return "Avx512";
    case Isa::Avx2:   return "Avx2";
    case Isa::Sse2:   return "Sse2";
    case Isa::Scalar: return "Scalar";
    }
    return "Unknown";
}

auto detect_isa() noexcept -> Isa
{
    // 函数局部 static 初始化在 C++11 起保证线程安全，且仅执行一次
    static const Isa kCached = detect_impl();
    return kCached;
}

// 保留原组件名函数，供诊断/测试引用
auto component_name() noexcept -> const char*
{
    return "SIMD";
}

} // namespace bee::simd
