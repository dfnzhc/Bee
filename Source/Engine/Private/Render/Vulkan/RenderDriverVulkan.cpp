/**
 * @File RenderDriverVulkan.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/25
 * @Brief This file is part of Bee.
 */

#include "Render/Vulkan/RenderDriverVulkan.hpp"

using namespace bee;

RenderDriverVulkan::RenderDriverVulkan(RenderContextVulkan* context) : _context(context)
{
}

Error RenderDriverVulkan::initialize(u32 deviceIndex, u32 frameCount)
{
    // TODO：从 context 中获取物理设备 & 队列信息
    // TODO：Extensions 和 Layers
    // TODO：设备特性与 Capabilities
    // TODO：创建设备
    // TODO：创建分配器 vma，自定义缓冲池等
    // TODO：创建 pipeline cache
    
    
    return Error::Ok;
}

void RenderDriverVulkan::createBuffer()
{
}

void RenderDriverVulkan::createTexture()
{
}

void RenderDriverVulkan::createSampler()
{
}

void RenderDriverVulkan::createSemaphore()
{
}

void RenderDriverVulkan::createFence()
{
}

void RenderDriverVulkan::createSwapChain()
{
}

void RenderDriverVulkan::createFramebuffer()
{
}

void RenderDriverVulkan::createRenderPass()
{
}

void RenderDriverVulkan::createShader()
{
}

void RenderDriverVulkan::createComputePipeline()
{
}

void RenderDriverVulkan::createRenderPipeline()
{
}

bool RenderDriverVulkan::createPipelineCache()
{
    return false;
}

void RenderDriverVulkan::createInputLayout()
{
}

void RenderDriverVulkan::createUniformSet()
{
}

void RenderDriverVulkan::createQueryPool()
{
}

void RenderDriverVulkan::createCommandQueue()
{
}

void RenderDriverVulkan::createCommandPool()
{
}

void RenderDriverVulkan::createCommandBuffer()
{
}

u8* RenderDriverVulkan::mapBuffer()
{
    return nullptr;
}

void RenderDriverVulkan::unmapBuffer()
{
}

u8* RenderDriverVulkan::mapTexture()
{
    return nullptr;
}

void RenderDriverVulkan::unmapTexture()
{
}

Error RenderDriverVulkan::waitFence()
{
    return Error::Ok;
}

Error RenderDriverVulkan::resizeSwapChain()
{
    return Error::Ok;
}

void RenderDriverVulkan::acquireFramebufferFromSwapChain()
{
}

void RenderDriverVulkan::renderPassInSwapChain()
{
}

void RenderDriverVulkan::swapChainFormat()
{
}

Vector<u8> RenderDriverVulkan::compileShaderToBinary()
{
    return bee::Vector<u8>();
}

void RenderDriverVulkan::queryResults()
{
}

u64 RenderDriverVulkan::queryResultToTime(u64 result)
{
    return 0;
}

void RenderDriverVulkan::commandQueueFamily()
{
}

Error RenderDriverVulkan::commandQueueExecuteAndPresent()
{
    return Error::Ok;
}

bool RenderDriverVulkan::commandBufferBegin()
{
    return false;
}

void RenderDriverVulkan::commandBufferEnd()
{
}

bool RenderDriverVulkan::commandBufferBeginSecondary()
{
    return false;
}

void RenderDriverVulkan::commandBufferExecuteSecondary()
{
}

void RenderDriverVulkan::clearBuffer()
{
}

void RenderDriverVulkan::copyBuffer()
{
}

void RenderDriverVulkan::copyTexture()
{
}

void RenderDriverVulkan::resolveTexture()
{
}

void RenderDriverVulkan::clearTexture()
{
}

void RenderDriverVulkan::copyBufferToTexture()
{
}

void RenderDriverVulkan::copyTextureToBuffer()
{
}

void RenderDriverVulkan::bindPushConstants()
{
}

void RenderDriverVulkan::beginRenderPass()
{
}

void RenderDriverVulkan::endRenderPass()
{
}

void RenderDriverVulkan::nextRenderSubpass()
{
}

void RenderDriverVulkan::setViewport()
{
}

void RenderDriverVulkan::setScissor()
{
}

void RenderDriverVulkan::clearAttachments()
{
}

void RenderDriverVulkan::bindRenderPipeline()
{
}

void RenderDriverVulkan::bindRenderUniformSet()
{
}

void RenderDriverVulkan::draw()
{
}

void RenderDriverVulkan::drawIndexed()
{
}

void RenderDriverVulkan::drawIndexedIndirect()
{
}

void RenderDriverVulkan::drawIndexedIndirectCount()
{
}

void RenderDriverVulkan::drawIndirect()
{
}

void RenderDriverVulkan::drawIndirectCount()
{
}

void RenderDriverVulkan::bindVertexBuffers()
{
}

void RenderDriverVulkan::bindIndexBuffer()
{
}

void RenderDriverVulkan::setBlendConstants()
{
}

void RenderDriverVulkan::setLineWidth()
{
}

void RenderDriverVulkan::bindComputePipeline()
{
}

void RenderDriverVulkan::bindComputeUniformSet()
{
}

void RenderDriverVulkan::dispatch()
{
}

void RenderDriverVulkan::dispatchIndirect()
{
}

void RenderDriverVulkan::queryPoolReset()
{
}

void RenderDriverVulkan::writeTimestamp()
{
}

void RenderDriverVulkan::beginLabel()
{
}

void RenderDriverVulkan::endLabel()
{
}

void RenderDriverVulkan::setObjectName()
{
}

u64 RenderDriverVulkan::getResourceNativeHandle()
{
    return 0;
}

u64 RenderDriverVulkan::getTotalMemoryUsed()
{
    return 0;
}

u64 RenderDriverVulkan::getLimit()
{
    return 0;
}

bool RenderDriverVulkan::hasFeature()
{
    return false;
}

String RenderDriverVulkan::apiName() const
{
    return bee::String();
}

String RenderDriverVulkan::apiVersion() const
{
    return bee::String();
}

bool RenderDriverVulkan::isCompositeAlphaSupported() const
{
    return RenderDriver::isCompositeAlphaSupported();
}
