/**
 * @File EngineTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/15
 * @Brief This file is part of Bee.
 */

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <GFX/Vulkan/VK_Context.hpp>
#include <GFX/Vulkan/VK_DeviceDriver.hpp>
#include <GFX/Vulkan/VK_PhysicalDevice.hpp>

#include "Base/Globals.hpp"
#include <vulkan/vulkan.hpp>

#include <gtest/gtest.h>

using namespace bee;

TEST(GFX_Vulkan, Context)
{
    auto ctx = std::make_unique<VK_Context>();

    GFX_Context::Config config;
    config.headless = false;
    EXPECT_EQ(ctx->create(config), Error::Ok);
    EXPECT_TRUE(ctx->instance() != VK_NULL_HANDLE);

    EXPECT_TRUE(ctx->deviceCount() > 0);

    EXPECT_TRUE(ctx->apiVersion() >= VK_API_VERSION_1_3);

    // check extensions
    {
        const auto extensions = ctx->enabledExtensions();

        std::vector<const char*> extNames
        {
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        };
        if (!config.headless) {
            extNames.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
            extNames.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        }

        if (Globals::EnableValidationLayer()) {
            extNames.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            extNames.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        for (const auto* ext : extNames) {
            EXPECT_TRUE(std::ranges::find_if(extensions, [ext](const char* name) {
                return std::strcmp(ext, name) == 0;
                }) != extensions.end());
        }
    }

    // check layers
    {
        const auto layers = ctx->enabledLayers();

        std::vector<const char*> layNames;
        if (Globals::EnableValidationLayer()) {
            layNames.emplace_back("VK_LAYER_KHRONOS_validation");
        }

        for (const auto* lay : layNames) {
            EXPECT_TRUE(std::ranges::find_if(layers, [lay](const char* name) {
                return std::strcmp(lay, name) == 0;
                }) != layers.end());
        }
    }

    ctx->destroy();
}

TEST(GFX_Vulkan, PhysicalDevice)
{
    auto ctx = std::make_unique<VK_Context>();

    GFX_Context::Config config;
    config.headless = false;
    ctx->create(config);

    EXPECT_TRUE(ctx->deviceCount() > 0);

    u32 devId = 0;
    auto physicalDevice = std::make_unique<VK_PhysicalDevice>();
    EXPECT_EQ(physicalDevice->setup(ctx->physicalDevice(devId)), Error::Ok);

    // extensions
    {
        physicalDevice->registerExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);
        physicalDevice->registerExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, true);
        physicalDevice->registerExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, true);

        EXPECT_TRUE(physicalDevice->enabledExtensions().empty());
        physicalDevice->finalizeRegister();
        EXPECT_FALSE(physicalDevice->enabledExtensions().empty());

        EXPECT_TRUE(physicalDevice->isExtensionEnabled(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
        EXPECT_TRUE(physicalDevice->isExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME));
        EXPECT_TRUE(physicalDevice->isExtensionEnabled(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME));
    }

    // queues
    {
        const auto flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer ;
        physicalDevice->requestQueues(flags);

        EXPECT_TRUE(physicalDevice->graphicsFamilyCount() > 0);
        EXPECT_TRUE(physicalDevice->transferFamilyCount() > 0);
        
        EXPECT_TRUE(physicalDevice->presentationFamilyCount() == 0);
        EXPECT_TRUE(physicalDevice->computeFamilyCount() == 0);
    }
}