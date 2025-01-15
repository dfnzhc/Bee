/**
 * @File VK_Debug.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/1/6
 * @Brief This file is part of Bee.
 */

#pragma once

#include "Graphics/Vulkan/VK_Common.hpp"

namespace bee {
class BEE_API VK_Debug
{
public:
    VK_Debug() = default;

    VK_Debug(vk::Device device) : _device(device)
    {
    }

    static void setEnabled(bool state) { sEnabled = state; }

    static bool isEnabled() { return sEnabled; }

    void setup(vk::Device device) { _device = device; }

    void setObjectName(const u64 object, const std::string& name, vk::ObjectType type) const
    {
        if (sEnabled) {
            const vk::DebugUtilsObjectNameInfoEXT nameInfo{type, object, name.c_str()};
            _device.setDebugUtilsObjectNameEXT(nameInfo);
        }
    }

    template<class T> using CTypeVK = typename T::NativeType;
    template<typename VK_T> static u64 objectHandle(VK_T object) { return u64((CTypeVK<VK_T>)(object)); }

#define DEFINE_SET_OBJECT_NAME(type)                                                                                   \
    void setObjectName(type object, const std::string& name) const                                                     \
    {                                                                                                                  \
        setObjectName(objectHandle(object), name, object.objectType);                                                  \
    }

    DEFINE_SET_OBJECT_NAME(vk::Buffer)
    DEFINE_SET_OBJECT_NAME(vk::BufferView)
    DEFINE_SET_OBJECT_NAME(vk::CommandBuffer)
    DEFINE_SET_OBJECT_NAME(vk::CommandPool)
    DEFINE_SET_OBJECT_NAME(vk::DescriptorPool)
    DEFINE_SET_OBJECT_NAME(vk::DescriptorSet)
    DEFINE_SET_OBJECT_NAME(vk::DescriptorSetLayout)
    DEFINE_SET_OBJECT_NAME(vk::Device)
    DEFINE_SET_OBJECT_NAME(vk::DeviceMemory)
    DEFINE_SET_OBJECT_NAME(vk::Framebuffer)
    DEFINE_SET_OBJECT_NAME(vk::Image)
    DEFINE_SET_OBJECT_NAME(vk::ImageView)
    DEFINE_SET_OBJECT_NAME(vk::Pipeline)
    DEFINE_SET_OBJECT_NAME(vk::PipelineLayout)
    DEFINE_SET_OBJECT_NAME(vk::QueryPool)
    DEFINE_SET_OBJECT_NAME(vk::Queue)
    DEFINE_SET_OBJECT_NAME(vk::RenderPass)
    DEFINE_SET_OBJECT_NAME(vk::Sampler)
    DEFINE_SET_OBJECT_NAME(vk::Semaphore)
    DEFINE_SET_OBJECT_NAME(vk::ShaderModule)
    DEFINE_SET_OBJECT_NAME(vk::SwapchainKHR)

#if VK_NV_ray_tracing
    DEFINE_SET_OBJECT_NAME(vk::AccelerationStructureNV)
#endif
#if VK_KHR_acceleration_structure
    DEFINE_SET_OBJECT_NAME(vk::AccelerationStructureKHR)
#endif

    //---------------------------------------------------------------------------
    //
    void beginLabel(vk::CommandBuffer cmdBuf, const std::string& label)
    {
        if (sEnabled) {
            const vk::DebugUtilsLabelEXT labelInfo(label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f});
            cmdBuf.beginDebugUtilsLabelEXT(labelInfo);
        }
    }

    void endLabel(vk::CommandBuffer cmdBuf)
    {
        if (sEnabled) {
            cmdBuf.endDebugUtilsLabelEXT();
        }
    }

    void insertLabel(vk::CommandBuffer cmdBuf, const std::string& label)
    {
        if (sEnabled) {
            const vk::DebugUtilsLabelEXT labelInfo(label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f});
            cmdBuf.insertDebugUtilsLabelEXT(labelInfo);
        }
    }

    struct ScopedCmdLabel
    {
        ScopedCmdLabel(vk::CommandBuffer cmdBuf, const std::string& label) : _cmdBuf(cmdBuf)
        {
            if (sEnabled) {
                const vk::DebugUtilsLabelEXT labelInfo(label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f});
                cmdBuf.beginDebugUtilsLabelEXT(labelInfo);
            }
        }

        ~ScopedCmdLabel()
        {
            if (sEnabled) {
                _cmdBuf.endDebugUtilsLabelEXT();
            }
        }

        void insertLabel(const std::string& label) const
        {
            if (sEnabled) {
                const vk::DebugUtilsLabelEXT labelInfo(label.c_str(), {1.0f, 1.0f, 1.0f, 1.0f});
                _cmdBuf.insertDebugUtilsLabelEXT(labelInfo);
            }
        }

    private:
        vk::CommandBuffer _cmdBuf;
    };

    ScopedCmdLabel scopeLabel(vk::CommandBuffer cmdBuf, const std::string& label)
    {
        return ScopedCmdLabel(cmdBuf, label);
    }

private:
    vk::Device _device = VK_NULL_HANDLE;
    inline static bool sEnabled = false;
};
} // namespace bee