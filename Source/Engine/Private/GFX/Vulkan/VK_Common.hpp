/**
 * @File VulkanCommon.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/12/3
 * @Brief This file is part of Bee.
 */

#pragma once

#include <volk.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <Utility/Error.hpp>
#include <Utility/Macros.hpp>

#include <any>

namespace bee {

template<int N = 15> class VulkanChainList
{
public:
    VulkanChainList() = default;
    
    BEE_PUSH_WARNING
    BEE_CLANG_DISABLE_WARNING("-Wsign-conversion")
    auto& insert(auto nextStruct)
    {
        BEE_DEBUG_ASSERT(_currentIndex < N, "VulkanChainList is full");
        _data[_currentIndex] = nextStruct;

        auto& next = std::any_cast<decltype(nextStruct)&>(_data[_currentIndex]);
        next.pNext = std::exchange(_head, &next);

        _currentIndex += 1;

        return next;
    }
    BEE_POP_WARNING

    [[nodiscard]] void* head() const { return _head; }

private:
    std::array<std::any, N> _data;
    void* _head       = nullptr;
    int _currentIndex = 0;
};

} // namespace bee