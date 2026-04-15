#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <thread>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #include <immintrin.h>
#endif

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
    #include <pthread.h>
#endif

namespace bee
{

inline void thread_yield() noexcept
{
    std::this_thread::yield();
}

inline void thread_pause_relaxed() noexcept
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    _mm_pause();
#elif defined(__i386__) || defined(__x86_64__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    asm volatile("yield" ::: "memory");
#endif
}

namespace internal
{
#if defined(_WIN32)
    // Win 下减少调出
    constexpr std::uint32_t kSpinPauseRepeats = 8;
    constexpr std::uint32_t kSpinYieldMask    = 0x3FF;
#elif defined(__linux__)
    constexpr std::uint32_t kSpinPauseRepeats = 2;
    constexpr std::uint32_t kSpinYieldMask    = 0x7F;
#else
    constexpr std::uint32_t kSpinPauseRepeats = 1;
    constexpr std::uint32_t kSpinYieldMask    = 0x3F;
#endif

    template <std::uint32_t PauseRepeats, std::uint32_t YieldMask>
    struct AdaptiveSpinPolicy
    {
        static void wait(std::uint32_t spin_count) noexcept
        {
            for (std::uint32_t i = 0; i < PauseRepeats; ++i) {
                thread_pause_relaxed();
            }

            if ((spin_count & YieldMask) == YieldMask) {
                thread_yield();
            }
        }
    };

    template <std::uint32_t PauseRepeats>
    struct ThroughputSpinPolicy
    {
        static void wait(std::uint32_t) noexcept
        {
            for (std::uint32_t i = 0; i < PauseRepeats; ++i) {
                thread_pause_relaxed();
            }
        }
    };

} // namespace internal

// 默认自旋策略：一定次数等待后换出
using DefaultSpinPolicy = internal::AdaptiveSpinPolicy<internal::kSpinPauseRepeats, internal::kSpinYieldMask>;
// 吞吐量导向自旋策略：不进行换出
using ThroughputSpinPolicy = internal::ThroughputSpinPolicy<(internal::kSpinPauseRepeats * 2U)>;

template <typename SpinPolicy = DefaultSpinPolicy>
inline void try_contention(std::uint32_t& spin_count) noexcept
{
    // try 路径保持轻量退避，降低 CAS 热点冲突。
    // 设计取向：优先减少失败 CAS 洪泛，而非追求“永不让步”。
    SpinPolicy::wait(spin_count);
    ++spin_count;
}

template <typename SpinPolicy = DefaultSpinPolicy>
void adaptive_backoff(std::uint32_t& spin_count, std::uint32_t spin_limit, std::uint32_t yield_limit) noexcept
{
    // 分段退避：
    // 1) 前段以 pause 为主（低开销，低延迟）
    // 2) 中段插入少量 yield（降低争用）
    // 3) 后段以 yield 为主（避免长期空转）
    if (spin_count < spin_limit) {
        SpinPolicy::wait(spin_count);
    } else if (spin_count < yield_limit) {
        if ((spin_count & 0x07u) == 0x07u) {
            thread_yield();
        } else {
            SpinPolicy::wait(spin_count);
        }
    } else {
        thread_yield();
    }
    ++spin_count;
}

[[nodiscard]] inline auto hardware_thread_count() noexcept -> std::uint32_t
{
    const auto count = std::thread::hardware_concurrency();
    return count == 0 ? 1u : count;
}

[[nodiscard]] inline auto thread_id_hash() noexcept -> std::size_t
{
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

inline void sleep_for_nanos(std::uint64_t nanoseconds) noexcept
{
    std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
}

inline void sleep_for_micros(std::uint64_t microseconds) noexcept
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

inline auto set_current_thread_name(std::string_view name) noexcept -> bool
{
    if (name.empty()) {
        return false;
    }

#if defined(_WIN32)
    constexpr auto kMaxWideLength         = 64u;
    wchar_t        buffer[kMaxWideLength] = {};
    int            length = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), buffer, static_cast<int>(kMaxWideLength - 1));
    if (length <= 0) {
        return false;
    }
    buffer[length] = L'\0';
    return SUCCEEDED(SetThreadDescription(GetCurrentThread(), buffer));
#elif defined(__APPLE__)
    return pthread_setname_np(std::string(name).c_str()) == 0;
#elif defined(__linux__)
    return pthread_setname_np(pthread_self(), std::string(name).c_str()) == 0;
#else
    (void)name;
    return false;
#endif
}

} // namespace bee
