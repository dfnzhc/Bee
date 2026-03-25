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

inline void ThreadYield() noexcept
{
    std::this_thread::yield();
}

inline void ThreadPauseRelaxed() noexcept
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
                ThreadPauseRelaxed();
            }

            if ((spin_count & YieldMask) == YieldMask) {
                ThreadYield();
            }
        }
    };

    template <std::uint32_t PauseRepeats>
    struct ThroughputSpinPolicy
    {
        static void wait(std::uint32_t) noexcept
        {
            for (std::uint32_t i = 0; i < PauseRepeats; ++i) {
                ThreadPauseRelaxed();
            }
        }
    };

} // namespace internal

using DefaultSpinPolicy           = internal::AdaptiveSpinPolicy<internal::kSpinPauseRepeats, internal::kSpinYieldMask>;
using ThroughputDefaultSpinPolicy = internal::ThroughputSpinPolicy<(internal::kSpinPauseRepeats * 2U)>;

inline auto HardwareThreadCount() noexcept -> std::uint32_t
{
    const auto count = std::thread::hardware_concurrency();
    return count == 0 ? 1u : count;
}

inline auto ThreadIdHash() noexcept -> std::size_t
{
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

inline void SleepForNanos(std::uint64_t nanoseconds) noexcept
{
    std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
}

inline void SleepForMicros(std::uint64_t microseconds) noexcept
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

inline auto SetCurrentThreadName(std::string_view name) noexcept -> bool
{
    if (name.empty()) {
        return false;
    }

    #if defined(_WIN32)
    constexpr auto kMaxWideLength = 64u;
    wchar_t buffer[kMaxWideLength] = {};
    int length = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), buffer, static_cast<int>(kMaxWideLength - 1));
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
