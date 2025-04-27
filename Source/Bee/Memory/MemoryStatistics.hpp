/**
 * @File MemoryStatistics.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include <format>

namespace bee {
struct MemoryAllocationInfo
{
    const char* label = nullptr;
    const char* file  = nullptr;
    void* ptr         = nullptr;

    Size size         = 0;
    u32 line          = 0;
    MemoryUsage usage = MemoryUsage::Unknown;
};
} // namespace bee

template<> struct std::formatter<bee::MemoryAllocationInfo>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context> constexpr auto format(const bee::MemoryAllocationInfo& allocInfo, Context& ctx) const
    {
        // TODO
        return format_to(ctx.out(), "AllocInfo {}");
    }
};