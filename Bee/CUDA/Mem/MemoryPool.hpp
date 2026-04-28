/**
 * @File Mem/MemoryPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Brief 基于 cudaMemPool_t + cudaMallocFromPoolAsync 的设备内存池（host only）。
 *
 * 设计要点：
 *  - 每个设备使用一个默认 mem pool，并把 release_threshold 调高以减少频繁
 *    向驱动归还内存造成的抖动；
 *  - 分配和释放都绑定到指定 stream，遵循 CUDA 的 stream-ordered 语义；
 *  - 调用方必须通过 stream 同步或 event 保证内存不再被后续工作访问。
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
    // 取指定设备的默认内存池；首次调用会设置 release_threshold=UINT64_MAX，
    // 让空闲块尽量保留在池中复用，直到显式 trim。
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

    // 按 stream 顺序提交分配请求。返回值是设备指针；nbytes 为 0 时返回空指针。
    [[nodiscard]] auto allocate_async(std::size_t nbytes, StreamView stream) -> Result<void*>
    {
        if (nbytes == 0)
            return static_cast<void*>(nullptr);
        void* ptr = nullptr;
        BEE_CUDA_CHECK(cudaMallocFromPoolAsync(&ptr, nbytes, pool_, stream.native_handle()));
        return ptr;
    }

    // 按 stream 顺序提交释放请求。调用方必须保证同一地址不会被仍在执行的
    // 其他 stream 继续访问。
    void deallocate_async(void* ptr, StreamView stream) noexcept
    {
        if (!ptr)
            return;
        (void)cudaFreeAsync(ptr, stream.native_handle());
    }

    // 将池中超过 keep_bytes 的空闲内存归还给驱动，用于降低长期驻留显存。
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
