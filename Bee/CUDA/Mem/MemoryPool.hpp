/**
 * @File Mem/MemoryPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 基于 cudaMemPool_t + cudaMallocFromPoolAsync 的设备内存池（host only）。
 *
 * plan-cuda §5：
 *  - 每设备一个 mem pool（使用各设备的默认池，调整 release_threshold）；
 *  - 分配/释放都绑定到指定 stream（stream-ordered）；
 *  - 调用者负责在使用完成前 synchronize 或通过 event 等待。
 *
 * M1 只提供头与最小接口；真正的 Tensor 端接入在 M2（经由 Api.cpp::allocate）。
 */

#pragma once

#include <cuda_runtime.h>

#include <cstddef>
#include <mutex>
#include <unordered_map>

#include "Base/Diagnostics/Error.hpp"
#include "CUDA/Core/Check.hpp"
#include "CUDA/Core/Stream.hpp"

namespace bee::cuda
{

class MemoryPool
{
public:
    // 取指定设备的默认内存池（首次调用会设置 release_threshold=UINT64_MAX 以减少归还）。
    [[nodiscard]] static auto get_default(int device_index) -> Result<MemoryPool*>
    {
        auto&           inst = instance();
        std::lock_guard lk(inst.mutex_);

        auto it = inst.pools_.find(device_index);
        if (it != inst.pools_.end())
            return &it->second;

        cudaMemPool_t pool = nullptr;
        BEE_CUDA_CHECK(cudaDeviceGetDefaultMemPool(&pool, device_index));

        // 避免频繁归还：保留池化的内存直到显式 trim。
        std::uint64_t threshold = ~std::uint64_t{0};
        BEE_CUDA_CHECK(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &threshold));

        auto [ins_it, ok] = inst.pools_.emplace(device_index, MemoryPool{pool, device_index});
        (void)ok;
        return &ins_it->second;
    }

    // Stream-ordered 分配。调用前确保 stream 有效；返回 device 指针。
    [[nodiscard]] auto allocate_async(std::size_t nbytes, StreamView stream) -> Result<void*>
    {
        if (nbytes == 0)
            return static_cast<void*>(nullptr);
        void* ptr = nullptr;
        BEE_CUDA_CHECK(cudaMallocFromPoolAsync(&ptr, nbytes, pool_, stream.native_handle()));
        return ptr;
    }

    // Stream-ordered 释放。释放在 stream 上的后续工作可能仍需该地址；需调用方保证时序。
    void deallocate_async(void* ptr, StreamView stream) noexcept
    {
        if (!ptr)
            return;
        (void)cudaFreeAsync(ptr, stream.native_handle());
    }

    // 将池中空闲内存归还给 OS/驱动（high-watermark 维护）。
    [[nodiscard]] auto trim(std::size_t keep_bytes = 0) -> Result<void>
    {
        BEE_CUDA_CHECK(cudaMemPoolTrimTo(pool_, keep_bytes));
        return {};
    }

    [[nodiscard]] cudaMemPool_t native_handle() const noexcept
    {
        return pool_;
    }
    [[nodiscard]] int device_index() const noexcept
    {
        return device_;
    }

private:
    MemoryPool() = default;
    MemoryPool(cudaMemPool_t p, int dev) noexcept
        : pool_(p)
        , device_(dev)
    {
    }

    struct Registry
    {
        std::mutex                          mutex_;
        std::unordered_map<int, MemoryPool> pools_;
    };

    static Registry& instance()
    {
        static Registry r;
        return r;
    }

    cudaMemPool_t pool_   = nullptr;
    int           device_ = -1;
};

} // namespace bee::cuda
