/**
 * @File VK_DeviceDriver.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#include "GFX/Vulkan/VK_DeviceDriver.hpp"

using namespace bee;

VK_DeviceDriver::VK_DeviceDriver(VK_Context* context) : _context(context)
{
}

VK_DeviceDriver::~VK_DeviceDriver()
{
    
}

Error VK_DeviceDriver::create(u32 deviceIndex, u32 frameCount)
{
    LogInfo("创建 Vulkan 渲染驱动...");
    // TODO：从 context 中获取物理设备 & 队列信息
    // TODO：Extensions 和 Layers
    // TODO：设备特性与 Capabilities
    // TODO：创建设备
    // TODO：创建分配器 vma，自定义缓冲池等
    // TODO：创建 pipeline cache
    
    
    return Error::Ok;
}

void VK_DeviceDriver::destroy()
{
    LogInfo("Vulkan 渲染驱动已摧毁");
}

void VK_DeviceDriver::createBuffer()
{
}

void VK_DeviceDriver::createTexture()
{
}

void VK_DeviceDriver::createSampler()
{
}

void VK_DeviceDriver::createSemaphore()
{
}

void VK_DeviceDriver::createFence()
{
}

void VK_DeviceDriver::createSwapChain()
{
}

void VK_DeviceDriver::createFramebuffer()
{
}

void VK_DeviceDriver::createRenderPass()
{
}

void VK_DeviceDriver::createShader()
{
}

void VK_DeviceDriver::createComputePipeline()
{
}

void VK_DeviceDriver::createRenderPipeline()
{
}

bool VK_DeviceDriver::createPipelineCache()
{
    return false;
}

void VK_DeviceDriver::createInputLayout()
{
}

void VK_DeviceDriver::createUniformSet()
{
}

void VK_DeviceDriver::createQueryPool()
{
}

void VK_DeviceDriver::createCommandQueue()
{
}

void VK_DeviceDriver::createCommandPool()
{
}

void VK_DeviceDriver::createCommandBuffer()
{
}

u8* VK_DeviceDriver::mapBuffer()
{
    return nullptr;
}

void VK_DeviceDriver::unmapBuffer()
{
}

u8* VK_DeviceDriver::mapTexture()
{
    return nullptr;
}

void VK_DeviceDriver::unmapTexture()
{
}

Error VK_DeviceDriver::waitFence()
{
    return Error::Ok;
}

Error VK_DeviceDriver::resizeSwapChain()
{
    return Error::Ok;
}

void VK_DeviceDriver::acquireFramebufferFromSwapChain()
{
}

void VK_DeviceDriver::renderPassInSwapChain()
{
}

void VK_DeviceDriver::swapChainFormat()
{
}

Vector<u8> VK_DeviceDriver::compileShaderToBinary()
{
    return bee::Vector<u8>();
}

void VK_DeviceDriver::queryResults()
{
}

u64 VK_DeviceDriver::queryResultToTime(u64 result)
{
    return 0;
}

void VK_DeviceDriver::commandQueueFamily()
{
}

Error VK_DeviceDriver::commandQueueExecuteAndPresent()
{
    return Error::Ok;
}

bool VK_DeviceDriver::commandBufferBegin()
{
    return false;
}

void VK_DeviceDriver::commandBufferEnd()
{
}

bool VK_DeviceDriver::commandBufferBeginSecondary()
{
    return false;
}

void VK_DeviceDriver::commandBufferExecuteSecondary()
{
}

void VK_DeviceDriver::clearBuffer()
{
}

void VK_DeviceDriver::copyBuffer()
{
}

void VK_DeviceDriver::copyTexture()
{
}

void VK_DeviceDriver::resolveTexture()
{
}

void VK_DeviceDriver::clearTexture()
{
}

void VK_DeviceDriver::copyBufferToTexture()
{
}

void VK_DeviceDriver::copyTextureToBuffer()
{
}

void VK_DeviceDriver::bindPushConstants()
{
}

void VK_DeviceDriver::beginRenderPass()
{
}

void VK_DeviceDriver::endRenderPass()
{
}

void VK_DeviceDriver::nextRenderSubpass()
{
}

void VK_DeviceDriver::setViewport()
{
}

void VK_DeviceDriver::setScissor()
{
}

void VK_DeviceDriver::clearAttachments()
{
}

void VK_DeviceDriver::bindRenderPipeline()
{
}

void VK_DeviceDriver::bindRenderUniformSet()
{
}

void VK_DeviceDriver::draw()
{
}

void VK_DeviceDriver::drawIndexed()
{
}

void VK_DeviceDriver::drawIndexedIndirect()
{
}

void VK_DeviceDriver::drawIndexedIndirectCount()
{
}

void VK_DeviceDriver::drawIndirect()
{
}

void VK_DeviceDriver::drawIndirectCount()
{
}

void VK_DeviceDriver::bindVertexBuffers()
{
}

void VK_DeviceDriver::bindIndexBuffer()
{
}

void VK_DeviceDriver::setBlendConstants()
{
}

void VK_DeviceDriver::setLineWidth()
{
}

void VK_DeviceDriver::bindComputePipeline()
{
}

void VK_DeviceDriver::bindComputeUniformSet()
{
}

void VK_DeviceDriver::dispatch()
{
}

void VK_DeviceDriver::dispatchIndirect()
{
}

void VK_DeviceDriver::queryPoolReset()
{
}

void VK_DeviceDriver::writeTimestamp()
{
}

void VK_DeviceDriver::beginLabel()
{
}

void VK_DeviceDriver::endLabel()
{
}

void VK_DeviceDriver::setObjectName()
{
}

u64 VK_DeviceDriver::getResourceNativeHandle()
{
    return 0;
}

u64 VK_DeviceDriver::getTotalMemoryUsed()
{
    return 0;
}

u64 VK_DeviceDriver::getLimit()
{
    return 0;
}

bool VK_DeviceDriver::hasFeature()
{
    return false;
}

String VK_DeviceDriver::apiName() const
{
    return bee::String();
}

String VK_DeviceDriver::apiVersion() const
{
    return bee::String();
}

bool VK_DeviceDriver::isCompositeAlphaSupported() const
{
    return GFX_DeviceDriver::isCompositeAlphaSupported();
}

