/**
 * @File AllocStats.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/23
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Core/Defines.hpp"

#include <atomic>

namespace bee {
/**
 * Structure to hold allocation statistics
 */
struct BEE_API AllocStats
{
    /** Total bytes ever requested from this source (cumulative) */
    std::atomic<Size> totalAllocatedBytes = 0;

    /** Current number of bytes allocated and not yet deallocated from this source */
    std::atomic<Size> currentUsageBytes = 0;

    /** High watermark of currentUsageBytes */
    std::atomic<Size> peakUsageBytes = 0;

    /** Total number of successful allocation calls */
    std::atomic<Size> allocationCount = 0;

    /** Total number of successful deallocation calls */
    std::atomic<Size> deallocationCount = 0;

    AllocStats() = default;

    AllocStats(const AllocStats& other) :
        totalAllocatedBytes(other.totalAllocatedBytes.load(std::memory_order_relaxed)),
        currentUsageBytes(other.currentUsageBytes.load(std::memory_order_relaxed)),
        peakUsageBytes(other.peakUsageBytes.load(std::memory_order_relaxed)),
        allocationCount(other.allocationCount.load(std::memory_order_relaxed)),
        deallocationCount(other.deallocationCount.load(std::memory_order_relaxed))
    {
    }

    AllocStats& operator=(const AllocStats& other)
    {
        if (this != &other) {
            totalAllocatedBytes.store(other.totalAllocatedBytes.load(std::memory_order_relaxed), std::memory_order_relaxed);
            currentUsageBytes.store(other.currentUsageBytes.load(std::memory_order_relaxed), std::memory_order_relaxed);
            peakUsageBytes.store(other.peakUsageBytes.load(std::memory_order_relaxed), std::memory_order_relaxed);
            allocationCount.store(other.allocationCount.load(std::memory_order_relaxed), std::memory_order_relaxed);
            deallocationCount.store(other.deallocationCount.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    void alloc(Size size) noexcept
    {
        // update the stats
        allocationCount.fetch_add(1, std::memory_order_relaxed);
        totalAllocatedBytes.fetch_add(size, std::memory_order_relaxed);

        Size current_usage = currentUsageBytes.fetch_add(size, std::memory_order_relaxed) + size;

        // Update peak usage safely
        Size current_peak = peakUsageBytes.load(std::memory_order_relaxed);
        while (current_usage > current_peak) {
            if (peakUsageBytes.compare_exchange_weak(current_peak,
                                                     current_usage,
                                                     std::memory_order_relaxed,
                                                     std::memory_order_relaxed)) {
                // Successfully updated
                break;
            }
        }
    }

    void dealloc(Size size) noexcept
    {
        currentUsageBytes.fetch_sub(size, std::memory_order_relaxed);
        deallocationCount.fetch_add(1, std::memory_order_relaxed);
    }
};

} // namespace bee