/**
 * @File BaseUtilsTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/18
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string_view>

#include "Base/Bit.hpp"
#include "Concurrency/Threading.hpp"

TEST(BaseThreadingTests, ThreadPauseIsCallable)
{
    for (std::uint32_t i = 0; i < 256; ++i) {
        bee::thread_pause_relaxed();
    }
    SUCCEED();
}

TEST(BaseThreadingTests, ThreadPauseVariantsAndContentionHelpersAreCallable)
{
    bee::thread_yield();
    bee::thread_pause_relaxed();
    
    bee::DefaultSpinPolicy::wait(1);
    SUCCEED();
    
    bee::ThroughputSpinPolicy::wait(1);
    SUCCEED();
}

TEST(BaseThreadingTests, HardwareThreadCountAndThreadIdHashAreAvailable)
{
    EXPECT_GE(bee::hardware_thread_count(), 1u);
    const auto id_hash = bee::thread_id_hash();
    EXPECT_EQ(id_hash, bee::thread_id_hash());
}

TEST(BaseThreadingTests, SleepUtilitiesAreCallable)
{
    const auto begin = std::chrono::steady_clock::now();
    bee::sleep_for_nanos(1000);
    bee::sleep_for_micros(1);
    const auto end = std::chrono::steady_clock::now();
    EXPECT_GE(end, begin);
}

TEST(BaseThreadingTests, SetCurrentThreadNameAcceptsNonEmptyName)
{
    EXPECT_FALSE(bee::set_current_thread_name(std::string_view{}));
    SUCCEED() << bee::set_current_thread_name("BaseThreadingTests");
}
