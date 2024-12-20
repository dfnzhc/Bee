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

class VulkanChainList
{
public:
    VulkanChainList() = default;
    
    void insert(auto& nextStruct)
    {
        nextStruct.pNext = std::exchange(_head, &nextStruct);
    }

    [[nodiscard]] void* head() const { return _head; }

private:
    void* _head       = nullptr;
};

} // namespace bee